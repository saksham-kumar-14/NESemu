#include "../src/bus.h"
#include "../src/cpu.h"
#include "../src/cartridge.h"
#include <iostream>
#include <vector>

int main() {
    Bus bus;

    Cartridge cart("HelloWorld.nes");
    cart.romLoad = true;
    cart.PRGMemory.resize(0x8000, 0xEA); // Fill with NOPs
    bus.ConnectCartridge(&cart);

    std::vector<uint8_t> program = {
        0xA9, 0x05,
        0xE8,
        0xE8,
        0x00
    };

    bus.cpu.LoadProgram(program, 0x8000);

    bus.cpuWrite(0xFFFC, 0x00);
    bus.cpuWrite(0xFFFD, 0x80);

    bus.cpu.Reset();
    bus.cpu.Run();

    std::cout << "A = " << (int)bus.cpu.A << "\n";
    std::cout << "X = " << (int)bus.cpu.X << "\n";
}
