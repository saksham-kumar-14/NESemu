#include "bus.h"

uint8_t Bus::cpuRead(uint16_t addr) {
    if (addr <= 0x1FFF)
        return RAM[addr & 0x07FF];
    else if (addr >= 0x2000 && addr <= 0x3FFF)
        return ppu->cpuRead(addr);
    else if (addr >= 0x8000 && addr <= 0xFFFF)
        return cart ? cart->cpuRead(addr) : 0x00;
    return 0x00;
}

void Bus::cpuWrite(uint16_t addr, uint8_t data) {
    if (addr <= 0x1FFF)
        RAM[addr & 0x07FF] = data;
    else if (addr >= 0x2000 && addr <= 0x3FFF)
        ppu->cpuWrite(addr, data);
    else if (addr >= 0x8000 && addr <= 0xFFFF)
        if (cart) cart->cpuWrite(addr, data);
}

uint8_t Bus::ppuRead(uint16_t addr) {
    if (cart && addr < 0x2000) {
        return cart->ppuRead(addr);
    }
    return 0x00;
}

void Bus::ppuWrite(uint16_t addr, uint8_t data) {
    if (cart && addr < 0x2000) {
        cart->ppuWrite(addr, data);
        return;
    }
}
