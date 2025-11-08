#pragma once
#include <array>
#include <cstdint>
#include <SDL2/SDL.h>

class Bus;

class PPU {
public:
    static const int SCREEN_WIDTH = 256;
    static const int SCREEN_HEIGHT = 240;

    PPU();
    ~PPU();

    void ConnectBus(Bus* b) { bus = b; }

    void Reset();
    void Step();
    void RenderFrame();
    void UpdateSurface(SDL_Surface* surface);

    uint8_t ppuRead(uint16_t addr);
    void ppuWrite(uint16_t addr, uint8_t data);

    void RenderPatternTables();

private:
    Bus* bus = nullptr;

    std::array<uint8_t, 2048> vram{};
    std::array<uint8_t, 32> palette{};
    std::array<uint32_t, SCREEN_WIDTH * SCREEN_HEIGHT> framebuffer{};
};
