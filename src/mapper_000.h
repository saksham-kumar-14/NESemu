#pragma once
#include "mapper.h"

class Mapper_000 : public Mapper {
public:
    Mapper_000(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {}

    bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr) override {
        if (addr >= 0x8000 && addr <= 0xFFFF) {
            // If PRGROM is 16KB (1 bank), mask it with 0x3FFF to mirror it.
            // If PRGROM is 32KB (2 banks), mask it with 0x7FFF.
            mapped_addr = addr & (prgBanks > 1 ? 0x7FFF : 0x3FFF);
            return true;
        }
        return false;
    }

    bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override {
        if (addr >= 0x8000 && addr <= 0xFFFF) {
            mapped_addr = addr & (prgBanks > 1 ? 0x7FFF : 0x3FFF);
            // NROM is read-only, but we return true to let the Cartridge know the mapper handled the address space.
            return true;
        }
        return false;
    }
};
