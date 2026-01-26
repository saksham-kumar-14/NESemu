#include "../src/cpu.h"
#include <cstdint>
#include <sys/types.h>

int main(){
    Bus bus;

    std::vector<uint8_t> program = {
        0xA9, 0x05, // LDA #$05
        0xE8,       // INX
        0xE8,       // INX
        0x00        // BRK
    };

    bus.cpu.LoadProgram(program, 0x0000);
    bus.cpu.Reset();
    bus.cpu.Run();

    std::cout << "A = " << (int)bus.cpu.A << "\n";
    std::cout << "X = " << (int)bus.cpu.X << "\n";
    return 0;
}


/*
  Expected
  A = 5 (loaded 5 to A)
  X = 2 (INX 2 times)
 */
