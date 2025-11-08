#include "ppu.h"
#include "bus.h"
#include "cartridge.h"
#include <SDL2/SDL.h>
#include <iostream>

int main() {
    Cartridge cart("DonkeyKong.nes");
    Bus bus;
    PPU ppu;

    if (cart.CHRMemory.empty()) {
        std::cout << "No CHR ROM found — using 8KB CHR RAM (graphics will be uploaded at runtime).\n";
    } else {
        std::cout << "Loaded CHR ROM: " << (cart.CHRMemory.size() / 1024) << " KB\n";
    }

    bus.ConnectCartridge(&cart);
    bus.ConnectPPU(&ppu);
    ppu.ConnectBus(&bus);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow(
        "Pattern Table Viewer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT, 0
    );

    SDL_Surface* surface = SDL_GetWindowSurface(window);

    ppu.RenderPatternTables();
    ppu.UpdateSurface(surface);
    SDL_UpdateWindowSurface(window);

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }
        SDL_Delay(16);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
}
