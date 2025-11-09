#pragma once
#include <array>
#include <cstdint>
#include <SDL2/SDL.h>

class Bus;

class PPU {
public:
    static const int SCREEN_WIDTH = 256;
    static const int SCREEN_HEIGHT = 240;
    static const uint32_t NES_PALETTE[64];

    PPU();
    ~PPU();

    void ConnectBus(Bus* b) { bus = b; }

    void Reset();
    void Step();
    void RenderFrame();
    void UpdateSurface(SDL_Surface* surface);

    uint8_t ppuRead(uint16_t addr);
    void ppuWrite(uint16_t addr, uint8_t data);

    void RenderPatternTables(); // CHR ROME
    uint32_t NESColor(uint8_t index); // COLOR PALETTE

    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);

    void RenderNametable(uint16_t baseAddr);

    std::array<uint8_t, 256> OAM{};
    void RenderSprites();

private:
    Bus* bus = nullptr;

    std::array<uint8_t, 2048> vram{};
    std::array<uint8_t, 32> paletteRAM{};
    std::array<uint32_t, SCREEN_WIDTH * SCREEN_HEIGHT> framebuffer{};

    // PPU Registers
    uint8_t PPUCTRL = 0x00;
    uint8_t PPUMASK = 0x00;
    uint8_t PPUSTATUS = 0xA0;
    uint8_t OAMADDR = 0x00;

    uint16_t vramAddr = 0x0000; // current VRAM address
    uint16_t tempAddr = 0x0000; // temporary VRAM address
    uint8_t fineX = 0x00;       // fine x scroll
    bool addrLatch = false;     // PPUADDR/PPUSCROLL toggle

    uint8_t ppuDataBuffer = 0x00;
};
