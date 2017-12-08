package com.haocai.ffmpegtest.view;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * 视频绘制的"画布"
 */
public class VideoView extends SurfaceView {

	public VideoView(Context context) {
		super(context);
		init();
	}

	public VideoView(Context context, AttributeSet attrs) {
		super(context, attrs);
		init();
	}

	public VideoView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		init();
	}
	
	private void init(){
		//初始化，SufaceView绘制的像素格式
		SurfaceHolder holder = getHolder();
		holder.setFormat(PixelFormat.RGBA_8888);
	}
	

}
