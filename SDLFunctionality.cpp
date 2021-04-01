#include "SDLFunctionality.h"

using std::cerr;
using std::endl;
SDLFunctionality::SDLFunctionality() {

}

int SDLFunctionality::Init() {
    using std::cerr;
    using std::endl;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        cerr << "SDL_Init Error: " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    win = SDL_CreateWindow("Hello World!", 100, 100, 620, 387, SDL_WINDOW_SHOWN);
    if (win == nullptr) {
        cerr << "SDL_CreateWindow Error: " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr) {
        cerr << "SDL_CreateRenderer Error" << SDL_GetError() << endl;
        if (win != nullptr) {
            SDL_DestroyWindow(win);
        }
        SDL_Quit();
        return EXIT_FAILURE;
    }
}
int SDLFunctionality::ShowImage(s_decoded_frame* s) {
    bmp = SDL_CreateRGBSurfaceFrom(s->data, s->width, s->height, 24, s->linesize, 0x0000FF, 0x00FF00, 0xFF0000, 0x000000);
    
    if (bmp == nullptr) {
        cerr << "SDL_LoadBMP Error: " << SDL_GetError() << endl;
        if (ren != nullptr) {
            SDL_DestroyRenderer(ren);
        }
        if (win != nullptr) {
            SDL_DestroyWindow(win);
        }
        SDL_Quit();
        return EXIT_FAILURE;
    }
    tex = SDL_CreateTextureFromSurface(ren, bmp);
    if (tex == nullptr) {
        cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << endl;
        if (bmp != nullptr) {
            SDL_FreeSurface(bmp);
        }
        if (ren != nullptr) {
            SDL_DestroyRenderer(ren);
        }
        if (win != nullptr) {
            SDL_DestroyWindow(win);
        }
        SDL_Quit();
        return EXIT_FAILURE;
    }
    SDL_FreeSurface(bmp);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    SDL_RenderPresent(ren);
    //system("pause");
    return EXIT_SUCCESS;
}
int SDLFunctionality::Quit() {
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return EXIT_SUCCESS;
}
