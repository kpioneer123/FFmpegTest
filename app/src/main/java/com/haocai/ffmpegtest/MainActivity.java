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
    private  VideoPlayer player ;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ButterKnife.bind(this);
        player = new VideoPlayer();
    }
    @OnClick({R.id.btn_decode,R.id.btn_render, R.id.btn_play,R.id.btn_audio_decode,R.id.btn_audio_player,R.id.btn_transcoding_compress})
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
            case R.id.btn_transcoding_compress:
                transcodingCompress();
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
        player.audioPlayer(input);
        Log.d("Main","正在播放");
    }

    public void play(){
        Intent intent = new Intent(MainActivity.this,SimplePlayActivity.class);
        MainActivity.this.startActivity(intent);
    }

    public void transcodingCompress(){

        final File inputFile;
        final File outputFile;
        final File dic = Environment.getExternalStorageDirectory();
        inputFile = new File(dic,"告白气球.avi");
        outputFile = new File(dic,"告白气球.mp4");

        new Thread(new Runnable() {
            @Override
            public void run() {
                //获取视频文件的路径(sdcard路径)
                // 拼接cmd 指令 ffmpeg -i 告白气球.avi -b:v 640k 告白气球.mp4 (与windows 命令行相同)
                StringBuilder builder = new StringBuilder();
                builder.append("ffmpeg ");
                builder.append("-i ");
                builder.append(inputFile.getAbsolutePath()+" ");
                builder.append("-b:v 640k ");  //码率越大 清晰度越高 码率越小 清晰度越低
                builder.append(outputFile.getAbsolutePath());
                String[] argv = builder.toString().split(" ");
                int argc = argv.length;

                player.transcodingCompress(argc,argv);
            }
        }).start();

    }
}
