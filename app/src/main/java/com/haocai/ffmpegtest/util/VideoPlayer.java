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
