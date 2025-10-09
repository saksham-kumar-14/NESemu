#include "cartridge.h"
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

    std::uint8_t prgBanks = header[4];
    std::uint8_t chrBanks = header[5];
    std::uint8_t flag6 = header[6]; // Mirroring
    std::uint8_t flag7 = header[7];

    mirror = flag6 & 0x01;
    mapperID = ((flag7 >> 4) << 4) | (flag6 >> 4);

    bool trainer = flag6 & 0x04;
    if (trainer) {
        ifs.ignore(512);
    }

    PRGMemory.resize(prgBanks * 1024 * 16);
    ifs.read(reinterpret_cast<char*>(PRGMemory.data()), PRGMemory.size());
    CHRMemory.resize(chrBanks * 1024 * 8);
    if (chrBanks > 0) {
        ifs.read(reinterpret_cast<char*>(CHRMemory.data()), CHRMemory.size());
    }

    romLoad = true;
    ifs.close();
}
