package com.example.mediaffmpegplayer.opengl;


import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;


public class MNGLSurfaceView extends GLSurfaceView {
    private MNRender mnRender;

    public MNGLSurfaceView(Context context) {
        this(context, null);
    }

    public MNGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        mnRender = new MNRender(context);
        setRenderer(mnRender);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    public void setYUVData(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (mnRender != null) {
            mnRender.setYUVRenderData(width, height, y, u, v);
            requestRender();
        }

    }
}
