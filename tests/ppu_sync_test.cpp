// in main.cpp (or tests/runner)
#include "../src/bus.h"
#include "../src/ppu.h"
#include "../src/cartridge.h"
#include <SDL2/SDL.h>

int main() {
    Cartridge cart("DonkeyKong.nes");
    Bus bus;
    PPU ppu;

    bus.ConnectCartridge(&cart);
    bus.ConnectPPU(&ppu);
    ppu.ConnectBus(&bus);

    // Ensure CPU knows bus
    bus.cpu.integrateBus(&bus);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("NES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT, 0);
    SDL_Surface* surface = SDL_GetWindowSurface(window);

    // Reset CPU/PPU
    bus.cpu.Reset();
    ppu.Reset();

    bool running = true;
    SDL_Event event;
    while (running) {
        // Poll OS events (keep window responsive)
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        // One CPU step (this executes one instruction)
        bus.cpu.Clock();

        // For each CPU instruction, run 3 PPU clocks (approximate)
        ppu.Clock();
        ppu.Clock();
        ppu.Clock();

        // Render when PPU finished a frame
        if (ppu.frameComplete) {
            ppu.frameComplete = false;
            // call your frame rendering. You might call RenderNametable + RenderSprites etc.
            ppu.RenderNametable(0x2000);
            ppu.RenderSprites();
            ppu.UpdateSurface(surface);
            SDL_UpdateWindowSurface(window);
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
