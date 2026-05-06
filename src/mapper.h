#pragma once
#include <cstdint>

class Mapper {
protected:
    uint8_t prgBanks = 0;
    uint8_t chrBanks = 0;

public:
    Mapper(uint8_t prgBanks, uint8_t chrBanks) : prgBanks(prgBanks), chrBanks(chrBanks) {}
    virtual ~Mapper() = default;

    // Transform CPU bus address into PRG ROM physical address
    virtual bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) = 0;
};
