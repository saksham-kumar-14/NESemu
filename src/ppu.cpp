#include "ppu.h"

PPU::PPU() {
    ctrl.reg = 0x00;
    mask.reg = 0x00;
    status.reg = 0x00;

    nametable.fill(0x00);
    pallete_table.fill(0x00);
    oam_memory.fill(0x00);
}

void PPU::cpuWrite(uint16_t addr, uint8_t data) {
    // incoming address from CPU Bus is already masked (addr & 0x0007)
    switch (addr) {
        case 0x0000: // 0x2000: PPUCTRL
            ctrl.reg = data;
            break;

        case 0x0001: // 0x2001: PPUMASK
            mask.reg = data;
            break;

        case 0x0002: // 0x2002: PPUSTATUS  read only
            break;

        case 0x0003: // 0x2003: OAMADDR
            oam_addr = data;
            break;

        case 0x0004: // 0x2004: OAMDATA
            oam_memory[oam_addr] = data;
            oam_addr++; // auto increments on every write
            break;

        case 0x0005: // 0x2005: PPUSCROLL
            // fine scrolling implementation
            break;

        case 0x0006: // 0x2006: PPUADDR
            if (address_latch == 0) {
                // First write: High Byte (mask out bit 14, 15 since PPU address spaces are 14-bit)
                ppu_address = (ppu_address & 0x00FF) | ((data & 0x3F) << 8);
                address_latch = 1;
            } else {
                // Second write: Low Byte
                ppu_address = (ppu_address & 0xFF00) | data;
                address_latch = 0;
            }
            break;

        case 0x0007: // 0x2007: PPUDATA
            ppuWrite(ppu_address, data);

            // Increment address pointer based on PPUCTRL setup (+1 for horizontal, +32 for vertical)
            ppu_address += (ctrl.increment_mode ? 32 : 1);
            break;
    }
}

uint8_t PPU::cpuRead(uint16_t addr) {
    uint8_t data = 0x00;
    switch (addr) {
        case 0x0000: // 0x2000: PPUCTRL (Write Only)
            break;

        case 0x0001: // 0x2001: PPUMASK (Write Only)
            break;

        case 0x0002: // 0x2002: PPUSTATUS
            // Mask out the lower 5 bits to return open bus state data if needed
            data = (status.reg & 0xE0) | (ppu_data_buffer & 0x1F);

            // Reading status clears VBLANK flag and resets the 0x2006 latch address state
            status.vertical_blank = 0;
            address_latch = 0;
            break;

        case 0x0003: // 0x2003: OAMADDR (Write Only)
            break;

        case 0x0004: // 0x2004: OAMDATA
            data = oam_memory[oam_addr];
            break;

        case 0x0005: // 0x2005: PPUSCROLL (Write Only)
            break;

        case 0x0006: // 0x2006: PPUADDR (Write Only)
            break;

        case 0x0007: // 0x2007: PPUDATA
            // Standard VRAM reads are delayed by 1 cycle through a buffer
            data = ppu_data_buffer;
            ppu_data_buffer = ppuRead(ppu_address);

            // !!!!!!!!!EXCEPTION: Palette reads ($3F00-$3FFF) are returned immediately without buffer delay!!!!!!!!
            if (ppu_address >= 0x3F00) {
                data = ppu_data_buffer;
            }

            // auto increments pointer
            ppu_address += (ctrl.increment_mode ? 32 : 1);
            break;
    }
    return data;
}


uint8_t PPU::ppuRead(uint16_t addr) {
    addr &= 0x3FFF; // Cap at 14-bit PPU boundary

    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // CHR ROM/RAM space on the Cartridge (Pattern Tables)
        // Handled by - return cartridge->ppuRead(addr);
        return 0x00;
    }
    else if (addr >= 0x2000 && addr <= 0x3EFF) {
        // Nametable Space (Mirrored down to internal 2KB array size)
        addr &= 0x0FFF;

        // Actual nametable mirroring layout handling (Horizontal/Vertical)
        // should eventually be processed here or sent to the cartridge mapper logic.
        return nametable[addr & 0x07FF];
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        // Palette Address space mirroring rule
        addr &= 0x001F;

        // Palette mirroring trick: 0x3F10, 0x3F14, 0x3F18, 0x3F1C mirror down to 0x3F00, 0x3F04...
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;

        return pallete_table[addr];
    }
    return 0x00;
}

void PPU::ppuWrite(uint16_t addr, uint8_t data) {
    addr &= 0x3FFF;

    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // CHR RAM configuration writes (if cartridge supports it)
        // Handled by - cartridge->ppuWrite(addr, data);
    }
    else if (addr >= 0x2000 && addr <= 0x3EFF) {
        addr &= 0x0FFF;
        nametable[addr & 0x07FF] = data;
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        addr &= 0x001F;
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;

        pallete_table[addr] = data;
    }
}

void PPU::Clock() {
    cycle++;
    if (cycle >= 341) {
        cycle = 0;
        scanline++;
        if (scanline >= 262) {
            scanline = 0;
            frame_complete = true;
        }
    }
}
