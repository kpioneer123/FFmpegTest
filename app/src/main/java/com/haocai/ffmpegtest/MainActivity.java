package com.haocai.ffmpegtest;

import android.content.Intent;
import android.os.Bundle;
import java.io.File;
import android.app.Activity;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import com.haocai.ffmpegtest.util.VideoPlayer;

import butterknife.ButterKnife;
import butterknife.OnClick;

public class MainActivity extends Activity {

//    @BindView(R.id.btn_decode)
//    Button btnDecode;
//    @BindView(R.id.btn_play)
//    Button btn_play;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ButterKnife.bind(this);
    }
    @OnClick({R.id.btn_decode,R.id.btn_render, R.id.btn_play,R.id.btn_audio_decode,R.id.btn_audio_player})
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.btn_decode:
                doDecode();
                break;
            case R.id.btn_render:
                Intent intent = new Intent(MainActivity.this,SimpleRenderActivity.class);
                MainActivity.this.startActivity(intent);
                break;
            case R.id.btn_audio_decode:

                audioDecode();
                break;
            case R.id.btn_audio_player:
                audioPlayer();
                break;
            case R.id.btn_play:
                play();
                break;
        }
    }

    /**
     * 视频解码
     */
    public void doDecode(){
        String input = new File(Environment.getExternalStorageDirectory(),"小苹果.mp4").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(),"小苹果_out.yuv").getAbsolutePath();
        VideoPlayer.decode(input, output);
        Toast.makeText(this,"正在解码...",Toast.LENGTH_SHORT).show();
    }

    /**
     * 音频解码
     */
    public void audioDecode(){
        String input = new File(Environment.getExternalStorageDirectory(),"说散就散.mp3").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(),"说散就散.pcm").getAbsolutePath();
        VideoPlayer player = new VideoPlayer();
        player.audioDecode(input, output);
        Toast.makeText(this,"正在解码...",Toast.LENGTH_SHORT).show();
    }
    /**
     * 音频解码
     */
    public void audioPlayer(){
        /**
         * 1.播放视频文件中的音频
         */
    //   String input = new File(Environment.getExternalStorageDirectory(),"告白气球.avi").getAbsolutePath();

        /**
         * 2.播放音频文件中的音频
         */
        String input = new File(Environment.getExternalStorageDirectory(),"说散就散.mp3").getAbsolutePath();
        VideoPlayer player = new VideoPlayer();
        player.audioPlayer(input);
        Log.d("Main","正在播放");
    }

    public void play(){
        Intent intent = new Intent(MainActivity.this,SimplePlayActivity.class);
        MainActivity.this.startActivity(intent);
    }
}
