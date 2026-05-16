#pragma once
#include <cstdint>
#include <array>
#include <iostream>

class PPU {
public:
    PPU();
    ~PPU() = default;

    // The CPU uses these to talk to the PPU registers (mirrored between 0x2000 - 0x3FFF)
    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);

    void Clock();

    bool frame_complete = false;

private:
    std::array<uint8_t, 2048> nametable;        // 2KB Internal VRAM for Nametables (enough for 2 physical screens)
    std::array<uint8_t, 32> pallete_table;      // 32 Bytes for Palettes (0x3F00 - 0x3FFF)
    std::array<uint8_t, 256> oam_memory;        // 256 Bytes of Object Attribute Memory for Sprites (64 sprites * 4 bytes each)

    // Memory Mapped REgisters
    // 0x2000: PPUCTRL
    union PPUCTRL {
        struct {
            uint8_t nametable_x: 1;             // Base nametable x-scroll (0 = $2000, 1 = $2400)
            uint8_t nametable_y: 1;             // Base nametable y-scroll (0 = $2000, 1 = $2800)
            uint8_t increment_mode: 1;          // VRAM address increment per 0x2007 read/write (0: +1, 1: +32)
            uint8_t sprite_pattern: 1;          // Sprite pattern table address for 8x8 sprites (0: $0000, 1: $1000)
            uint8_t background_pattern: 1;      // Background pattern table address (0: $0000, 1: $1000)
            uint8_t sprite_size: 1;             // Sprite size (0: 8x8, 1: 8x16)
            uint8_t master_slave: 1;            // Master/slave select (not used in NES)
            uint8_t generate_nmi: 1;            // Enable VBLANK NMI generation (0: off, 1: on)
        };
        uint8_t reg;
    } ctrl;

    // 0x2001: PPUMASK
    union PPUMASK {
        struct {
            uint8_t grayscale: 1;               // Grayscale mode (0: normal, 1: monochrome)
            uint8_t render_background_left: 1;  // Show background in leftmost 8 pixels of screen
            uint8_t render_sprites_left: 1;     // Show sprites in leftmost 8 pixels of screen
            uint8_t render_background: 1;       // Enable background rendering
            uint8_t render_sprites: 1;          // Enable sprite rendering
            uint8_t emphasize_red: 1;           // Color emphasis
            uint8_t emphasize_green: 1;         // Color emphasis
            uint8_t emphasize_blue: 1;          // Color emphasis
        };
        uint8_t reg;
    } mask;

    // 0x2002: PPUSTATUS
    union PPUSTATUS {
        struct {
            uint8_t open_bus: 5;                // Unused bits (returns garbage/last written data values)
            uint8_t sprite_overflow: 1;         // Set if > 8 sprites appear on a single scanline
            uint8_t sprite_zero_hit: 1;         // Set if Sprite 0 opaque pixel collides with background opaque pixel
            uint8_t vertical_blank: 1;          // Set when PPU enters VBLANK phase
        };
        uint8_t reg;
    } status;

    // 0x2003: OAMADDR
    uint8_t oam_addr = 0x00;

    // Internal Timing Clocks & Interface Tracking Latches
    int16_t scanline = 0;
    int16_t cycle = 0;

    uint8_t address_latch = 0;     // Toggles between 0 (high byte) and 1 (low byte) for 0x2006 writes
    uint8_t ppu_data_buffer = 0x00;// Essential 1-byte read delay buffer for 0x2007
    uint16_t ppu_address = 0x0000; // Constructed 14-bit VRAM address pointer

    // Sub-Bus VRAM Access Helpers
    // Used inside cpuRead/cpuWrite to read/write from PPU space (Pattern tables, Nametables, Palettes)
    uint8_t ppuRead(uint16_t addr);
    void ppuWrite(uint16_t addr, uint8_t data);
};
