#include "cartridge.h"
#include <fstream>
#include <iostream>

Cartridge::Cartridge(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open ROM: " << filename << '\n';
        return;
    }

    uint8_t header[16];
    ifs.read(reinterpret_cast<char*>(header), 16);
    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A) {
        std::cerr << "Invalid NES header\n";
        return;
    }

    uint8_t prgBanks = header[4];
    uint8_t chrBanks = header[5];
    uint8_t flag6    = header[6];
    uint8_t flag7    = header[7];

    mirror    = flag6 & 0x01;
    mapperID  = ((flag7 >> 4) << 4) | (flag6 >> 4);
    bool trainer = flag6 & 0x04;
    if (trainer) ifs.ignore(512);

    PRGMemory.resize(prgBanks * 16 * 1024);
    ifs.read(reinterpret_cast<char*>(PRGMemory.data()), PRGMemory.size());

    CHRMemory.resize(chrBanks * 8 * 1024);
    if (chrBanks > 0)
        ifs.read(reinterpret_cast<char*>(CHRMemory.data()), CHRMemory.size());
    else
        CHRMemory.resize(8192, 0); // allocate CHR RAM if cart lacks CHR ROM

    romLoad = true;
    ifs.close();

    std::cout << "First PRG bytes: ";
    for (int i = 0; i < 8; ++i)
        std::cout << std::hex << (int)PRGMemory[i] << " ";
    std::cout << "\n";

    std::cout << "PRG Banks: " << (int)prgBanks
              << " (total " << (prgBanks * 16) << "KB)"
              << ", CHR Banks: " << (int)chrBanks
              << " (total " << (chrBanks * 8) << "KB)"
              << ", Mapper ID: " << (int)mapperID
              << ", Mirroring: " << ((mirror) ? "Vertical" : "Horizontal")
              << std::endl;
}

uint8_t Cartridge::cpuRead(uint16_t addr) {
    if (addr < 0x8000 || !romLoad) return 0x00;

    // --- Mapper 0: NROM ---
    if (mapperID == 0) {
        uint32_t mapped = addr - 0x8000;
        if (PRGMemory.size() == 0x4000) mapped %= 0x4000;
        return PRGMemory[mapped];
    }

    // --- Mapper 3: CNROM ---
    if (mapperID == 3) {
        uint32_t mapped = addr - 0x8000;
        if (PRGMemory.size() == 0x4000) mapped %= 0x4000;
        return PRGMemory[mapped];
    }

    return 0x00;
}

void Cartridge::cpuWrite(uint16_t addr, uint8_t data) {
    if (!romLoad) return;

    if (mapperID == 3 && addr >= 0x8000) {
        chrBankSelect = data & 0x03;
    }
}

uint8_t Cartridge::ppuRead(uint16_t addr) {
    if (addr >= 0x2000 || CHRMemory.empty()) return 0x00;

    if (mapperID == 0) {
        return CHRMemory[addr];
    }
    if (mapperID == 3) {
        uint32_t mapped = (chrBankSelect * 0x2000) + addr;
        mapped %= CHRMemory.size();
        return CHRMemory[mapped];
    }
    return 0x00;
}

void Cartridge::ppuWrite(uint16_t addr, uint8_t data) {
    if (addr >= 0x2000 || CHRMemory.empty()) return;

    if (mapperID == 0 || mapperID == 3) {
        uint32_t mapped = (mapperID == 3)
                            ? (chrBankSelect * 0x2000) + addr
                            : addr;
        if (mapped < CHRMemory.size())
            CHRMemory[mapped] = data;
    }
}
