package com.haocai.ffmpegtest;

import android.content.Intent;
import android.os.Bundle;
import java.io.File;
import android.app.Activity;
import android.os.Environment;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import butterknife.BindView;
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
    @OnClick({R.id.btn_decode, R.id.btn_play})
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.btn_decode:
                doDecode();
                break;
            case R.id.btn_play:
                Intent intent = new Intent(MainActivity.this,SimplePlayActivity.class);
                MainActivity.this.startActivity(intent);
                break;
        }
    }
    public void doDecode(){
        String input = new File(Environment.getExternalStorageDirectory(),"小苹果.mp4").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(),"小苹果_out.yuv").getAbsolutePath();
        VideoUtils.decode(input, output);
        Toast.makeText(this,"正在解码...",Toast.LENGTH_SHORT).show();
    }

}
