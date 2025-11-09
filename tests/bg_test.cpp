#include "../src/ppu.h"
#include "../src/bus.h"
#include "../src/cartridge.h"
#include <SDL2/SDL.h>
#include <iostream>

int main() {
    Cartridge cart("DonkeyKong.nes");
    Bus bus;
    PPU ppu;

    bus.ConnectCartridge(&cart);
    bus.ConnectPPU(&ppu);
    ppu.ConnectBus(&bus);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow(
        "Nametable Renderer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT, 0
    );
    SDL_Surface* surface = SDL_GetWindowSurface(window);

    // Donkey Kong stores background layout in $2000
    ppu.RenderNametable(0x2000);
    ppu.UpdateSurface(surface);
    SDL_UpdateWindowSurface(window);

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = false;
        }
        SDL_Delay(16);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
}
