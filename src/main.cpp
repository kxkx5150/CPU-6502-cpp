#define SDL_MAIN_HANDLED
#include "PC.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <time.h>

const int width = 560, height = 384;

void UpdateTexture(SDL_Texture *texture, uint32_t *imgdata)
{
    size_t     imgidx = 0;
    SDL_Color *color;
    Uint8     *src;
    Uint32    *dst;
    int        row, col;
    void      *pixels;
    int        pitch;
    SDL_LockTexture(texture, NULL, &pixels, &pitch);

    for (row = 0; row < height; ++row) {
        dst = (Uint32 *)((Uint8 *)pixels + row * pitch);
        for (col = 0; col < width; ++col) {
            *dst++ = imgdata[imgidx++];
        }
    }
    SDL_UnlockTexture(texture);
}
int main(int ArgCount, char **Args)
{
    int Running = 1;
    PC *pc      = new PC();
    pc->init();

    SDL_Window *window =
        SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width * 1, height * 1, SDL_WINDOW_OPENGL);
    SDL_Texture  *MooseTexture;
    SDL_Renderer *render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(render, 1, 1);
    MooseTexture = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    pc->load_prg("rom/starblazer.bin");
    pc->start();

    while (Running) {
        pc->tick();
        if (pc->cpu->get_img_status()) {
            Uint64 start   = SDL_GetPerformanceCounter();
            auto   imgdata = pc->cpu->get_img_data();
            pc->cpu->clear_img();
            UpdateTexture(MooseTexture, imgdata);
            SDL_RenderClear(render);
            SDL_RenderCopy(render, MooseTexture, NULL, NULL);
            SDL_RenderPresent(render);

            Uint64 end       = SDL_GetPerformanceCounter();
            float  elapsedMS = (end - start) / (float)SDL_GetPerformanceFrequency() * 1000.0f;
            if (16.666f > elapsedMS)
                SDL_Delay(floor(16.666f - elapsedMS));
        }

        SDL_Event Event;
        while (SDL_PollEvent(&Event)) {
            if (Event.type == SDL_QUIT)
                Running = 0;
        }
    }
    delete pc;
    return 0;
}
