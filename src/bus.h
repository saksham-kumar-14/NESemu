#pragma once
#include "cartridge.h"
#include "cpu.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>


class Bus{
public:
    CPU6502 cpu;
    Cartridge* cart;

    static const uint8_t RAMsize = 2048; // 2Kb
    std::array<uint8_t, RAMsize> RAM;

    Bus(){
        cpu.memory = std::vector<uint8_t>(0x1000000); // 64Kb
    }

    void ConnectCartridge(Cartridge *c){
        cart = c;
    }

    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);
};
