#include "../src/bus.h"
#include "../src/ppu.h"
#include "cartridge.h"

int main() {
    Cartridge cart("HelloWorld.nes");
    Bus bus;
    PPU ppu;

    bus.ConnectCartridge(&cart);
    bus.ConnectPPU(&ppu);
    ppu.ConnectBus(&bus);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow(
        "NES PPU Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT, 0);

    SDL_Surface* surface = SDL_GetWindowSurface(window);
    ppu.RenderFrame();
    ppu.UpdateSurface(surface);
    SDL_UpdateWindowSurface(window);

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        SDL_Delay(16);
    }

    SDL_Quit();

}
