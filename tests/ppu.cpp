#include "../src/ppu.h"

int main(){
    PPU ppu;

    ppu.cpuWrite(0x2006, 0x3F); // high byte
    ppu.cpuWrite(0x2006, 0x00); // low byte

    ppu.cpuWrite(0x2007, 0x1A); // Write color 0x1A to Palette 0x3F00

    ppu.cpuWrite(0x2006, 0x3F);
    ppu.cpuWrite(0x2006, 0x00);

    uint8_t color = ppu.cpuRead(0x2007);
    if(color == 0x1A){
        std::cout << "PASSED!\n";
    }else{
        std::cerr << "FAILED! Got the color : " << color << " instead\n";
    }

}
