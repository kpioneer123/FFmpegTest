#include "jni.h"
#include "ffmpeg.h"


//视频转码压缩主函数入口
//SDL（main）
//ffmpeg_mod.c有一个FFmpeg视频转码主函数入口
//标记（声明有一个这样的函数提供给我调用）
//参数含义分析
//首先分析：String str = "ffmpeg -i input.mov -b:v 640k output.mp4"
// argc = str.split(" ").length()
// argv = str.split(" ")  字符串数组
//参数一：命令行字符串命令个数
//参数二：命令行字符串数组
int ffmpegmain(int argc, char **argv);


JNIEXPORT void JNICALL Java_com_haocai_ffmpegtest_util_VideoPlayer_transcodingCompress
        (JNIEnv *env, jobject jobj,jint jlen,jobjectArray jobjArray){
    //转码
    //将java的字符串数组转成C字符串
    int argc = jlen;
    //开辟内存空间
    char **argv = (char**)malloc(sizeof(char*) * argc);

    //填充内容
    for (int i = 0; i < argc; ++i) {
        jstring str = (*env)->GetObjectArrayElement(env,jobjArray,i);
        const char* tem = (*env)->GetStringUTFChars(env,str,0);
        argv[i] = (char*)malloc(sizeof(char)*1024);
        strcpy(argv[i],tem);
    }

    //开始转码(底层实现就是只需命令)
    ffmpegmain(argc,argv);

    //释放内存空间
    for (int i = 0; i < argc; ++i) {
        free(argv[i]);
    }

    //释放数组
    free(argv);

}


