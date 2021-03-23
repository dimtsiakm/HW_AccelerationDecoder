#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "SDL.h"
#include "SDL_image.h"
}

#include "SDLFunctionality.h"
#include <wtypes.h>
#include <chrono>
#include <thread>

#include <ppltasks.h>
#include <iostream>
#include "C:/Program Files (x86)/Windows Kits/10/Include/10.0.18362.0/um/libloaderapi.h"

using namespace concurrency;
using namespace std;

#undef main

//also, check -> https://github.com/leixiaohua1020/simplest_ffmpeg_streamer/blob/master/simplest_ffmpeg_receiver/simplest_ffmpeg_receiver.cpp

typedef HRESULT(CALLBACK* LPFNDLLFUNC1)(int, int);
typedef svideoparams* (CALLBACK* LPFNDLLINIT)(const char*);
typedef sdecoded_frame* (CALLBACK* LPFNDLLLIVESTREAMING)(void);
typedef int(CALLBACK* LPFNDLLCLOSE)(void);



int wmain(int argc, char** argv) {
    printf("START PROGRAM __");
    SDLFunctionality* sdlf = new SDLFunctionality();

    HINSTANCE hDLL;               // Handle to DLL
    LPFNDLLFUNC1 lpfnDllFunc1;    // Function pointer
    LPFNDLLINIT lpfnDllInit;
    LPFNDLLLIVESTREAMING lpfnDllLiveStreaming;
    LPFNDLLCLOSE lpfnDllClose;

    HRESULT hrReturnVal;
    svideoparams* hrReturnInit;
    sdecoded_frame* hrReturnLiveStreaming;

    hDLL = LoadLibrary(L"RTStreaming.dll");

    lpfnDllInit = (LPFNDLLINIT)GetProcAddress(hDLL, "init");
    lpfnDllLiveStreaming = (LPFNDLLLIVESTREAMING)GetProcAddress(hDLL, "live_streaming");
    lpfnDllClose = (LPFNDLLCLOSE)GetProcAddress(hDLL, "close");


    //hrReturnVal = lpfnDllFunc1(3, 3);
    const char* filename = "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov";


    hrReturnInit = lpfnDllInit(filename);
    std::cout << "value returned : " << hrReturnInit->width << ", " << std::endl;
    //hrReturnLiveStreaming = lpfnDllLiveStreaming();
    int i = 0;
    sdlf->Init();
    while (true) {
        i++;
        sdecoded_frame* frame = lpfnDllLiveStreaming();
        sdlf->ShowImage(frame);
        if (i > 180) {
            break;
        }
    }
    int k = lpfnDllClose();
    printf("close result : %d\n", k);
    //sdlf->Quit();

int main()
{
    std::cout << "Hello World!\n";
}
