// cartridge.cpp
#include "cartridge.h"
#include "mapper_000.h"
#include <fstream>
#include <iostream>

Cartridge::Cartridge(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open ROM: " << filename << '\n';
        romLoad = false;
        return;
    }

    std::uint8_t header[16];
    ifs.read(reinterpret_cast<char*>(header), 16);

    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A) {
        std::cerr << "Invalid header\n";
        romLoad = false;
        return;
    }

    prgBanks = header[4];
    chrBanks = header[5];
    std::uint8_t flag6 = header[6]; // Mirroring
    std::uint8_t flag7 = header[7];

    mirror = flag6 & 0x01;

    mapperID = ((flag7 >> 4) << 4) | (flag6 >> 4);

    bool trainer = flag6 & 0x04;
    if (trainer) {
        ifs.ignore(512);
    }

    // Loading program
    PRGMemory.resize(prgBanks * 1024 * 16);
    ifs.read(reinterpret_cast<char*>(PRGMemory.data()), PRGMemory.size());
    CHRMemory.resize(chrBanks * 1024 * 8);
    if (chrBanks > 0) {
        ifs.read(reinterpret_cast<char*>(CHRMemory.data()), CHRMemory.size());
    }

    // Deciding the mapper
    switch (mapperID) {
        case 0:
            pMapper = std::make_shared<Mapper_000>(prgBanks, chrBanks);
            break;
        // case 1:
        //     pMapper = std::make_shared<Mapper_001>(prgBanks, chrBanks);
        //     break;
        default:
            std::cerr << "Mapper " << (int)mapperID << " not supported yet!\n";
            romLoad = false; // Fail load if mapper is unknown
            return;
    }

    romLoad = true;
    ifs.close();
}

bool Cartridge::cpuRead(uint16_t addr, uint8_t& data) {
    uint32_t mapped_addr = 0;
    // Ask the mapper to translate the address
    if (pMapper->cpuMapRead(addr, mapped_addr)) {
        data = PRGMemory[mapped_addr];
        return true;
    }
    return false;
}

bool Cartridge::cpuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = 0;
    // Ask the mapper to translate the address
    if (pMapper->cpuMapWrite(addr, mapped_addr, data)) {
        // NROM is read-only, but later mappers (like PRG RAM) might allow writes here:
        // PRGMemory[mapped_addr] = data;
        return true;
    }
    return false;
}

uint8_t Cartridge::ppuRead(uint16_t addr) {
    uint32_t mapped_addr = 0;
    if (pMapper->ppuMapRead(addr, mapped_addr)) {
        return CHRMemory[mapped_addr];
    }
    return 0x00;
}

void Cartridge::ppuWrite(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = 0;
    if (pMapper->ppuMapWrite(addr, mapped_addr)) {
        CHRMemory[mapped_addr] = data;
    }
}
