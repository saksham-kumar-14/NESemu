// tests.cpp
#include "../src/mapper_000.h"
#include "../src/bus.h"
#include "../src/cpu.h"
#include <cassert>
#include <iostream>

void testMapper32KB() {
    // 2 PRG Banks = 32KB, 1 CHR Bank
    Mapper_000 mapper(2, 1);
    uint32_t mapped_addr = 0;

    // Lowest valid address maps to index 0
    assert(mapper.cpuMapRead(0x8000, mapped_addr) == true);
    assert(mapped_addr == 0x0000);

    // Highest valid address maps to index 0x7FFF (32KB max)
    assert(mapper.cpuMapRead(0xFFFF, mapped_addr) == true);
    assert(mapped_addr == 0x7FFF);

    // Out of bounds address (e.g., PPU registers) should return false
    assert(mapper.cpuMapRead(0x2000, mapped_addr) == false);

    std::cout << ">> Mapper_000 32KB PRG tests passed!\n";
}

void testMapper16KBMirroring() {
    // 1 PRG Bank = 16KB, 1 CHR Bank
    Mapper_000 mapper(1, 1);
    uint32_t mapped_addr = 0;

    // 0x8000 maps to 0
    assert(mapper.cpuMapRead(0x8000, mapped_addr) == true);
    assert(mapped_addr == 0x0000);

    // 0xC000 should wrap around and point back to index 0 due to 16KB mirroring!
    assert(mapper.cpuMapRead(0xC000, mapped_addr) == true);
    assert(mapped_addr == 0x0000);

    // 0xFFFF should wrap around to 0x3FFF
    assert(mapper.cpuMapRead(0xFFFF, mapped_addr) == true);
    assert(mapped_addr == 0x3FFF);

    std::cout << ">> Mapper_000 16KB Mirroring tests passed!\n";
}

void testCPUDebugString() {
    CPU6502 cpu;

    cpu.PC = 0xC000;
    cpu.A  = 0xAA;
    cpu.X  = 0x01;
    cpu.Y  = 0x02;
    cpu.P  = 0x24; // 0010 0100
    cpu.SP = 0xFD;

    // The output must exactly match the nestest.log format
    assert(cpu.GetDebugString() == "C000  A:AA X:01 Y:02 P:24 SP:FD");

    std::cout << ">> CPU Diagnostic String formatting passed!\n";
}

void testCPUInstruction_LDA() {
    Bus bus;
    // Fake a tiny program in RAM
    // 0xA9 is 'LDA Immediate', followed by the value '0x00'
    bus.cpuWrite(0x0000, 0xA9);
    bus.cpuWrite(0x0001, 0x00);

    bus.cpu.PC = 0x0000;

    // Execute 1 instruction (LDA #$00)
    bus.cpu.Clock();

    // Accumulator should now be 0x00
    assert(bus.cpu.A == 0x00);

    // Because we loaded 0x00, the Zero flag (bit 1) should be true
    assert(bus.cpu.GetFlag(1 << 1) == true);

    // The Negative flag (bit 7) should be false
    assert(bus.cpu.GetFlag(1 << 7) == false);

    std::cout << ">> CPU Instruction logic (LDA Immediate) passed!\n";
}

int main() {
    testMapper32KB();
    testMapper16KBMirroring();
    testCPUDebugString();
    testCPUInstruction_LDA();

    std::cout << "Mapper 000 Tests passed successfully!" << std::endl;
    return 0;
}
