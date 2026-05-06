#pragma once
#include <cstdint>
#include <vector>
#include <iostream>

class Bus;

class CPU6502 {
public:
    uint8_t A = 0x00;
    uint8_t X = 0x00;
    uint8_t Y = 0x00;
    uint8_t SP = 0xFD;
    uint16_t PC = 0x0000;
    uint8_t P = 0x24;

    CPU6502();

    void Reset();
    void NMI();
    void IRQ();
    void LoadProgram(const std::vector<uint8_t>& program, uint16_t startAddr);
    void Clock();
    void Run();

    void integrateBus(Bus* b){
        bus = b;
    }

    bool GetFlag(uint8_t bit);

    // clock cycles
    uint8_t cycles = 0;         // The number of cycles remaining for current instruction
    uint32_t total_cycles = 0;  // for debugging purpose mostly

    std::string GetDebugString();   // for diagnostic logging

private:
    uint8_t Execute(uint8_t opcode);
    void SetFlag(uint8_t bit, bool value);

    Bus* bus = nullptr;
};
