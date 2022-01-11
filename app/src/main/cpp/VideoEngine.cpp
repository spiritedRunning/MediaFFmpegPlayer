//
// Created by Zach on 2022/1/11.
//

#include "VideoEngine.h"

VideoEngine::VideoEngine(PlayStatus *playStatus, CallJavaWrapper *wlCallJava) {
    this->playStatus = playStatus;
    this->wlCallJava = wlCallJava;
    queue = new DataQueue(playStatus);
    pthread_mutex_init(&codecMutex, NULL);
}

VideoEngine::~VideoEngine() {
    pthread_mutex_destroy(&codecMutex);
}

void *playVideo(void *data) {
    VideoEngine *video = static_cast<VideoEngine *>(data);
    while (video->playStatus != NULL && !video->playStatus->exit) {
        if (video->playStatus->seek) {
            av_usleep(1000 * 100);
            continue;
        }
        if (video->playStatus->pause) {
            av_usleep(1000 * 100);
            continue;
        }
        if (video->queue->getQueueSize() == 0) { // 可能网络不佳，通知上层等待
            if (!video->playStatus->load) {
                video->playStatus->load = true;
                video->wlCallJava->onCallLoad(CHILD_THREAD, true);
                av_usleep(1000 * 100);
                continue;
            }

        }

        AVPacket *avPacket = av_packet_alloc();
        if (video->queue->getAvPacket(avPacket) != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }
        // 视频解码 比较耗时
        pthread_mutex_lock(&video->codecMutex);

        if (avcodec_send_packet(video->avCodecContext, avPacket) != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&video->codecMutex);
            continue;
        }
        AVFrame *avFrame = av_frame_alloc();

        if (avcodec_receive_frame(video->avCodecContext, avFrame) != 0) {
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&video->codecMutex);
            continue;
        }

        if (avFrame->format == AV_PIX_FMT_YUV420P) {
            LOGE("当前视频是YUV420P格式");
//            av_usleep(33 * 1000);
            double diff = video->getFrameDiffTime(avFrame);
            av_usleep(video->getDelayTime(diff) * 1000000);

            video->wlCallJava->onCallRenderYUV(
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    avFrame->data[0], // y
                    avFrame->data[1], // u
                    avFrame->data[2]); // v
        } else {
            LOGE("当前视频不是YUV420P格式");
            AVFrame *pFrameYUV420P = av_frame_alloc();
            int num = av_image_get_buffer_size(
                    AV_PIX_FMT_YUV420P,
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    1);
            uint8_t *buffer = static_cast<uint8_t *>(av_malloc(num * sizeof(uint8_t)));
            av_image_fill_arrays(
                    pFrameYUV420P->data,
                    pFrameYUV420P->linesize,
                    buffer,
                    AV_PIX_FMT_YUV420P,
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    1);
            SwsContext *sws_ctx = sws_getContext(
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    video->avCodecContext->pix_fmt,
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    AV_PIX_FMT_YUV420P,
                    SWS_BICUBIC, NULL, NULL, NULL);

            if (!sws_ctx) {
                av_frame_free(&pFrameYUV420P);
                av_free(pFrameYUV420P);
                av_free(buffer);
                pthread_mutex_unlock(&video->codecMutex);
                continue;
            }
            sws_scale(
                    sws_ctx,
                    reinterpret_cast<const uint8_t *const *>(avFrame->data),
                    avFrame->linesize,
                    0,
                    avFrame->height,
                    pFrameYUV420P->data,
                    pFrameYUV420P->linesize);
            //渲染
            video->wlCallJava->onCallRenderYUV(
                    video->avCodecContext->width,
                    video->avCodecContext->height,
                    pFrameYUV420P->data[0],
                    pFrameYUV420P->data[1],
                    pFrameYUV420P->data[2]);

            av_frame_free(&pFrameYUV420P);
            av_free(pFrameYUV420P);
            av_free(buffer);
            sws_freeContext(sws_ctx);
        }
        av_frame_free(&avFrame);
        av_free(avFrame);
        avFrame = NULL;
        av_packet_free(&avPacket);
        av_free(avPacket);
        avPacket = NULL;
        pthread_mutex_unlock(&video->codecMutex);
    }
    pthread_exit(&video->thread_play);
}

void VideoEngine::play() {
    pthread_create(&thread_play, NULL, playVideo, this);
}

double VideoEngine::getDelayTime(double diff) {
    if (diff > 0.003) { // 音频超越视频3ms
        delayTime = delayTime * 2 / 3;
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff < -0.003) { // 视频超越音频
        delayTime = delayTime * 3 / 2;
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    }

    if (diff >= 0.5) {
        delayTime = 0;
    } else if (diff <= -0.5) {
        delayTime = defaultDelayTime * 2;
    }

    if (diff >= 10) { // 音频太快, 直接清空
        queue->clearAvPacket();
        delayTime = defaultDelayTime;
    }

    if (diff <= -10) { // 视频太快 音频赶不上
        audio->queue->clearAvPacket();
        delayTime = defaultDelayTime;
    }
    return delayTime; // 视频休眠时间
}

double VideoEngine::getFrameDiffTime(AVFrame *avFrame) {
    double pts = av_frame_get_best_effort_timestamp(avFrame);
    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }
    LOGD("pts = %f", pts );
    // pts = pts * time_base.num / time_base.den;
    pts *= av_q2d(time_base);
    LOGD("pts2 = %f", pts );
    if (pts > 0) {
        clock = pts;
    }
    return audio->clock - clock;
}
