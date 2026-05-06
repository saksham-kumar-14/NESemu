#include "bus.h"

void Bus::cpuWrite(uint16_t addr, uint8_t data) {
    if (cart && cart->romLoad && cart->cpuWrite(addr, data)) {
        // Cartridge handled the write (e.g. mapper config)
    } else if (addr <= 0x1FFF) {
        RAM[addr & 0x07FF] = data;
    }
}

uint8_t Bus::cpuRead(uint16_t addr) {
    uint8_t data = 0x00;
    if (cart && cart->romLoad && cart->cpuRead(addr, data)) {
        return data;
    } else if (addr <= 0x1FFF) {
        return RAM[addr & 0x07FF];
    }
    return 0x00;
}
