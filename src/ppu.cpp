#include "ppu.h"
#include "bus.h"
#include <cstdint>
#include <cstring>

PPU::PPU() {
    Reset();
}

PPU::~PPU() {}

void PPU::Reset() {
    vram.fill(0);
    palette.fill(0);
    framebuffer.fill(0x000000FF);
}

uint8_t PPU::ppuRead(uint16_t addr) {
    addr &= 0x3FFF;

    if (addr < 0x2000) {
        return bus->ppuRead(addr);
    }
    else if (addr >= 0x2000 && addr <= 0x3EFF) {
        return vram[addr & 0x07FF];
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        return palette[addr & 0x001F];
    }

    return 0x00;
}

void PPU::ppuWrite(uint16_t addr, uint8_t data) {
    addr &= 0x3FFF;

    if (addr < 0x2000) {
        bus->ppuWrite(addr, data);
    }
    else if (addr >= 0x2000 && addr <= 0x3EFF) {
        vram[addr & 0x07FF] = data;
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        palette[addr & 0x001F] = data;
    }
}

void PPU::Step() {
    // Each PPU cycle would go here
}

void PPU::RenderFrame() {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            uint8_t pixel = (x ^ y) & 0xFF;
            framebuffer[y * SCREEN_WIDTH + x] = (pixel << 16) | (pixel << 8) | pixel;
        }
    }
}

void PPU::UpdateSurface(SDL_Surface* surface) {
    SDL_LockSurface(surface);
    std::memcpy(surface->pixels, framebuffer.data(), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    SDL_UnlockSurface(surface);
}

// For viewing CHR ROM tiles
void PPU::RenderPatternTables(){
    if(!bus || !bus->cart || bus->cart->CHRMemory.empty()) {
        std::cerr << "NO ROM FOUND\n";
        return;
    }

    // acc to nes std layout, 16 tiles per row
    // each tile = 8x8 pixels
    auto &chr = bus->cart->CHRMemory;
    const int tilesPerRow = 16;
    const int tileSize = 8;
    const int bytesPerTile = 16;

    framebuffer.fill(0x000000FF);

    int nTiles = chr.size() / bytesPerTile;

    for(int i = 0; i < nTiles; ++i){
        int tileX = (i % tilesPerRow) * tileSize;
        int tileY = (i / tilesPerRow) * tileSize;

        int baseAddr = i * bytesPerTile;

        //
        for(int row = 0; row < 8; ++row){
            uint8_t plane0 = chr[baseAddr + row];
            uint8_t plane1 = chr[baseAddr + row + 8];

            for(int col = 0; col < 8; ++col){
                uint8_t lsbit = (plane0 >> (7 - col)) & 1;
                uint8_t msbit = (plane1 >> (7 - col)) & 1;
                uint8_t pixel = (msbit << 1) | lsbit;

                uint8_t shade = pixel * 85;
                int x = tileX + col;
                int y = tileY + row;

                if(x < SCREEN_WIDTH and y < SCREEN_HEIGHT) {
                    framebuffer[y * SCREEN_WIDTH + x] = (shade << 16) | (shade << 6) | shade;
                }
            }
        }
    }
}
