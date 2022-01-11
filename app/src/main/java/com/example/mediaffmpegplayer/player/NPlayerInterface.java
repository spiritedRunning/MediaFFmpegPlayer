package com.example.mediaffmpegplayer.player;

import android.text.TextUtils;
import android.util.Log;

import com.example.mediaffmpegplayer.listener.IPlayerListener;
import com.example.mediaffmpegplayer.listener.WlOnPreparedListener;
import com.example.mediaffmpegplayer.opengl.MNGLSurfaceView;


/**
 * Created by Zach on 2021/12/15 16:41
 * <p>
 * java与native的互相回调接口
 */
public class NPlayerInterface {
    private static final String TAG = "NPlayerInterface";

    static {
        System.loadLibrary("native-lib");
    }

    private String source;//数据源
    private WlOnPreparedListener wlOnPreparedListener;

    private IPlayerListener playerListener;

    private MNGLSurfaceView mediaSurfaceView;

    private int duration;

    public void setPlayerListener(IPlayerListener playerListener) {
        this.playerListener = playerListener;
    }

    public NPlayerInterface() {
    }

    public void setSource(String source) {
        this.source = source;
    }

    public void setMNGLSurfaceView(MNGLSurfaceView surfaceView) {
        this.mediaSurfaceView = surfaceView;
    }

    public void setWlOnPreparedListener(WlOnPreparedListener wlOnPreparedListener) {
        this.wlOnPreparedListener = wlOnPreparedListener;
    }

    public int getDuration() {
        return duration;
    }

    public void prepared() {
        if (TextUtils.isEmpty(source)) {
            Log.d(TAG, "source not be empty");
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                n_prepared(source);
            }
        }).start();

    }

    public void start() {
        if (TextUtils.isEmpty(source)) {
            Log.d(TAG, "source is empty");
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                n_start();
            }
        }).start();
    }

    public void stop() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                n_stop();
            }
        }).start();
    }


    public void pause() {
        n_pause();
    }

    public void resume() {
        n_resume();
    }

    public void setChannel(int channel) {
        n_channel(channel);
    }


    public void seek(int sec) {
        n_seek(sec);
    }

    public void setVolume(int vol) {
        n_volume(vol);
    }

    public void setSpeed(float speed) {
        n_speed(speed);
    }


    public void setPitch(float pitch) {
        n_pitch(pitch);
    }


    public native void n_prepared(String source);

    public native void n_start();

    public native void n_stop();

    private native void n_seek(int sec);

    private native void n_resume();

    private native void n_pause();

    private native void n_channel(int channel);

    private native void n_volume(int vol);

    private native void n_speed(float speed);

    private native void n_pitch(float pitch);

    /*******************************************************************************************/
    /***** C++ 回调Java start ****/
    public void onCallPrepared() {
        if (wlOnPreparedListener != null) {
            wlOnPreparedListener.onPrepared();
        }
    }

    public void onCallTimeInfo(int currentTime, int totalTime) {
        if (playerListener == null) {
            return;
        }

        duration = totalTime;
        playerListener.onCurrentTime(currentTime, totalTime);
    }

    public void onCallLoad(boolean load) {
        Log.e(TAG, "onCallLoad load: " + load);
    }

    public void onCallRenderYUV(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (this.mediaSurfaceView != null) {
            this.mediaSurfaceView.setYUVData(width, height, y, u, v);
        }
    }

}
