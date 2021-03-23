#pragma once
#include <iostream>
#include "SDL.h"
#include "SDL_image.h"

struct svideoparams {
	int width;
	int height;
	double frame_rate;
};
struct sdecoded_frame {
	uint8_t* data; //data as rgb or yuv format; set the dependencies
	int linesize;
	int width;
	int height;
};
class SDLFunctionality
{
	private:
		SDL_Window* win;
		SDL_Renderer* ren;
		SDL_Surface* bmp;
		SDL_Texture* tex;
	public:
		SDLFunctionality();
		int Init();
		int ShowImage(sdecoded_frame* s);
		int Quit();
};
