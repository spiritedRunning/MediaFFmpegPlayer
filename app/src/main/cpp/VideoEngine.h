//
// Created by Zach on 2022/1/11.
//

#ifndef MEDIAFFMPEGPLAYER_VIDEOENGINE_H
#define MEDIAFFMPEGPLAYER_VIDEOENGINE_H


#include "DataQueue.h"
#include "CallJavaWrapper.h"
#include "AudioEngine.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};


class VideoEngine {
public:
    DataQueue *queue = NULL;
    int steamIndex = -1;
    AVCodecContext *avCodecContext = NULL;
    AVCodecParameters *codecpar = NULL;
    PlayStatus *playStatus = NULL;
    CallJavaWrapper *wlCallJava = NULL;

    pthread_mutex_t codecMutex;
    pthread_t thread_play;

    double clock = 0;
    double delayTime = 0;  //  实时计算与音频的差值
    double defaultDelayTime = 0.04;  // 默认休眠时间40ms  帧率25帧
    AVRational time_base;
    AudioEngine *audio = NULL;


public:
    VideoEngine(PlayStatus *playStatus, CallJavaWrapper *wlCallJava);
    ~VideoEngine();
    void play();

    double getDelayTime(double diff);
    double getFrameDiffTime(AVFrame *avFrame);
};


#endif //MEDIAFFMPEGPLAYER_VIDEOENGINE_H
