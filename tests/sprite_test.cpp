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
        "Sprite Test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        PPU::SCREEN_WIDTH, PPU::SCREEN_HEIGHT, 0
    );
    SDL_Surface* surface = SDL_GetWindowSurface(window);

    // Fill sprite data (draw 3 test sprites)
    ppu.OAM[0] = 50;   // Y
    ppu.OAM[1] = 1;    // Tile index
    ppu.OAM[2] = 0x01; // Palette 1
    ppu.OAM[3] = 100;  // X

    ppu.OAM[4] = 60;
    ppu.OAM[5] = 2;
    ppu.OAM[6] = 0x02;
    ppu.OAM[7] = 120;

    ppu.OAM[8] = 70;
    ppu.OAM[9] = 3;
    ppu.OAM[10] = 0x03;
    ppu.OAM[11] = 140;

    // Render both background + sprites
    ppu.RenderNametable(0x2000);
    ppu.RenderSprites();

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
