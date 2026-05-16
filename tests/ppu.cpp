#include "../src/ppu.h"
#include "../src/cpu.h"
#include "../src/bus.h"
#include <cstdint>
#include <iostream>

int main() {
    PPU ppu;
    Bus bus;
    ppu.ConnectCPU(&bus.cpu);

    // Test 1: VBLANK flag timing test
    // ppu starts at scanline 0 and cycle 0
    // VBLANK triggers when scanline 241 and cycle 1
    // Each scanline is 341 cycles, so total cycles  = 241 * 341 + 2
    // we need 1 tick for cycle 0 and another for executing cycle 1
    int target_cycles = (241 * 341) + 2;
    for(int i = 0; i < target_cycles; ++i){
        ppu.Clock();
    }

    // VBLANK flag should be 1 (which is bit 7 of 0x0002)
    uint8_t status = ppu.cpuRead(0x0002);
    if((status & 0x80) == 0x80){
        std::cout << "Sucess! VBLANK flag set correctly at scanline 241, cycle 1\n";
    }else{
        std::cerr << "Failed! VBLANK flag not set correctly\n";
        return 1;
    }


    // Test 2: VBLANK flag auto clear
    // we just read flag PPUSTATUS flag in prev test so it should be 0 here
    uint8_t status_cleared = ppu.cpuRead(0x0002);
    if ((status_cleared & 0x80) == 0x00) {
        std::cout << "Sucess! reading PPUSTATUS correctly auto cleared the VBLANK flag\n";
    }else{
        std::cerr << "Failed! VBLANK flag was not cleared after read\n";
        return 1;
    }


    // Test 3: Dynamic NMI Injection
    // fast forward to the end of the frame, then back to the next VBLANK.
    while (!ppu.frame_complete) {
        ppu.Clock();
    }
    ppu.frame_complete = false;

    // fast forward to Scanline 241, Cycle 1 of the NEXT frame
    for (int i = 0; i < target_cycles; i++) {
        ppu.Clock();
    }

    // PPU is at VBLANK now but NMI is off by default
    // We write to PPUCTRL (0x0000) to flip the generate_nmi bit to 1
    // Because we are currently in VBLANK, this should immediately trigger cpu->NMI()
    ppu.cpuWrite(0x0000, 0x80);
    // When CPU6502::NMI() runs, it sets its internal cycles variable to 8 to simulate the interrupt cost
    if(bus.cpu.cycles == 8){
        std::cout << "Success! Dynamic NMI Injection instantly interrupted the CPU\n";
    }else{
        std::cerr << "Failure! Dynamic NMI Injection did not interrput the CPU\n";
        return 1;
    }


    return 0;
}
