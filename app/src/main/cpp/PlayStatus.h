//
// Created by Zach on 2021/12/15.
//

#ifndef MUSICFFMPEGPLAYER_PLAYSTATUS_H
#define MUSICFFMPEGPLAYER_PLAYSTATUS_H

class PlayStatus {
public:
    bool exit;
    bool seek = false;
    bool pause = false;
    bool load = true;

    PlayStatus();
};


#endif //MUSICFFMPEGPLAYER_PLAYSTATUS_H
