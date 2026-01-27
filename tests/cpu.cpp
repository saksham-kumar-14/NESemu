#include "../src/cpu.h"
#include "../src/bus.h"
#include <cstdint>
#include <sys/types.h>
#include <cassert>

int main(){
    Bus bus;

    std::vector<uint8_t> program = {
        0xA9, 0x05, // LDA
        0xE8,       // INX
        0xE8,       // INX
        0x00        // BRK
    };
    /*
      Expected
      A = 5 (loaded 5 to A)
      X = 2 (INX 2 times)
     */

    bus.cpu.LoadProgram(program, 0x0000);
    bus.cpu.Reset();
    bus.cpu.Run();

    assert(bus.cpu.A == 5);
    assert(bus.cpu.X == 2);
    std::cout << "TEST Passed\n" << '\n';

    // for CPU cycles (test 2)
    std::vector<uint8_t> program2 = {
        0x18,
        0xA9,
        0x7F,
        0x69,
        0x01,
        0x00
    };
    bus.cpu.LoadProgram(program2, 0x8000);
    bus.cpu.Reset();
    bus.cpu.PC = 0x8000;
    bus.cpu.Run();      // should run until BRK is reached


    return 0;
}
