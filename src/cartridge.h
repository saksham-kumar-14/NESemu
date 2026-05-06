#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include "mapper.h"

class Cartridge {
public:
    bool romLoad = false;
    std::uint8_t mapperID = 0;
    std::uint8_t mirror = 0;

    std::uint8_t prgBanks = 0; // Stored to pass to mapper
    std::uint8_t chrBanks = 0; // Stored to pass to mapper

    std::shared_ptr<Mapper> pMapper; // Polymorphic pointer to the active mapper

    std::vector<std::uint8_t> PRGMemory;
    std::vector<std::uint8_t> CHRMemory;

    Cartridge(const std::string& filename);

    bool cpuRead(uint16_t addr, uint8_t& data);
    bool cpuWrite(uint16_t addr, uint8_t data);
};
