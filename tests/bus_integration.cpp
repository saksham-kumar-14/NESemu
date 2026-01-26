#include "../src/bus.h"
#include <iostream>
#include <cassert>
#include <vector>

int main() {
    Bus bus;

    // program: LDA #$42, INX, BRK
    std::vector<uint8_t> program = { 0xA9, 0x42, 0xE8, 0x00 };

    bus.cpu.LoadProgram(program, 0x0000);
    bus.cpu.Reset();

    std::cout << "Starting CPU Run...\n";
    bus.cpu.Run();

    assert(bus.cpu.A == 0x42);
    assert(bus.cpu.X == 0x01);

    std::cout << ">> CPU+Bus test passed! A=0x"
              << std::hex << (int)bus.cpu.A
              << " X=0x" << (int)bus.cpu.X << "\n";

    return 0;
}
