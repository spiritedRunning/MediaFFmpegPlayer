//
// Created by Zach on 2022/1/11.
//

#ifndef MEDIAFFMPEGPLAYER_VIDEOENGINE_H
#define MEDIAFFMPEGPLAYER_VIDEOENGINE_H


#include "DataQueue.h"
#include "CallJavaWrapper.h"

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

public:
    VideoEngine(PlayStatus *playStatus, CallJavaWrapper *wlCallJava);
    ~VideoEngine();
    void play();

};


#endif //MEDIAFFMPEGPLAYER_VIDEOENGINE_H
