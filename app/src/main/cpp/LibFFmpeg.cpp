//
// Created by Zach on 2021/12/15.
//

#include "LibFFmpeg.h"

LibFFmpeg::LibFFmpeg(PlayStatus *playstatus, CallJavaWrapper *callJava, const char *url) {
    this->playStatus = playstatus;
    this->callJava = callJava;
    this->url = url;
    pthread_mutex_init(&init_mutex, NULL);
    pthread_mutex_init(&seek_mutex, NULL);
}

LibFFmpeg::~LibFFmpeg() {
    pthread_mutex_destroy(&seek_mutex);
    pthread_mutex_destroy(&init_mutex);
}


void *decodeFFmpeg(void *data) {
    LibFFmpeg *wlFFmpeg = (LibFFmpeg *) data;
    wlFFmpeg->decodeFFmpegThread();
    pthread_exit(&wlFFmpeg->decodeThread);
}

void LibFFmpeg::prepared() {
    pthread_create(&decodeThread, NULL, decodeFFmpeg, this);
}

void LibFFmpeg::start() {
    if (audio == NULL) {
        if (LOG_DEBUG) {
            LOGE("audio is null");
            return;
        }
    }
    audio->play();
    video->play();
    video->audio = audio;

    int count = 0;
    while (playStatus != NULL && !playStatus->exit) {
        if (playStatus->seek) {  // important!  seek后清空数据，不能播放已清空的数据
            continue;
        }
        // 放入队列
        if (audio->queue->getQueueSize() > 40) {
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();
        if (av_read_frame(pFormatCtx, avPacket) == 0) {
            if (avPacket->stream_index == audio->streamIndex) {
                //解码操作
                count++;
                if (LOG_DEBUG) {
                    LOGE("audio decode %d frame", count);
                }
                audio->queue->putAvPacket(avPacket);
            } else if (avPacket->stream_index == video->steamIndex) {
                if (LOG_DEBUG) {
                    LOGE("video decode %d frame", count);
                }
                video->queue->putAvPacket(avPacket);
            } else {
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else {
            av_packet_free(&avPacket);
            av_free(avPacket);
            while (playStatus != NULL && !playStatus->exit) {
                if (audio->queue->getQueueSize() > 0) {
                    continue;
                } else if (video->queue->getQueueSize() > 0) {
                    continue;
                } else {
                    playStatus->exit = true;
                    break;
                }
            }
        }
    }

    if (LOG_DEBUG) {
        LOGD("解码完成");
    }
}

void LibFFmpeg::decodeFFmpegThread() {
    pthread_mutex_lock(&init_mutex);
    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    int err_code;
    char buf[1024];
    err_code = avformat_open_input(&pFormatCtx, url, NULL, NULL);
    if (err_code != 0) {
        av_strerror(err_code, buf, 1024);
        LOGE("can not open url: %s, errMsg: %s", url, buf);
        return;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find streams from %s", url);
        }
        return;
    }
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) { //得到音频流
            if (audio == NULL) {
                audio = new AudioEngine(playStatus, pFormatCtx->streams[i]->codecpar->sample_rate,
                                        callJava);
                audio->streamIndex = i;
                audio->codecpar = pFormatCtx->streams[i]->codecpar;

                audio->duration = pFormatCtx->duration / AV_TIME_BASE;  // 总时长
                audio->time_base = pFormatCtx->streams[i]->time_base;
                duration = audio->duration;
            }
        } else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (video == NULL) {
                video = new VideoEngine(playStatus, callJava);
                video->steamIndex = i;
                video->codecpar = pFormatCtx->streams[i]->codecpar;

                video->time_base = pFormatCtx->streams[i]->time_base;
                int num = pFormatCtx->streams[i]->avg_frame_rate.num;
                int den = pFormatCtx->streams[i]->avg_frame_rate.den;
                if (num != 0 && den != 0) {
                    int fps = num / den;
                    video->defaultDelayTime = 1.0 / fps;
                }
            }
        }
    }

    if (audio != NULL) {
        getCodecContext(audio->codecpar, &audio->avCodecContext);
    }
    if (video != NULL) {
        getCodecContext(video->codecpar, &video->avCodecContext);
    }

    callJava->onCallPrepared(CHILD_THREAD);
    pthread_mutex_unlock(&init_mutex);
}


int LibFFmpeg::getCodecContext(AVCodecParameters *codecpar, AVCodecContext **avCodecContext) {
    AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
    if (!dec) {
        if (LOG_DEBUG) {
            LOGE("can not find decoder");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    *avCodecContext = avcodec_alloc_context3(dec);
    if (!audio->avCodecContext) {
        if (LOG_DEBUG) {
            LOGE("------------------can not alloc new decodecctx");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    if (avcodec_parameters_to_context(*avCodecContext, codecpar) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not fill decodecctx");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    if (avcodec_open2(*avCodecContext, dec, 0) != 0) {
        if (LOG_DEBUG) {
            LOGE("cant not open audio streams");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    return 0;
}

void LibFFmpeg::seek(int64_t sec) {
    if (duration <= 0) {
        return;
    }
    LOGE("seek start");
    if (sec >= 0 && sec <= duration) {
        if (audio != NULL && video != NULL) {
            playStatus->seek = true;
            audio->queue->clearAvPacket();
            audio->clock = 0;
            audio->last_tick = 0;

            video->queue->clearAvPacket();
            video->clock = 0;

            pthread_mutex_lock(&seek_mutex);
            int64_t rel = sec * AV_TIME_BASE;
            avformat_seek_file(pFormatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);
            pthread_mutex_unlock(&seek_mutex);

            LOGE("seek finish");
            playStatus->seek = false;
        }
    }
}

void LibFFmpeg::pause() {
    if (audio != NULL) {
        audio->pause();
    }

}

void LibFFmpeg::resume() {
    if (audio != NULL) {
        audio->resume();
    }
}

void LibFFmpeg::setChannel(int channel) {
    if (audio != NULL) {
        audio->setChannel(channel);
    }
}

void LibFFmpeg::setVolume(int vol) {
    if (audio != NULL) {
        audio->setVolume(vol);
    }
}

void LibFFmpeg::setSpeed(float speed) {
    if (audio != NULL) {
        audio->setSpeed(speed);
    }
}

void LibFFmpeg::setPitch(float pitch) {
    if (audio != NULL) {
        audio->setPitch(pitch);
    }
}

void LibFFmpeg::release() {
    if (LOG_DEBUG) {
        LOGE("开始释放ffmpeg");
    }
    playStatus->exit = true;

    int sleepCount = 0;
    pthread_mutex_lock(&init_mutex);
    while (!exit) {
        if (sleepCount > 1000) {
            exit = true;
        }
        if (LOG_DEBUG) {
            LOGE("wait ffmpeg  exit %d", sleepCount);
        }
        sleepCount++;
        av_usleep(1000 * 10); //暂停10毫秒
    }

    if (audio != NULL) {
        audio->release();
        delete (audio);
        audio = NULL;
    }
    if (video != NULL) {
        delete (video);
        video = NULL;
    }

    if (LOG_DEBUG) {
        LOGE("释放 封装格式上下文");
    }
    if (pFormatCtx != NULL) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放 callJava");
    }
    if (callJava != NULL) {
        callJava = NULL;
    }
    if (LOG_DEBUG) {
        LOGE("释放 playstatus");
    }
    if (playStatus != NULL) {
        playStatus = NULL;
    }
    pthread_mutex_unlock(&init_mutex);

}

