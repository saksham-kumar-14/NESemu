#pragma once
#include <cstdint>
#include <vector>
#include <iostream>

class CPU6502 {
public:
    uint8_t A = 0x00;
    uint8_t X = 0x00;
    uint8_t Y = 0x00;
    uint8_t SP = 0xFD;
    uint16_t PC = 0x0000;
    uint8_t P = 0x24;

    std::vector<uint8_t> memory;

    CPU6502();

    void Reset();
    void LoadProgram(const std::vector<uint8_t>& program, uint16_t startAddr);
    void Clock();
    void Run();

private:
    void Execute(uint8_t opcode);
    bool GetFlag(uint8_t bit);
    void SetFlag(uint8_t bit, bool value);
};
