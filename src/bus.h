#pragma once
#include "cartridge.h"
#include "cpu.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

/*
    Memory map handled by Bus:
    0x0000 - 0x07FF : 2KB internal RAM
    0x0800 - 0x1FFF : Mirror of 0x0000 - 0x07FF
    0x2000 - 0x3FFF : PPU registers (mirrored every 8 bytes)
    0x8000 - 0xFFFF : PRG ROM (from cartridge)
*/

class Bus {
public:
    CPU6502 cpu;
    Cartridge* cart = nullptr;

    static constexpr size_t RAMsize = 2048; // 2KB
    std::array<uint8_t, RAMsize> RAM{};

    Bus() {
        RAM.fill(0x00);
        cpu.integrateBus(this);
    }

    void ConnectCartridge(Cartridge* c) {
        cart = c;
    }

    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);
};
