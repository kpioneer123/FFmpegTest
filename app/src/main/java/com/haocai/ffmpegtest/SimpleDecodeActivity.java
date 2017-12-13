package com.haocai.ffmpegtest;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;

import com.haocai.ffmpegtest.util.VideoPlayer;

import java.io.File;

public class SimpleDecodeActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    public void mDecode(View btn){
        String input = new File(Environment.getExternalStorageDirectory(),"小苹果.mp4").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(),"小苹果_out.yuv").getAbsolutePath();
        VideoPlayer.decode(input, output);
    }
}
