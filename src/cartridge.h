#pragma once
#include <vector>
#include <string>
#include <cstdint>

class Cartridge {
public:
    bool romLoad = false;
    std::uint8_t mapperID = 0;
    std::uint8_t mirror = 0;

    std::vector<std::uint8_t> PRGMemory;
    std::vector<std::uint8_t> CHRMemory;

    Cartridge(const std::string& filename);
};
