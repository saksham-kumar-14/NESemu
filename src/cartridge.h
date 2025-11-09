#pragma once
#include <vector>
#include <string>
#include <cstdint>

class Cartridge {
public:
    bool romLoad = false;
    uint8_t mapperID = 0;
    uint8_t mirror = 0;

    std::vector<uint8_t> PRGMemory;
    std::vector<uint8_t> CHRMemory;

    // Mapper-specific state
    uint8_t chrBankSelect = 0;

    Cartridge(const std::string& filename);

    // Mapper-aware read/write
    uint8_t cpuRead(uint16_t addr);
    void    cpuWrite(uint16_t addr, uint8_t data);
    uint8_t ppuRead(uint16_t addr);
    void    ppuWrite(uint16_t addr, uint8_t data);
};
