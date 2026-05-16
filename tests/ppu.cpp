#include "../src/ppu.h"
#include <iostream>
#include <iomanip>

int main() {
    PPU ppu;

    // Use mapped address space indexes 0x0006 and 0x0007 (addr & 0x0007)
    ppu.cpuWrite(0x0006, 0x3F); // High Byte
    ppu.cpuWrite(0x0006, 0x00); // Low Byte

    ppu.cpuWrite(0x0007, 0x1A); // Write color data 0x1A

    // Reset address latch to read it back
    ppu.cpuWrite(0x0006, 0x3F);
    ppu.cpuWrite(0x0006, 0x00);

    uint8_t color = ppu.cpuRead(0x0007);

    if (color == 0x1A) {
        std::cout << "PASSED!\n";
        return 0;
    } else {
        std::cerr << "FAILED! Palette Immediate Read failed!\n";
        std::cerr << "\t Expected: 0x1A\n";
        std::cerr << "\t Got:      0x" << std::hex << static_cast<int>(color) << "\n";
        return 1;
    }
}
