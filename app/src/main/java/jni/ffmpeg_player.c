#include <com_haocai_ffmpegtest_VideoUtils.h>
#include <android/log.h>
#include <stdio.h>
//解码
#include "include/libavcodec/avcodec.h"
//封装格式处理
#include "include/libavformat/avformat.h"
//像素处理
#include "include/libswscale/swscale.h"

#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"FFmpeg",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"FFmpeg",FORMAT,##__VA_ARGS__);

JNIEXPORT void JNICALL Java_com_haocai_ffmpegtest_VideoUtils_decode
(JNIEnv *env, jclass jcls, jstring input_jstr, jstring output_jstr) {
	const char* input_cstr = (*env)->GetStringUTFChars(env, input_jstr, NULL);
	const char* output_cstr = (*env)->GetStringUTFChars(env, output_jstr, NULL);

	//1.注册组件
	av_register_all();

	//封装格式文件
	AVFormatContext *pFormatCtx = avformat_alloc_context();

	//2.打开输入视频文件
	if (avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL) != 0) {
		LOGI("%s", "打开输入视频文件失败");
		return;
	}


	//3.获取文件视频信息
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		LOGE("%s", "获取视频信息失败");
		return;
	}


	//视频解码，需要找到视频对应的AVStream所在pFormatCtx->streams的索引位置
	int video_stream_idx = -1;
	int i = 0;
	for (; i < pFormatCtx->nb_streams; i++) {
		//根据类型判断，是否是视频流
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_idx = i;
			break;
		}
	}
	//4.获取解码器
	AVCodecContext *pCodecCtx = pFormatCtx->streams[video_stream_idx]->codec;
	AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		LOGE("%s", "找不到解码器");
		return;
	}
	//5.打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		LOGE("%s", "解码器无法打开");
		return;
	};
		//输出视频信息
    	LOGE("视频的文件格式：%s", pFormatCtx->iformat->name);
    	LOGE("视频时长：%lld", (pFormatCtx->duration) / 1000000);
    	LOGE("视频的宽高：%d,%d", pCodecCtx->width, pCodecCtx->height);
    	LOGE("解码器的名称：%s", pCodec->name);

	//编码数据
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(packet);
	//像素数据(解码数据)
	AVFrame *frame = av_frame_alloc();
	AVFrame *yuvFrame = av_frame_alloc();


	//只有指定了AVFrame的像素格式、画面大小才能真正分配内存
	//缓冲区分配内存
	uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	//初始化缓冲区
	avpicture_fill((AVPicture *)yuvFrame, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

	//输出文件
	FILE* fp_yuv = fopen(output_cstr, "wb");

	//用于像素格式转换或者缩放
	struct SwsContext  *sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);

	int len, got_frame, framecount = 0;

	//6.一帧一帧读取压缩的视频数据AVPacket
	while (av_read_frame(pFormatCtx, packet) >= 0) {

		//AVPacket->AVFrame
		len = avcodec_decode_video2(pCodecCtx, frame, &got_frame, packet);

		//Zero if no frame cloud be decompressed
		//非零，正在解码
		if (got_frame) {
			//AVFrame转为像素格式YUV420，宽高
			//2 6输入、输出数据
			//3 7输入、输出画面一行的数据的大小 AVFrame 转换是一行一行转换的
			//4 输入数据第一列要转码的位置 从0开始
			//5 输入画面的高度
			sws_scale(sws_ctx, frame->data, frame->linesize, 0,
				frame->height, yuvFrame->data, yuvFrame->linesize);

			//输出到YUV文件
			//AVFrame像素帧写入文件
			//data解码后的图像像素数据（音频采样数据）
			//Y 亮度 UV 色度（压缩了） 人对亮度更加敏感
			//U V 个数是Y的1/4
			int y_size = pCodecCtx->width * pCodecCtx->height;

			fwrite(yuvFrame->data[0], 1, y_size, fp_yuv);
			fwrite(yuvFrame->data[1], 1, y_size / 4, fp_yuv);
			fwrite(yuvFrame->data[2], 1, y_size / 4, fp_yuv);

			LOGI("解码%d帧", framecount++);
		}
		av_free_packet(packet);
	}
	fclose(fp_yuv);
	av_frame_free(&frame);
	avcodec_close(pCodecCtx);
	avformat_free_context(pFormatCtx);


	(*env)->ReleaseStringUTFChars(env, input_jstr, input_cstr);
	(*env)->ReleaseStringUTFChars(env, output_jstr, output_cstr);
}