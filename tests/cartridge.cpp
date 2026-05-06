#include <iostream>
#include <fstream>
#include "../src/cartridge.h"
#include <cassert>

void create_dummy_rom(const std::string& filename) {
    std::ofstream ofs(filename, std::ios::binary);
    // iNES header
    ofs.write("NES\x1A", 4);
    uint8_t prg_banks = 1; // 16KB
    uint8_t chr_banks = 1; // 8KB
    ofs.put(prg_banks);
    ofs.put(chr_banks);
    ofs.put(0x00); // flags 6
    ofs.put(0x00); // flags 7
    for (int i = 0; i < 8; ++i) ofs.put(0x00); // padding
    
    // PRG ROM (16KB)
    for (int i = 0; i < 16 * 1024; ++i) {
        ofs.put((uint8_t)(i & 0xFF));
    }
    // CHR ROM (8KB)
    for (int i = 0; i < 8 * 1024; ++i) {
        ofs.put(0xFF);
    }
}

int main() {
    create_dummy_rom("dummy.nes");
    Cartridge cart("dummy.nes");

    assert(cart.romLoad == true);
    assert(cart.mapperID == 0);
    assert(cart.PRGMemory.size() == 16 * 1024);
    
    uint8_t data;
    bool read_success = cart.cpuRead(0x8000, data);
    assert(read_success);
    assert(data == 0x00); // First byte of PRG
    
    read_success = cart.cpuRead(0x8001, data);
    assert(read_success);
    assert(data == 0x01); // Second byte
    
    // Test write
    bool write_success = cart.cpuWrite(0x8000, 0x99);
    assert(write_success); 
    
    // Write should not actually modify PRG ROM in Mapper 0
    // Wait, the dummy write method just returns true without modifying PRGMemory right now,
    // so a read should still be 0x00
    read_success = cart.cpuRead(0x8000, data);
    assert(read_success);
    assert(data == 0x00);
    
    // Test invalid read
    read_success = cart.cpuRead(0x2000, data);
    assert(!read_success);
    
    std::cout << "All Cartridge tests passed!\n";
    std::remove("dummy.nes");
    return 0;
}