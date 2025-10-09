#include "../src/cpu.h"
#include <iostream>

int main() {
    CPU6502 cpu;
    cpu.Reset();

    std::vector<uint8_t> program = {
        0xA9, 0x05, // LDA
        0xE8,       // INX
        0xE8,       // INX
        0x00        // BRK
    };

    cpu.LoadProgram(program, 0x8000);
    cpu.Run();

    std::cout << "A = " << (int)cpu.A << "\n";
    std::cout << "X = " << (int)cpu.X << "\n";
}

/*
  Expected
  A = 5 (loaded 5 to A)
  X = 2 (INX 2 times)
 */
