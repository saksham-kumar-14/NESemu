#include <iostream>
#include <string>
#include <SDL2/SDL.h>
#include "bus.h"
#include "cpu.h"
#include "ppu.h"
#include "cartridge.h"

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow(
        "NES Emulator - Pattern Table Debug",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        256 * 3, 240 * 3, SDL_WINDOW_SHOWN
    );
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        256, 240
    );

    Bus bus;
    PPU ppu;

    std::string rom_path = "DonkeyKong.nes";
    Cartridge cart(rom_path);

    if (!cart.romLoad) {
        std::cerr << "Failed to load nestest.nes! Check your ROM_DIR path.\n";
        return 1;
    }

    bus.ConnectCartridge(&cart);
    ppu.ConnectCPU(&bus.cpu);
    ppu.ConnectCartridge(&cart);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        ppu.drawDebugPatterntable(0);
        ppu.frame_complete = true;
        if (ppu.frame_complete) {
            SDL_UpdateTexture(texture, nullptr, ppu.screen_pixels.data(), 256 * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);

            ppu.frame_complete = false;
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
