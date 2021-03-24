#pragma once
#include <iostream>
#include "SDL.h"
#include "SDL_image.h"

struct buffer_data {
	uint8_t* ptr;
	size_t size; ///< size left in the buffer
};
struct s_dimension {
	int width;
	int height;
};
struct s_decoded_frame {
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
		int ShowImage(s_decoded_frame* s);
		int Quit();
};
