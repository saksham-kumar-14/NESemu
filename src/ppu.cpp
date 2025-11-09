#include "ppu.h"
#include "bus.h"
#include <cstdint>
#include <cstring>
#include <iostream>

PPU::PPU() {
    Reset();
}

PPU::~PPU() {}

void PPU::Reset() {
    vram.fill(0);
    paletteRAM.fill(0);
    framebuffer.fill(0x000000);

    // Default colors (for pattern table)
    paletteRAM[0] = 0x0F;
    paletteRAM[1] = 0x11;
    paletteRAM[2] = 0x21;
    paletteRAM[3] = 0x31;
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
        addr &= 0x001F;
        // Palette mirroring
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;
        return paletteRAM[addr] & 0x3F;
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
        addr &= 0x001F;
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;
        paletteRAM[addr] = data & 0x3F;
    }
}

void PPU::Step() {
    // Each PPU cycle will go here
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

// Official NES master palette (approximate RGB values)
const uint32_t PPU::NES_PALETTE[64] = {
    0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
    0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
    0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
    0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
    0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
    0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
    0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
    0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
};

uint32_t PPU::NESColor(uint8_t index) {
    return NES_PALETTE[index % 64];
}

void PPU::RenderPatternTables() {
    if (!bus || !bus->cart || bus->cart->CHRMemory.empty()) {
        std::cerr << "NO ROM FOUND\n";
        return;
    }

    auto &chr = bus->cart->CHRMemory;
    const int tilesPerRow = 16;
    const int tileSize = 8;
    const int bytesPerTile = 16;

    framebuffer.fill(0x000000);

    int nTiles = chr.size() / bytesPerTile;
    std::cout << "Rendering " << nTiles << " CHR tiles\n";

    for (int i = 0; i < nTiles; ++i) {
        int tileX = (i % tilesPerRow) * tileSize;
        int tileY = (i / tilesPerRow) * tileSize;

        int baseAddr = i * bytesPerTile;

        for (int row = 0; row < 8; ++row) {
            uint8_t plane0 = chr[baseAddr + row];
            uint8_t plane1 = chr[baseAddr + row + 8];

            for (int col = 0; col < 8; ++col) {
                uint8_t lsbit = (plane0 >> (7 - col)) & 1;
                uint8_t msbit = (plane1 >> (7 - col)) & 1;
                uint8_t pixel = (msbit << 1) | lsbit;

                uint8_t colorIndex = paletteRAM[pixel & 0x03];
                uint32_t color = NESColor(colorIndex);

                int x = tileX + col;
                int y = tileY + row;

                if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
                    framebuffer[y * SCREEN_WIDTH + x] = color;
                }
            }
        }
    }
}

uint8_t PPU::cpuRead(uint16_t addr) {
    uint8_t data = 0x00;
    switch (addr & 0x0007) { // mirror every 8 bytes
        case 0x0002: // PPUSTATUS
            data = (PPUSTATUS & 0xE0) | (ppuDataBuffer & 0x1F);
            PPUSTATUS &= ~0x80; // clear VBlank
            addrLatch = false;
            break;
        case 0x0004: // OAMDATA
            data = OAM[OAMADDR];
            break;
        case 0x0007: // PPUDATA
            data = ppuDataBuffer;
            ppuDataBuffer = ppuRead(vramAddr);
            if (vramAddr >= 0x3F00)
                data = ppuDataBuffer; // no delay for palette
            vramAddr += (PPUCTRL & 0x04) ? 32 : 1;
            break;
    }
    return data;
}

void PPU::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr & 0x0007) { // mirror every 8 bytes
        case 0x0000: // PPUCTRL
            PPUCTRL = data;
            tempAddr = (tempAddr & 0xF3FF) | ((data & 0x03) << 10);
            break;

        case 0x0001: // PPUMASK
            PPUMASK = data;
            break;

        case 0x0003: // OAMADDR
            OAMADDR = data;
            break;

        case 0x0004: // OAMDATA
            OAM[OAMADDR++] = data;
            break;

        case 0x0005: // PPUSCROLL
            if (!addrLatch) {
                fineX = data & 0x07;
                tempAddr = (tempAddr & 0xFFE0) | (data >> 3);
                addrLatch = true;
            } else {
                tempAddr = (tempAddr & 0x8FFF) | ((data & 0x07) << 12);
                tempAddr = (tempAddr & 0xFC1F) | ((data & 0xF8) << 2);
                addrLatch = false;
            }
            break;

        case 0x0006: // PPUADDR
            if (!addrLatch) {
                tempAddr = (tempAddr & 0x00FF) | ((data & 0x3F) << 8);
                addrLatch = true;
            } else {
                tempAddr = (tempAddr & 0xFF00) | data;
                vramAddr = tempAddr;
                addrLatch = false;
            }
            break;

        case 0x0007: // PPUDATA
            ppuWrite(vramAddr, data);
            vramAddr += (PPUCTRL & 0x04) ? 32 : 1; // traversing x or y
            break;
    }
}

// $2000–$23BF - Name Table - 32 × 30 = 960 bytes (tile indices)
void PPU::RenderNametable(uint16_t baseAddr){
    if (!bus || !bus->cart || bus->cart->CHRMemory.empty()) {
        std::cerr << "NO ROM FOUND\n";
        return;
    }
    framebuffer.fill(0x000000);

    auto &chr = bus->cart->CHRMemory;
    const int tileSize = 8;
    const int bytesPerTile = 16;

    for(int row = 0; row < 30; ++row){
        for(int col = 0; col < 32; ++col){
            uint16_t tileAddr = baseAddr + (row * 32) + col;
            uint8_t tileIndex = ppuRead(tileAddr);

            // get pattern table address
            uint16_t patternBase = (PPUCTRL & 0x10) ? 0x1000 : 0x0000;
            uint16_t chrAddr = patternBase + tileAddr * bytesPerTile;

            // which palette to use
            uint16_t attrBase = baseAddr + 0x03C0;
            int attrX = col / 4;
            int attrY = row / 4;
            uint8_t attrByte = ppuRead(attrBase + attrY * 8 + attrX);

            int shift = ((row & 4) / 2) * 4 + ((col & 4) / 2) * 2;
            uint8_t palleteSelect = (attrByte >> shift) & 0x03;
            uint16_t palleteBase = 0x3F00 + (palleteSelect << 2);


            // draw 8x8 tile
            for(int i = 0; i < 8; ++i){     // y
                uint8_t plane0 = chr[chrAddr + i];
                uint8_t plane1 = chr[chrAddr + i + 8];
                for(int j = 0; j < 8; ++j){ // x
                    uint8_t lsbit = (plane0 >> (7 - j)) & 1;
                    uint8_t msbit = (plane1 >> (7 - j)) & 1;
                    uint8_t pixel = (msbit << 1) | lsbit;

                    if(pixel == 0) continue;

                    uint8_t colorIndex = ppuRead(palleteBase + pixel);
                    uint32_t color = NESColor(colorIndex);

                    int x = col * tileSize + j;
                    int y = row * tileSize + i;
                    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT){
                        framebuffer[y * SCREEN_WIDTH + x] = color;
                    }
                }
            }
        }
    }

    std::cout << "Rendered Nametable from $"
                  << std::hex << baseAddr << std::dec << "\n";
}


// Sprite palletes are at 0x3F10 - 0x3F1F
void PPU::RenderSprites(){
    auto &chr = bus->cart->CHRMemory;
    const int bytesPerTile = 16;

    for(int i = 0; i < 64; ++i){
        uint8_t yPos = OAM[i * 4];
        uint8_t titleIndex = OAM[i * 4 + 1];
        uint8_t attr = OAM[i * 4 + 2];
        uint8_t xPos = OAM[i * 4 + 3];

        uint8_t palleteNo = attr & 0x03;
        bool flipH = attr & 0x40;
        bool flipV = attr & 0x80;

        uint16_t patternBase = (PPUCTRL & 0x08) ? 0x1000 : 0x0000;
        uint16_t chrAddr = patternBase + titleIndex * bytesPerTile;
        uint16_t paletteBase = 0x3F10 + (palleteNo << 2);

        for(int row = 0; row < 8; ++row){
            uint8_t plane0 = chr[chrAddr + row];
            uint8_t plane1 = chr[chrAddr + row + 8];

            for(int col = 0; col < 8; ++col){
                int py = flipV ? (7 - row) : row;
                int px = flipH ? (7 - col) : col;

                uint8_t lsbit = (plane0 >> (7 - col)) & 1;
                uint8_t msbit = (plane1 >> (7 - col)) & 1;
                uint8_t pixel = (msbit << 1) | lsbit;
                if(pixel == 0) continue;

                uint8_t colorIndex = ppuRead(paletteBase + pixel);
                uint32_t color = NESColor(colorIndex);

                int x = xPos + px;
                int y = yPos + py;
                if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT){
                    framebuffer[y * SCREEN_WIDTH + x] = color;
                }
            }
        }
    }
}
