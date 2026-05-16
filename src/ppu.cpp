#include "ppu.h"
#include <cstdint>

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
        {
            uint8_t prev_generate_nmi = ctrl.generate_nmi;
            ctrl.reg = data;

            // if prev NMI is ON, we are in VBLANK
            // trigger an immediate execution interrupt
            if(prev_generate_nmi == 0 and ctrl.generate_nmi == 1 and status.vertical_blank == 1){
                if(cpu != nullptr){
                    cpu->NMI();
                }
            }
            break;
        }

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
                // First write: High Byte. Clean out bits 14 & 15 immediately.
                ppu_address = ((data & 0x3F) << 8) | (ppu_address & 0x00FF);
                address_latch = 1;
            } else {
                // Second write: Low Byte
                ppu_address = (ppu_address & 0xFF00) | data;
                address_latch = 0;
            }
            break;

        case 0x0007: // 0x2007: PPUDATA
            // Force mask to 14-bit PPU range right here to prevent downstream array overflow
            ppuWrite(ppu_address & 0x3FFF, data);

            // Auto-increment pointer
            ppu_address += (ctrl.increment_mode ? 32 : 1);
            ppu_address &= 0x3FFF; // Keep the actual register bound to 14-bit space
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
            // Clean the address pointer to ensure clear routing
            uint16_t clean_addr = ppu_address & 0x3FFF;

            data = ppu_data_buffer;

            //update the delay buffer with current VRAM data
            ppu_data_buffer = ppuRead(clean_addr);

            if (clean_addr >= 0x3F00) {
                data = ppuRead(clean_addr);
            }

            // auto increment the address pointer
            ppu_address += (ctrl.increment_mode ? 32 : 1);
            ppu_address &= 0x3FFF; // Maintain 14-bit bounds
            break;
    }
    return data;
}

uint8_t PPU::ppuRead(uint16_t addr) {
    addr &= 0x3FFF; // Cap at 14-bit PPU boundary

    if (addr >= 0x0000 && addr <= 0x1FFF) {
        // CHR ROM/RAM space on the Cartridge (Pattern Tables)
        // Handled by - return cartridge->ppuRead(addr);
        if (cart != nullptr) {
            return cart->ppuRead(addr);
        }
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
        if (cart != nullptr) {
            cart->ppuWrite(addr, data);
        }
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
    // Visible scanlines (from 0 to 239)
    if(scanline >= 0 and scanline <= 239){
        if(cycle >= 1 and cycle <= 256){

        }
    }

    // post render scanline (240)
    if(scanline == 240){
        // idle buffer it is
    }

    // VBLANK scanlines 241 to 260
    if(scanline == 241 and cycle == 1){
        status.vertical_blank = 1;  // VBLANK STARTS!!!!!

        if(ctrl.generate_nmi and cpu != nullptr) {
            cpu->NMI();
        }
    }

    // pre render scanline (261)
    if(scanline == 261){
        if(cycle == 1){
            // end of VBLANK. clear flags to prepare for next visible frame
            status.vertical_blank = 0;
            status.sprite_zero_hit = 0;
            status.sprite_overflow = 0;
        }
    }

    // advance the clock pipeline matrix
    cycle++;
    if(cycle >= 341){
        cycle = 0;
        scanline++;
        if(scanline >= 262){
            scanline = 0;
            frame_complete = true;
        }
    }

}

void PPU::drawDebugPatterntable(uint8_t table_index) {
    for(uint16_t tileY = 0; tileY < 16; ++tileY) {
        for(uint16_t tileX = 0; tileX < 16; ++tileX) {

            // byte offset in memory for this specific tile
            uint16_t offset = tileY * 256 + tileX * 16;

            // 8x8 is the tile
            for(uint16_t row = 0; row < 8; ++row) {
                // reads two bitplanes
                uint16_t address = (table_index * 0x1000) + offset + row;
                uint8_t lsb_bitplane = ppuRead(address);
                uint8_t msb_bitplane = ppuRead(address + 8);

                for(uint16_t col = 0; col < 8; ++col) {
                    uint8_t lsb = (lsb_bitplane & (0x80 >> col)) > 0;
                    uint8_t msb = (msb_bitplane & (0x80 >> col)) > 0;
                    uint8_t pixel_color = (msb << 1) | lsb;

                    uint8_t system_color = 0x0F; // Black
                    if (pixel_color == 1) system_color = 0x2D; // Dark Gray
                    if (pixel_color == 2) system_color = 0x00; // Light Gray
                    if (pixel_color == 3) system_color = 0x30; // White

                    int x = (tileX * 8) + col;
                    int y = (tileY * 8) + row;

                    screen_pixels[y * 256 + x] = NES_SYSTEM_PALETTE[system_color];
                }
            }


        }
    }
}
