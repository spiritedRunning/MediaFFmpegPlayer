package com.example.mediaffmpegplayer;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;


import androidx.appcompat.app.AppCompatActivity;

import com.example.mediaffmpegplayer.listener.IPlayerListener;
import com.example.mediaffmpegplayer.listener.WlOnPreparedListener;
import com.example.mediaffmpegplayer.opengl.MNGLSurfaceView;
import com.example.mediaffmpegplayer.player.NPlayerInterface;
import com.example.mediaffmpegplayer.utils.DisplayUtil;

import java.io.File;
import java.util.ArrayList;
import java.util.List;


public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";

    private NPlayerInterface nPlayerInterface;
    private TextView tvStartTime, tvEndTime;
    private MNGLSurfaceView mnGLSurfaceView;
    private SeekBar seekBar;
    private int position;
    private boolean seek = false;
    private List<String> paths = new ArrayList<>();


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        checkPermission();
        initView();
    }

    private void initView() {
        tvStartTime = findViewById(R.id.tv_starttime);
        tvEndTime = findViewById(R.id.tv_endtime);
        mnGLSurfaceView = findViewById(R.id.wlglsurfaceview);
        seekBar = findViewById(R.id.seekbar);

        nPlayerInterface = new NPlayerInterface();
        nPlayerInterface.setMNGLSurfaceView(mnGLSurfaceView);

        File file = new File(Environment.getExternalStorageDirectory(), "input.mkv");
        paths.add(file.getAbsolutePath());
        file = new File(Environment.getExternalStorageDirectory(), "input.avi");
        paths.add(file.getAbsolutePath());
        file = new File(Environment.getExternalStorageDirectory(), "input.rmvb");
        paths.add(file.getAbsolutePath());
        paths.add("http://mn.maliuedu.com/music/input.mp4");


        nPlayerInterface.setPlayerListener(new IPlayerListener() {
            @Override
            public void onLoad(boolean load) {

            }

            @Override
            public void onCurrentTime(final int currentTime, final int totalTime) {
                if (!seek && totalTime > 0) {
                    runOnUiThread(new Runnable() {
                        @SuppressLint("SetTextI18n")
                        @Override
                        public void run() {
                            seekBar.setProgress(currentTime * 100 / totalTime);
                            tvStartTime.setText(DisplayUtil.secdsToDateFormat(currentTime, 0));
                            tvEndTime.setText(DisplayUtil.secdsToDateFormat(totalTime, 0));

                        }
                    });

                }
            }

            @Override
            public void onError(int code, String msg) {

            }

            @Override
            public void onPause(boolean pause) {

            }

            @Override
            public void onDbValue(int db) {

            }

            @Override
            public void onComplete() {

            }

            @Override
            public String onNext() {
                return null;
            }
        });

        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                position = progress * nPlayerInterface.getDuration() / 100;
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                seek = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                nPlayerInterface.seek(position);
                seek = false;
            }
        });

    }

    public boolean checkPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(
                Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
            }, 1);

        }
        return false;
    }

    public void begin(View view) {
        nPlayerInterface.setWlOnPreparedListener(new WlOnPreparedListener() {
            @Override
            public void onPrepared() {
                Log.d(TAG, "准备好了，可以开始播放声音了");
                nPlayerInterface.start();
            }
        });

        File file = new File(Environment.getExternalStorageDirectory(), "v1080.mp4");
        nPlayerInterface.setSource(file.getAbsolutePath());

//        nPlayerInterface.setSource("http://mn.maliuedu.com/music/input.mp4");
        nPlayerInterface.prepared();
    }

    public void pause(View view) {
        nPlayerInterface.pause();
    }

    public void resume(View view) {
        nPlayerInterface.resume();
    }


    public void stop(View view) {
        nPlayerInterface.stop();
    }


    public void next(View view) {

    }

    public void speed1(View view) {
        nPlayerInterface.setSpeed(1.5f);
    }

    public void speed2(View view) {
        nPlayerInterface.setSpeed(2.0f);
    }
}
