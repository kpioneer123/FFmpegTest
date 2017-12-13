package com.haocai.ffmpegtest;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.haocai.ffmpegtest.util.VideoPlayer;

import java.io.File;

import butterknife.BindView;
import butterknife.ButterKnife;

public class SimpleAudioPlayActivity extends Activity  implements SurfaceHolder.Callback {


    @BindView(R.id.video_view)
    SurfaceView videoView;
    private VideoPlayer player;
    SurfaceHolder surfaceHolder;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simple_play);
        ButterKnife.bind(this);
        player = new VideoPlayer();
        surfaceHolder = videoView.getHolder();
        //surface
        surfaceHolder.addCallback(this);
    }

    @Override
    public void surfaceCreated(final SurfaceHolder holder) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                String input = new File(Environment.getExternalStorageDirectory(),"小苹果.mp4").getAbsolutePath();
                Log.d("main",input);
                player.render(input, holder.getSurface());
            }
        }).start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        holder.getSurface().release();
    }
}