#include <com_haocai_ffmpegtest_util_VideoPlayer.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <stdio.h>
#include <unistd.h>
//解码
#include "include/libavcodec/avcodec.h"
//封装格式处理
#include "include/libavformat/avformat.h"
//像素处理
#include "include/libswscale/swscale.h"

#include "include/libavutil/imgutils.h"
#define  LOG_TAG    "ffmpegandroidplayer"
#define  LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,FORMAT,##__VA_ARGS__);
#define  LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,FORMAT,##__VA_ARGS__);
#define  LOGD(FORMAT,...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,FORMAT, ##__VA_ARGS__)

//解码
JNIEXPORT void JNICALL Java_com_haocai_ffmpegtest_util_VideoPlayer_decode
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
			sws_scale(sws_ctx,(const uint8_t *const *) frame->data, frame->linesize, 0,
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

//视频播放
JNIEXPORT void JNICALL Java_com_haocai_ffmpegtest_util_VideoPlayer_render
  (JNIEnv *env, jobject jobj, jstring input_jstr, jobject surface){
	const char* file_name = (*env)->GetStringUTFChars(env, input_jstr, NULL);


    LOGD("play");


    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    // Open video file
    if (avformat_open_input(&pFormatCtx, file_name, NULL, NULL) != 0) {

        LOGD("Couldn't open file:%s\n", file_name);
        return ; // Couldn't open file
    }

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGD("Couldn't find stream information.");
        return ;
    }

    // Find the first video stream
    int videoStream = -1, i;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
            && videoStream < 0) {
            videoStream = i;
        }
    }
    if (videoStream == -1) {
        LOGD("Didn't find a video stream.");
        return ; // Didn't find a video stream
    }

    // Get a pointer to the codec context for the video stream
    AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        LOGD("Codec not found.");
        return ; // Codec not found
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGD("Could not open codec.");
        return ; // Could not open codec
    }

    // 获取native window
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    // 获取视频宽高
    int videoWidth = pCodecCtx->width;
    int videoHeight = pCodecCtx->height;

    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight,
                                     WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGD("Could not open codec.");
        return ; // Could not open codec
    }

    // Allocate video frame
    AVFrame *pFrame = av_frame_alloc();

    // 用于渲染
    AVFrame *pFrameRGBA = av_frame_alloc();
    if (pFrameRGBA == NULL || pFrame == NULL) {
        LOGD("Could not allocate video frame.");
        return ;
    }

    // Determine required buffer size and allocate buffer
    // buffer中数据就是用于渲染的,且格式为RGBA
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height,
                                            1);
    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, buffer, AV_PIX_FMT_RGBA,
                         pCodecCtx->width, pCodecCtx->height, 1);

    // 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,
                                                pCodecCtx->height,
                                                pCodecCtx->pix_fmt,
                                                pCodecCtx->width,
                                                pCodecCtx->height,
                                                AV_PIX_FMT_RGBA,
                                                SWS_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);

    int frameFinished;
    AVPacket packet;
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream) {

            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            // 并不是decode一次就可解码出一帧
            if (frameFinished) {

                // lock native window buffer
                ANativeWindow_lock(nativeWindow, &windowBuffer, 0);

                // 格式转换
                sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height,
                          pFrameRGBA->data, pFrameRGBA->linesize);

                // 获取stride
                uint8_t *dst = (uint8_t *) windowBuffer.bits;
                int dstStride = windowBuffer.stride * 4;
                uint8_t *src = (pFrameRGBA->data[0]);
                int srcStride = pFrameRGBA->linesize[0];

                // 由于window的stride和帧的stride不同,因此需要逐行复制
                int h;
                for (h = 0; h < videoHeight; h++) {
                    memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
                }

                ANativeWindow_unlockAndPost(nativeWindow);
                //延时绘制 否则视频快速播放
                usleep(1000 * 16);
            }

        }
        av_packet_unref(&packet);
    }

    av_free(buffer);
    av_free(pFrameRGBA);

    // Free the YUV frame
    av_free(pFrame);

    // Close the codecs
    avcodec_close(pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);
    (*env)->ReleaseStringUTFChars(env, input_jstr, file_name);
    return ;
}
