#include <com_haocai_ffmpegtest_util_VideoPlayer.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
//解码
#include "include/libavcodec/avcodec.h"
//封装格式处理
#include "include/libavformat/avformat.h"
//像素处理
#include "include/libswscale/swscale.h"

#include "include/libavutil/imgutils.h"
#include "include/libavutil/time.h"
//重采样
#include "include/libswresample/swresample.h"

#include "queue.h"

#define  LOG_TAG    "ffmpegandroidplayer"
#define  LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,FORMAT,##__VA_ARGS__);
#define  LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,FORMAT,##__VA_ARGS__);
#define  LOGD(FORMAT,...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,FORMAT, ##__VA_ARGS__)

//因为现在只有音频和视频数据，如果还有字幕的话，MAX_STREAM则为3
#define MAX_STREAM 2
//音频解码 采样率 新版版可达48000 * 4
#define MAX_AUDIO_FRME_SIZE  2 * 44100

#define PACKET_QUEUE_SIZE 50

typedef struct _Player Player;
typedef struct _DecoderData DecoderData;


/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

double audioClock;

/**
* 便于对视频流中不同数据（音频与视频播放 方式不同），进行针对处理
*/
struct _Player {
	JavaVM *javaVM;
	//封装格式上下文
	AVFormatContext *input_format_ctx;
	//音频视频流索引位置
	int video_stream_index;
	int audio_stream_index;
	//流的总个数
	int capture_streams_no;

	//解码器上下文数组
	AVCodecContext *input_codec_ctx[MAX_STREAM];
	//解码线程ID
	pthread_t decode_threads[MAX_STREAM];
	//视图显示窗口
	ANativeWindow* nativeWindow;

	struct SwsContext *sws_ctx;
	SwrContext *swr_ctx;
	//输入的采样格式
	enum AVSampleFormat in_sample_fmt;
	//输出采样格式16bit PCM
	enum AVSampleFormat out_sample_fmt;
	//输入采样率
	int in_sample_rate;
	//输出采样率
	int out_sample_rate;
	//输出的声道个数
	int out_channel_nb;

	//JNI
	jobject audio_track;
	jmethodID audio_track_write_mid;

	pthread_t thread_read_from_stream;
	//音频、视频对列数组
	Queue *packets[MAX_STREAM];

    //互斥锁
	pthread_mutex_t mutex;
	//条件变量
	pthread_cond_t cond;

	//视频开始播放的时间
	int64_t start_time;

	int64_t audio_clock;
};
//解码数据
struct _DecoderData {
	Player *player;
	int stream_index;
};

/**
* 初始化封装格式初始化
*/
void init_input_format_ctx(const char* input_cstr, Player *player) {

	LOGD("play");
	av_register_all();

	AVFormatContext *format_ctx = avformat_alloc_context();

	// Open video file
	if (avformat_open_input(&format_ctx, input_cstr, NULL, NULL) != 0) {

		LOGD("Couldn't open file:%s\n", input_cstr);
		return; // Couldn't open file
	}

	// Retrieve stream information
	if (avformat_find_stream_info(format_ctx, NULL) < 0) {
		LOGD("Couldn't find stream information.");
		return;
	}

	player->capture_streams_no = format_ctx->nb_streams;
	LOGI("captrue_streams_no:%d", player->capture_streams_no);
	// Find the first video stream
	//获取音频和视频流的不同索引位置
	int i;
	for (i = 0; i < format_ctx->nb_streams; i++) {
		if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			player->video_stream_index = i;
		}
		else if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			player->audio_stream_index = i;
		}

	}
	player->input_format_ctx = format_ctx;

}
void init_codec_context(Player *player, int stream_idx) {

	AVFormatContext *format_ctx = player->input_format_ctx;
	//获取解码器
	LOGI("init_codec_context begin");
	AVCodecContext *codec_ctx = format_ctx->streams[stream_idx]->codec;


	// Find the decoder for the video stream
	AVCodec *pCodec = avcodec_find_decoder(codec_ctx->codec_id);
	if (pCodec == NULL) {
		LOGD("Codec not found.");
		return; // Codec not found
	}
	if (avcodec_open2(codec_ctx, pCodec, NULL) < 0) {
		LOGD("Could not open codec.");
		return; // Could not open codec
	}
	player->input_codec_ctx[stream_idx] = codec_ctx;
}

//视频解码准备
void decode_video_prepare(JNIEnv *env, Player *player, jobject surface) {
	player->nativeWindow = ANativeWindow_fromSurface(env, surface);

	AVCodecContext *codec_ctx = player->input_codec_ctx[player->video_stream_index];
	int videoWidth = codec_ctx->width;
	int videoHeight = codec_ctx->height;
	// 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
	player->sws_ctx = sws_getContext(codec_ctx->width,
		codec_ctx->height,
		codec_ctx->pix_fmt,
		codec_ctx->width,
		codec_ctx->height,
		AV_PIX_FMT_RGBA,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL);

	//ANativeWindow_setBuffersGeometry(player->nativeWindow, videoWidth, videoHeight, WINDOW_FORMAT_RGBA_8888);

}



//音频解码准备
void decode_audio_prepare(Player *player) {

	AVCodecContext *codec_ctx = player->input_codec_ctx[player->audio_stream_index];
	//frame->16bit  44100 PCM 统一音频采样格式与采样率
	SwrContext *swr_ctx = swr_alloc();
	//输入采样率格式
	enum AVSampleFormat in_sample_fmt = codec_ctx->sample_fmt;
	//输出采样率格式16bit PCM
	enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	//输入采样率
	int in_sample_rate = codec_ctx->sample_rate;
	//输出采样率
	int out_sample_rate = 44100;
	//获取输入的声道布局
	//根据声道个数获取默认的声道布局(2个声道，默认立体声)
	//av_get_default_channel_layout(pCodeCtx->channels);
	uint64_t in_ch_layout = codec_ctx->channel_layout;
	//输出的声道布局
	uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;


	swr_alloc_set_opts(swr_ctx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate, 0, NULL);


	swr_init(swr_ctx);

	//获取输入输出的声道个数
	int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
	LOGI("out_count:%d", out_channel_nb);

	//重采样设置参数-------------end

	player->in_sample_fmt = in_sample_fmt;
	player->out_sample_fmt = out_sample_fmt;
	player->in_sample_rate = in_sample_rate;
	player->out_sample_rate = out_sample_rate;
	player->out_channel_nb = out_channel_nb;
	player->swr_ctx = swr_ctx;


}


void jni_audio_prepare(JNIEnv *env, jobject jthiz, Player *player) {
	//JNI begin------------------
	jclass cls = (*env)->FindClass(env, "com/haocai/ffmpegtest/util/AudioUtil");
	//jmethodID
	jmethodID  constructor_mid = (*env)->GetMethodID(env, cls, "<init>", "()V");

	//实例化一个AudioUtil对象(可以在constructor_mid后加参)
	jobject audioutil_obj = (*env)->NewObject(env, cls, constructor_mid);   //类似于AudioUtil audioutil =new AudioUtil();

																			//AudioTrack对象
	jmethodID create_audio_track_mid = (*env)->GetMethodID(env, cls, "createAudioTrack", "(II)Landroid/media/AudioTrack;");
	jobject audio_track = (*env)->CallObjectMethod(env, audioutil_obj, create_audio_track_mid, player->out_sample_rate, player->out_channel_nb);

	//调用AudioTrack.play方法
	jclass audio_track_class = (*env)->GetObjectClass(env, audio_track);
	jmethodID audio_track_play_mid = (*env)->GetMethodID(env, audio_track_class, "play", "()V");
	(*env)->CallVoidMethod(env, audio_track, audio_track_play_mid);

	//AudioTrack.write
	jmethodID audio_track_write_mid = (*env)->GetMethodID(env, audio_track_class, "write", "([BII)I");
	//JNI end------------------

	//audio_track变成全局引用，否则在子线程中会报错
	player->audio_track = (*env)->NewGlobalRef(env, audio_track);
	//(*env)->DeleteGlobalRef
	player->audio_track_write_mid = audio_track_write_mid;
}

/**
 * 获取视频当前播放时间
 */
int64_t player_get_current_video_time(Player *player) {
	int64_t current_time = av_gettime();
	return current_time - player->start_time;
}






/**
* 解码视频
*/
void decode_video(Player *player, AVPacket *packet,uint8_t *buffer) {

	AVFormatContext *input_format_ctx = player->input_format_ctx;
	AVStream *stream = input_format_ctx->streams[player->video_stream_index];
	//像素数据（解码数据）
	// Allocate video frame
	AVFrame *pFrame = av_frame_alloc();
	// 用于渲染
	AVFrame *pFrameRGBA = av_frame_alloc();
	//绘制时的缓冲区
	ANativeWindow_Buffer windowBuffer;
	AVCodecContext *codec_ctx = player->input_codec_ctx[player->video_stream_index];
	int videoWidth = codec_ctx->width;
	int videoHeight = codec_ctx->height;
	int got_frame;
	//解码AVPacket->AVFrame
	avcodec_decode_video2(codec_ctx, pFrame, &got_frame, packet);



	//Zero if no frame could be decompressed
	//非零，正在解码
	if (got_frame) {
		//lock
		//设置缓冲区的属性（宽、高、像素格式）
		ANativeWindow_setBuffersGeometry(player->nativeWindow, codec_ctx->width, codec_ctx->height, WINDOW_FORMAT_RGBA_8888);
		ANativeWindow_lock(player->nativeWindow, &windowBuffer, NULL);

		//设置rgb_frame的属性（像素格式、宽高）和缓冲区
		//rgb_frame缓冲区与outBuffer.bits是同一块内存

		av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, buffer, AV_PIX_FMT_RGBA,
			videoWidth,videoHeight, 1);

		// 格式转换
		sws_scale(player->sws_ctx, (uint8_t const *const *)pFrame->data,
			pFrame->linesize, 0, videoHeight,
			pFrameRGBA->data, pFrameRGBA->linesize);

		// 获取stride
		uint8_t *dst = (uint8_t *)windowBuffer.bits;
		int dstStride = windowBuffer.stride * 4;
		uint8_t *src = (pFrameRGBA->data[0]);
		int srcStride = pFrameRGBA->linesize[0];

		// 由于window的stride和帧的stride不同,因此需要逐行复制
		int h;
		for (h = 0; h < videoHeight; h++) {
			memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
		}


  double timestamp;
    if(packet->pts == AV_NOPTS_VALUE) {
        timestamp = 0;
    } else {
        timestamp = av_frame_get_best_effort_timestamp(pFrame)*av_q2d(stream->time_base);
    }
    double frameRate = av_q2d(stream->avg_frame_rate);
    frameRate += pFrame->repeat_pict * (frameRate * 0.5);
    if (timestamp == 0.0) {
        usleep((unsigned long)(frameRate*1000));
    }else {
        if (fabs(timestamp - audioClock) > AV_SYNC_THRESHOLD_MIN &&
                fabs(timestamp - audioClock) < AV_NOSYNC_THRESHOLD) {
            if (timestamp > audioClock) {
                usleep((unsigned long)((timestamp - audioClock)*1000000));
            }
        }
    }



		//unlock
		ANativeWindow_unlockAndPost(player->nativeWindow);

	}

	av_frame_free(&pFrame);
	av_frame_free(&pFrameRGBA);


}
/**
* 音频解码准备
*/

void decode_audio(Player *player, AVPacket *packet,uint8_t *buffer) {

	AVFormatContext *input_format_ctx = player->input_format_ctx;
	AVStream *stream = input_format_ctx->streams[player->video_stream_index];
	AVCodecContext *codec_ctx = player->input_codec_ctx[player->audio_stream_index];
	LOGI("%s", "decode_audio");
	//解压缩数据
	AVFrame *frame = av_frame_alloc();
	int got_frame;
	avcodec_decode_audio4(codec_ctx, frame, &got_frame, packet);
	//16bit 44100 PCM 数据（重采样缓冲区）

	//非0，正在解码
	if (got_frame > 0) {
		swr_convert(player->swr_ctx, &buffer, MAX_AUDIO_FRME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);
		//获取sample的size
		int out_buffer_size ;

    if (player->out_sample_fmt == AV_SAMPLE_FMT_S16P) {
       out_buffer_size = av_samples_get_buffer_size(frame->linesize, player->out_channel_nb, frame->nb_samples, player->out_sample_fmt, 1);
    }else {
       av_samples_get_buffer_size(&out_buffer_size, player->out_channel_nb, frame->nb_samples, player->out_sample_fmt, 1);
    }


        audioClock = frame->pkt_pts * av_q2d(stream->time_base);

		//关联当前线程的JNIEnv
		JavaVM *javaVM = player->javaVM;
		JNIEnv *env;
		(*javaVM)->AttachCurrentThread(javaVM, &env, NULL);
		//out_buffer 缓冲区数据，转换成byte数组
		jbyteArray audio_sample_array = (*env)->NewByteArray(env, out_buffer_size);

		jbyte* sample_byte = (*env)->GetByteArrayElements(env, audio_sample_array, NULL);

		//将out_buffer的数据复制到sample_byte
		memcpy(sample_byte, buffer, out_buffer_size);

		//同步数据 同时释放sample_byte
		(*env)->ReleaseByteArrayElements(env, audio_sample_array, sample_byte, 0);

		//AudioTrack.write PCM数据
		(*env)->CallIntMethod(env, player->audio_track, player->audio_track_write_mid, audio_sample_array, 0, out_buffer_size);

		//释放局部引用  否则报错JNI ERROR (app bug): local reference table overflow (max=512)
		(*env)->DeleteLocalRef(env, audio_sample_array);
		(*javaVM)->DetachCurrentThread(javaVM);

	}

	av_frame_free(&frame);
	//av_free(&out_buffer);


}
/**
* 解码子线程函数
*/
void* decode_data(void* arg) {
	DecoderData *decoder_data = (DecoderData*)arg;
	Player *player = decoder_data->player;
	int stream_index = decoder_data->stream_index;
	LOGI("queue:%d", stream_index)
	//根据stream_index获取对应的AVPacket队列
	Queue *queue = player->packets[stream_index];

	AVFormatContext *format_ctx = player->input_format_ctx;
	int video_frame_count = 0, audio_frame_count = 0;

         uint8_t *buffer;
    if (stream_index == player->video_stream_index) {
	    AVCodecContext *codec_ctx = player->input_codec_ctx[player->video_stream_index];
	    int videoWidth = codec_ctx->width;
	    int videoHeight = codec_ctx->height;
	     // buffer中数据就是用于渲染的,且格式为RGBA
	    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA,videoWidth, videoHeight,1);
        buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    	}
    	else if (stream_index == player->audio_stream_index) {
            buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);
    	}


	while(true) {

         pthread_mutex_lock(&player->mutex);
	   	AVPacket packet = QueuePop(queue,&player->mutex,&player->cond,0);

		if (packet.stream_index == player->video_stream_index) {
			decode_video(player,  &packet,buffer);
			LOGI("video_frame_count:%d", video_frame_count++);
		}
		else if (packet.stream_index == player->audio_stream_index) {
		   decode_audio(player, &packet,buffer);
		   LOGI("audio_frame_count:%d", audio_frame_count++);
		}
		    pthread_mutex_unlock(&player->mutex);
		    LOGI("pthread_mutex_unlock  AVPacket");
	  }
	av_free(buffer);
return 0;

}

/**
* 给AVPacket开辟空间，后面会将AVPacket栈内存数据拷贝至这里开辟的空间
*/
void* player_fill_packet() {
	//请参照我在vs中写的代码
	AVPacket *packet = malloc(sizeof(AVPacket));
	return packet;
}

/*
*初始化音频，视频AVPacket队列,长度15左右
*/
void player_alloc_queues(Player *player) {
	int i;
	//这里，正常是初始化两个队列
	for (i = 0; i < player->capture_streams_no; i++) {
		Queue *queue = (Queue *)CreateQueue();
		player->packets[i] = queue;
		//打印视频音频队列地址
		LOGI("stream index:%d,queue:%#x", i, (unsigned int)queue);

	}
}
/*
*生产者线程: read_stream线程负责不断的读取视频文件中AVPacket，分别放入两个队列中
*/
void* player_read_from_stream(void* arg) {
	int index = 0;
	LOGI("player_read_from_stream start");
	Player *player = (Player*)arg;
	int ret;
	//栈内存上保存一个AVPacket
	AVPacket packet;

	while(true){
		ret = av_read_frame(player->input_format_ctx, &packet);
		LOGI("player_read_from_stream :%d", index++);
		//到文件结尾
		if (ret < 0) {
			break;
		}


		if(packet.stream_index ==player->video_stream_index || packet.stream_index ==player->audio_stream_index){
			    pthread_mutex_lock(&player->mutex);
        	    QueuePush(player->packets[packet.stream_index],packet,&player->mutex,&player->cond,0);
        		pthread_mutex_unlock(&player->mutex);
		}

        LOGI("pthread_mutex_unlock  player_read_from_stream");
	}


	return 0;
}

void* packet_free_func(AVPacket *packet) {
	av_free(packet);
	return 0;
}

//音视频同步播放
//视频播放
JNIEXPORT void JNICALL Java_com_haocai_ffmpegtest_util_VideoPlayer_play
(JNIEnv *env, jobject jobj, jstring input_jstr, jobject surface) {
	const char* file_name = (*env)->GetStringUTFChars(env, input_jstr, NULL);
	Player *player = (Player*)malloc(sizeof(Player));
	(*env)->GetJavaVM(env, &(player->javaVM));
	//初始化封装格式上下文
	init_input_format_ctx(file_name, player);
	//视频索引
	int video_stream_index = player->video_stream_index;
	//音频索引
	int audio_stream_index = player->audio_stream_index;
	//获取音视频解码器，并打开
	init_codec_context(player, video_stream_index);
	init_codec_context(player, audio_stream_index);

	decode_video_prepare(env, player, surface);
	decode_audio_prepare(player);

	jni_audio_prepare(env, jobj, player);

	player_alloc_queues(player);

	pthread_mutex_init(&player->mutex,NULL);
	pthread_cond_init(&player->cond,NULL);
	//生产者线程
	pthread_create(&(player->thread_read_from_stream), NULL, player_read_from_stream, (void*)player);

	sleep(1);
    player->start_time = 0;

	DecoderData data1 = { player,video_stream_index }, *decoder_data1 = &data1;
	//消费者线程
	pthread_create(&(player->decode_threads[video_stream_index]), NULL, decode_data, (void*)decoder_data1);

	DecoderData data2 = { player,audio_stream_index }, *decoder_data2 = &data2;
	pthread_create(&(player->decode_threads[audio_stream_index]), NULL, decode_data, (void*)decoder_data2);

	pthread_join(player->thread_read_from_stream, NULL);
	pthread_join(player->decode_threads[video_stream_index], NULL);
	pthread_join(player->decode_threads[audio_stream_index], NULL);

    int i;
    for (i = 0; i < player->capture_streams_no; i++) {
         QueueFree(player->packets[i] );
         avcodec_close(player->input_codec_ctx[i]);
    }

    pthread_mutex_destroy(&player->mutex);
    pthread_cond_destroy(&player->cond);
	// Close the codecs

	// Close the video file
	avformat_close_input(&player->input_format_ctx);
	free(player);
	(*env)->ReleaseStringUTFChars(env, input_jstr, file_name);


	return;
}
