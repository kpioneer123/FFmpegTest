#include <com_haocai_ffmpegtest_util_VideoPlayer.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <unistd.h>
#include <stdlib.h>
//解码
#include "include/libavcodec/avcodec.h"
//封装格式处理
#include "include/libavformat/avformat.h"
//像素处理
#include "include/libswscale/swscale.h"
//重采样
#include "include/libswresample/swresample.h"


#define  LOG_TAG    "ffmpegandroidplayer"
#define  LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,FORMAT,##__VA_ARGS__);
#define  LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,FORMAT,##__VA_ARGS__);
#define  LOGD(FORMAT,...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,FORMAT, ##__VA_ARGS__)

//音频解码 采样率 新版版可达48000 * 4
#define MAX_AUDIO_FRME_SIZE  2 * 44100

//音频解码
JNIEXPORT void JNICALL Java_com_haocai_ffmpegtest_util_VideoPlayer_audioDecode
(JNIEnv *env, jobject jobj, jstring input_jstr, jstring output_jstr) {
	const char* input_cstr = (*env)->GetStringUTFChars(env, input_jstr, NULL);
	const char* output_cstr = (*env)->GetStringUTFChars(env, output_jstr, NULL);
	LOGI("%s", "init");
	//注册组件
	av_register_all();
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	//打开音频文件
	if (avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL) != 0) {
		LOGI("%s", "无法打开音频文件");
		return;
	}
	//获取输入文件信息
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		LOGI("%s", "无法获取输入文件信息");
		return;
	}
	//获取音频流索引位置
	int i = 0, audio_stream_idx = -1;
	for (; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_idx = i;
			break;
		}
	}
	if (audio_stream_idx == -1)
	{
		LOGI("%s", "找不到音频流");
		return;
	}
	//获取解码器
	AVCodecContext *pCodeCtx = pFormatCtx->streams[audio_stream_idx]->codec;
	AVCodec *codec = avcodec_find_decoder(pCodeCtx->codec_id);
	if (codec == NULL) {
		LOGI("%s", "无法获取加码器");
		return;
	}
	//打开解码器
	if (avcodec_open2(pCodeCtx, codec, NULL) < 0) {
		LOGI("%s", "无法打开解码器");
		return;
	}

	//压缩数据
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	//解压缩数据
	AVFrame *frame = av_frame_alloc();
	//frame->16bit  44100 PCM 统一音频采样格式与采样率
	SwrContext *swrCtx = swr_alloc();
	//重采样设置参数--------------start
	//输入采样率格式
	enum AVSampleFormat in_sample_fmt = pCodeCtx->sample_fmt;
	//输出采样率格式16bit PCM
	enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	//输入采样率
	int in_sample_rate = pCodeCtx->sample_rate;
	//输出采样率
	int out_sample_rate = 44100;
	//获取输入的声道布局
	//根据声道个数获取默认的声道布局(2个声道，默认立体声)
	//av_get_default_channel_layout(pCodeCtx->channels);
	uint64_t in_ch_layout = pCodeCtx->channel_layout;
	//输出的声道布局
	uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;


	swr_alloc_set_opts(swrCtx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate, 0, NULL);


	swr_init(swrCtx);

	//获取输入输出的声道个数
	int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
	LOGI("out_count:%d", out_channel_nb);
	//重采样设置参数--------------end

	//16bit 44100 PCM 数据
	uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);

	FILE *fp_pcm = fopen(output_cstr, "wb");
	int got_frame = 0, framecount = 0, ret;
	//6.一帧一帧读取压缩的音频数据AVPacket
	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == audio_stream_idx) {
			//解码
			ret = avcodec_decode_audio4(pCodeCtx, frame, &got_frame, packet);

			if (ret < 0) {
				LOGI("%s", "解码完成");
				break;
			}
			//非0，正在解码
			if (got_frame > 0) {
				LOGI("解码：%d", framecount++);
				swr_convert(swrCtx, &out_buffer, MAX_AUDIO_FRME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);
				//获取sample的size
				int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb, frame->nb_samples, out_sample_fmt, 1);


				fwrite(out_buffer, 1, out_buffer_size, fp_pcm);

			}
		}
		av_free_packet(packet);
	}
	fclose(fp_pcm);
	av_frame_free(&frame);
	av_free(out_buffer);
	swr_free(&swrCtx);
	avcodec_close(pCodeCtx);
	avformat_close_input(&pFormatCtx);

	(*env)->ReleaseStringUTFChars(env, input_jstr, input_cstr);
	(*env)->ReleaseStringUTFChars(env, output_jstr, output_cstr);


}


//音频播放
JNIEXPORT void JNICALL Java_com_haocai_ffmpegtest_util_VideoPlayer_audioPlayer
(JNIEnv *env, jobject jobj, jstring input_jstr) {
	const char* input_cstr = (*env)->GetStringUTFChars(env, input_jstr, NULL);
	LOGI("%s", "init");
	//注册组件
	av_register_all();
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	//打开音频文件
	if (avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL) != 0) {
		LOGI("%s", "无法打开音频文件");
		return;
	}
	//获取输入文件信息
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		LOGI("%s", "无法获取输入文件信息");
		return;
	}
	//获取音频流索引位置
	int i = 0, audio_stream_idx = -1;
	for (; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_idx = i;
			break;
		}
	}
	if (audio_stream_idx == -1)
	{
		LOGI("%s", "找不到音频流");
		return;
	}
	//获取解码器
	AVCodecContext *pCodeCtx = pFormatCtx->streams[audio_stream_idx]->codec;
	AVCodec *codec = avcodec_find_decoder(pCodeCtx->codec_id);
	if (codec == NULL) {
		LOGI("%s", "无法获取加码器");
		return;
	}
	//打开解码器
	if (avcodec_open2(pCodeCtx, codec, NULL) < 0) {
		LOGI("%s", "无法打开解码器");
		return;
	}

	//压缩数据
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	//解压缩数据
	AVFrame *frame = av_frame_alloc();
	//frame->16bit  44100 PCM 统一音频采样格式与采样率
	SwrContext *swrCtx = swr_alloc();

	//输入采样率格式
	enum AVSampleFormat in_sample_fmt = pCodeCtx->sample_fmt;
	//输出采样率格式16bit PCM
	enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	//输入采样率
	int in_sample_rate = pCodeCtx->sample_rate;
	//输出采样率
	int out_sample_rate = 44100;
	//获取输入的声道布局
	//根据声道个数获取默认的声道布局(2个声道，默认立体声)
	//av_get_default_channel_layout(pCodeCtx->channels);
	uint64_t in_ch_layout = pCodeCtx->channel_layout;
	//输出的声道布局
	uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;


	swr_alloc_set_opts(swrCtx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate, 0, NULL);


	swr_init(swrCtx);

	//获取输入输出的声道个数
	int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
	LOGI("out_count:%d", out_channel_nb);

     //jclass  player_class = (*env)->GetObjectClass(env,jobj);

    jclass cls = (*env)->FindClass(env, "com/haocai/ffmpegtest/util/AudioUtil");
     //jmethodID
    jmethodID  constructor_mid= (*env)->GetMethodID(env, cls,"<init>","()V");

    //实例化一个AudioUtil对象(可以在constructor_mid后加参)
    jobject audioutil_obj =  (*env)->NewObject(env, cls, constructor_mid);   //类似于AudioUtil audioutil =new AudioUtil();

    //AudioTrack对象
    jmethodID create_audio_track_mid =(*env)->GetMethodID(env,cls,"createAudioTrack","(II)Landroid/media/AudioTrack;");
 	jobject audio_track = (*env)->CallObjectMethod(env,audioutil_obj,create_audio_track_mid,out_sample_rate,out_channel_nb);

 	//调用AudioTrack.play方法
 	jclass audio_track_class = (*env)->GetObjectClass(env,audio_track);
 	jmethodID audio_track_play_mid = (*env)->GetMethodID(env,audio_track_class,"play","()V");
 	(*env)->CallVoidMethod(env,audio_track,audio_track_play_mid);

    //AudioTrack.write
    jmethodID audio_track_write_mid =  (*env)->GetMethodID(env,audio_track_class,"write","([BII)I");

	//16bit 44100 PCM 数据
	uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);

	int got_frame = 0, framecount = 0, ret;
	//6.一帧一帧读取压缩的音频数据AVPacket
	while (av_read_frame(pFormatCtx, packet) >= 0) {
		//解码音频类型的Packet，  packet 有可能是音频数据流 有可能是视频数据流
		if (packet->stream_index == audio_stream_idx) {
			//解码
			ret = avcodec_decode_audio4(pCodeCtx, frame, &got_frame, packet);

			if (ret < 0) {
				LOGI("%s", "解码完成");
				break;
			}
			//非0，正在解码
			if (got_frame > 0) {
				LOGI("解码：%d", framecount++);
				swr_convert(swrCtx, &out_buffer, MAX_AUDIO_FRME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);
				//获取sample的size
				int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb, frame->nb_samples, out_sample_fmt, 1);
                //out_buffer 缓冲区数据，转换成byte数组
                jbyteArray audio_sample_array =  (*env)->NewByteArray(env,out_buffer_size);

                jbyte* sample_byte = (*env)->GetByteArrayElements(env,audio_sample_array,NULL);

                 //将out_buffer的数据复制到sample_byte
                 memcpy(sample_byte,out_buffer,out_buffer_size);

                 //同步数据 同时释放sample_byte
                 (*env)->ReleaseByteArrayElements(env,audio_sample_array,sample_byte,0);

                //AudioTrack.write PCM数据
                 (*env)->CallIntMethod(env,audio_track,audio_track_write_mid,audio_sample_array,0,out_buffer_size);

                 //释放局部引用  否则报错JNI ERROR (app bug): local reference table overflow (max=512)
                 (*env)->DeleteLocalRef(env,audio_sample_array);
                usleep(1000 * 16); //16ms
			}
		}
		av_free_packet(packet);
	}
	av_frame_free(&frame);
	av_free(out_buffer);
	swr_free(&swrCtx);
	avcodec_close(pCodeCtx);
	avformat_close_input(&pFormatCtx);

	(*env)->ReleaseStringUTFChars(env, input_jstr, input_cstr);


}