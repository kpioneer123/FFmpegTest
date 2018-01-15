package com.haocai.ffmpegtest.util;

import android.view.Surface;

public class VideoPlayer {

    //视频解码
	public native static void decode(String input,String output);
	//视频播放
	public native void render(String input,Surface surface);
	//音频解码
    public native void audioDecode(String input,String output);
	//音频播放
	public native void audioPlayer(String input);
    //音视频播放
	public native void play(String input,Surface surface);

	//视频转码压缩
	public native void transcodingCompress(int argc,String[] argv);

	static{
		System.loadLibrary("avutil-54");
		System.loadLibrary("swresample-1");
		System.loadLibrary("avcodec-56");
		System.loadLibrary("avformat-56");
		System.loadLibrary("swscale-3");
		System.loadLibrary("postproc-53");
		System.loadLibrary("avfilter-5");
		System.loadLibrary("avdevice-56");
		System.loadLibrary("myffmpeg");
	}


}
