#include "../src/cpu.h"
#include "../src/bus.h"
#include <cstdint>
#include <cassert>
#include <iostream>
#include <vector>
#include <fstream>

void test_basic() {
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
    assert(bus.cpu.A == 5);
    assert(bus.cpu.X == 2);
    std::cout << "test_basic Passed\n";
}

void create_dummy_rom_for_cpu(const std::string& filename) {
    std::ofstream ofs(filename, std::ios::binary);
    ofs.write("NES\x1A", 4);
    ofs.put(1); // 16KB PRG
    ofs.put(1); // 8KB CHR
    ofs.put(0x00);
    ofs.put(0x00);
    for (int i = 0; i < 8; ++i) ofs.put(0x00);
    for (int i = 0; i < 16 * 1024; ++i) ofs.put(0x00);
    for (int i = 0; i < 8 * 1024; ++i) ofs.put(0xFF);
}

void test_nmi() {
    create_dummy_rom_for_cpu("nmi_dummy.nes");
    Cartridge cart("nmi_dummy.nes");
    Bus bus;
    bus.ConnectCartridge(&cart);
    bus.cpu.Reset();

    // Set NMI vector to 0x8100 directly in ROM memory
    // 0xFFFA maps to 0x7FFA in a 16KB PRG ROM
    cart.PRGMemory[0x7FFA] = 0x00;
    cart.PRGMemory[0x7FFB] = 0x81;

    // Put RTI (0x40) at 0x8100 -> maps to 0x0100 in ROM
    cart.PRGMemory[0x0100] = 0x40;

    bus.cpu.PC = 0x8000;
    bus.cpu.NMI();

    // NMI takes 8 cycles, and PC should now be 0x8100
    assert(bus.cpu.cycles == 8);
    assert(bus.cpu.PC == 0x8100);

    // Clock out the NMI pseudo-instruction cycles
    while (bus.cpu.cycles > 0) {
        bus.cpu.Clock();
    }

    // Now run RTI
    bus.cpu.Clock(); // Fetches RTI
    while (bus.cpu.cycles > 0) {
        bus.cpu.Clock();
    }

    assert(bus.cpu.PC == 0x8000); // Stack pushed 0x8000, should be restored
    std::cout << "test_nmi Passed\n";
    std::remove("nmi_dummy.nes");
}

void test_irq() {
    create_dummy_rom_for_cpu("irq_dummy.nes");
    Cartridge cart("irq_dummy.nes");
    Bus bus;
    bus.ConnectCartridge(&cart);
    bus.cpu.Reset();

    // Clear Interrupt Disable flag (I is bit 2)
    bus.cpu.P &= ~0x04;

    // Set IRQ vector to 0x8200 directly in ROM memory
    // 0xFFFE maps to 0x7FFE in a 16KB PRG ROM
    cart.PRGMemory[0x7FFE] = 0x00;
    cart.PRGMemory[0x7FFF] = 0x82;

    // Put RTI (0x40) at 0x8200 -> maps to 0x0200 in ROM
    cart.PRGMemory[0x0200] = 0x40;

    bus.cpu.PC = 0x8000;
    bus.cpu.IRQ();

    assert(bus.cpu.cycles == 7);
    assert(bus.cpu.PC == 0x8200);

    // Clock out IRQ cycles
    while (bus.cpu.cycles > 0) {
        bus.cpu.Clock();
    }

    // Now run RTI
    bus.cpu.Clock(); // Fetches RTI
    while (bus.cpu.cycles > 0) {
        bus.cpu.Clock();
    }

    assert(bus.cpu.PC == 0x8000);

    std::cout << "test_irq Passed\n";
    std::remove("irq_dummy.nes");
}

int main(){
    test_basic();
    test_nmi();
    test_irq();
    std::cout << "All CPU Tests Passed!\n";
    return 0;
}
