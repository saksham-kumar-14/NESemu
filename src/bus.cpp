#include "bus.h"

void Bus::cpuWrite(uint16_t addr, uint8_t data) {
    if (addr <= 0x1FFF) {
        RAM[addr & 0x07FF] = data;
    } else if (addr >= 0x8000 && addr <= 0xFFFF) {
        if (cart && cart->romLoad && !cart->PRGMemory.empty()) {
            uint32_t mapped = addr - 0x8000;
            if (cart->PRGMemory.size() == 0x4000)
                mapped %= 0x4000;
            cart->PRGMemory[mapped] = data;
        }
    }
}

uint8_t Bus::cpuRead(uint16_t addr) {
    if (addr <= 0x1FFF) {
        return RAM[addr & 0x07FF];
    } else if (addr >= 0x8000 && addr <= 0xFFFF) {
        if (cart && cart->romLoad && !cart->PRGMemory.empty()) {
            uint32_t mapped = addr - 0x8000;
            if (cart->PRGMemory.size() == 0x4000)
                mapped %= 0x4000;
            return cart->PRGMemory[mapped];
        }
    }
    return 0x00;
}
