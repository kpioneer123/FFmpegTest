#include <jni.h>
#include <android/log.h>

#define LOG_I(...) __android_log_print(ANDROID_LOG_ERROR , "main", __VA_ARGS__)

#include "SDL.h"
#include "SDL_log.h"
#include "SDL_main.h"


/*
//NDK代码
int main(int argc, char *argv[]) {
	//测试是否通过
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) == -1) {
    		LOG_I("SDL_Init failed %s", SDL_GetError());
        return 0;
    }

    LOG_I("SDL_Init Success!");

    SDL_Quit();
    return 0;
}
*/

//案例二：实现SDL播放YUV视频像素文件
int main(int argc,char *argv[]){
     //第一步：初始化SDL系统
      if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) == -1) {
          LOG_I("SDL_Init failed %s", SDL_GetError());
          return -1;
      }

      LOG_I("SDL_Init Success!");

      //第二步：创建窗口SDL_Window
      int screen_width = 640;
      int screen_height = 352;
      //帧画面宽高
      int frame_width = screen_width;
      int frame_height = screen_height;
      //参数一：窗体名字
      //参数二：窗体X坐标
      //参数三：窗体Y坐标
      //参数四：窗体宽度
      //参数五：窗体高度
      //参数六：窗体状态(打开)
      SDL_Window* sdl_window = SDL_CreateWindow("SDL Window"
              ,SDL_WINDOWPOS_CENTERED
              ,SDL_WINDOWPOS_CENTERED
              ,screen_width
              ,screen_height
              ,SDL_WINDOW_OPENGL);
      if (sdl_window == NULL){
          LOG_I("SDL_Window创建失败");
          return -1;
      }

      //第三步：创建渲染器SDL_Renderer
      //参数一：渲染到那个窗体上
      //参数二：从那个位置开始渲染
      //参数三：类型
      //SDL_RENDERER_SOFTWARE:软件渲染
      //SDL_RENDERER_ACCELERATED:硬件加速
      //SDL_RENDERER_PRESENTVSYNC:同步刷新
      //SDL_RENDERER_TARGETTEXTURE:纹理
      SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window,-1,0);

      //第四步：创建纹理SDL_Texture
      //参数一：渲染器
      //参数二：渲染格式(YUV格式:SDL_PIXELFORMAT_IYUV支持YUV420P)
      //参数三：绘制方式(SDL_TextureAccess)
      //SDL_TEXTUREACCESS_STATIC: 绘制极少
      //SDL_TEXTUREACCESS_STREAMING: 绘制频繁
      //SDL_TEXTUREACCESS_TARGET
      //参数四：纹理宽度
      //参数五：纹理高度
      SDL_Texture* sdl_texture = SDL_CreateTexture(sdl_renderer
              ,SDL_PIXELFORMAT_IYUV
              ,SDL_TEXTUREACCESS_STREAMING
              ,screen_width
              ,screen_height);


      //第五步：设置纹理的数据(SDL_UpdateTexture)
      //1.首先打开播放的YUV文件
      //不断更新数据（循环读取每一帧YUV视频像素数据，然后设置到纹理上，最终进行绘制）
      //打开YUV文件
      FILE* file_yuv = fopen("/storage/emulated/0/FFmpeg-Test/Test.yuv","rb+");
      if(file_yuv == NULL){
          LOG_I("打开文件失败!");
          return -1;
      }
      //2.循环读取YUV文件视频像素数据每一帧画面，进行渲染
      SDL_Rect sdl_rect;
      //Y:U:V = 4:1:1 假设：Y = 1.0  U = 0.25  V = 0.25  宽度：Y + U + V = 1.5
      //Y + U +V  = width * height * 1.5
      char buffer_pixels[screen_width * screen_height * 3 / 2];
      while (true){
          //一行一行的读取数据
          fread(buffer_pixels,1,screen_width * screen_height * 3 / 2,file_yuv);
          //判断文件是否读取完毕(视频是否播放完成)
          if(feof(file_yuv)){
              break;
          }
          //设置纹理的数据
          //参数一：目标纹理
          //参数二：渲染矩形区域(NULL:使用默认值-原始帧画面的宽高)
          //参数三：视频像素数据
          //参数四：帧画面宽
          SDL_UpdateTexture(sdl_texture,NULL,buffer_pixels,frame_width);

          //第六步：将纹理的数据拷贝给渲染器(SDL_RenderCopy)
          sdl_rect.x = 0;
          sdl_rect.y = 0;
          sdl_rect.w = screen_width;
          sdl_rect.h = screen_height;
          //先情况帧画面，再重新绘制
          SDL_RenderClear(sdl_renderer);
          SDL_RenderCopy(sdl_renderer,sdl_texture,NULL,&sdl_rect);

          //第七步：显示帧画面SDL_RenderPresent
          SDL_RenderPresent(sdl_renderer);

          //第八步：延时渲染(gif图片播放：播放间隔时间)
          //SDL_Delay():工具函数，用于延时
          SDL_Delay(20);
      }

      //第九步：释放资源
      fclose(file_yuv);
      SDL_DestroyTexture(sdl_texture);
      SDL_DestroyRenderer(sdl_renderer);

      //第十步：退出SDL系统SDL_Quit
      SDL_Quit();


}