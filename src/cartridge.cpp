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

    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A) {    // 0x1A = EOF
        std::cerr << "Invalid header\n";
        romLoad = false;
        return;
    }

    std::uint8_t prgBanks = header[4];
    std::uint8_t chrBanks = header[5];
    std::uint8_t flag6 = header[6]; // Mirroring
    std::uint8_t flag7 = header[7];

    mirror = flag6 & 0x01;      // extract the last bit

    /*
        Flag 6 : [XXXX (Low) | ....]
        Flag 7 : [XXXX (High)| ....]
        mapperID : [XXXX (High) | XXXX (Low)]
     */
    mapperID = ((flag7 >> 4) << 4) | (flag6 >> 4);

    // Bit 2 of flag6 tell if trainer exists or not
    // trainer is junk code (512 bytes) added at the start of ROM for cheat codes, valid NES hardware doesn't use it
    // so ignore it
    bool trainer = flag6 & 0x04;
    if (trainer) {
        ifs.ignore(512);
    }

    // Loading program
    PRGMemory.resize(prgBanks * 1024 * 16);     // NES programs are always 16 KBs
    ifs.read(reinterpret_cast<char*>(PRGMemory.data()), PRGMemory.size());
    CHRMemory.resize(chrBanks * 1024 * 8);      // NES graphics are always 8 KBs
    if (chrBanks > 0) {
        ifs.read(reinterpret_cast<char*>(CHRMemory.data()), CHRMemory.size());
    }

    romLoad = true;
    ifs.close();
}
