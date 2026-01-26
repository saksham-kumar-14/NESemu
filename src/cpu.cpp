#include "cpu.h"
#include <cstdint>
#include<iostream>
#include <sys/types.h>
#include "bus.h"

enum FLAGS {
    C = 1 << 0, Z = 1 << 1, I = 1 << 2, D = 1 << 3,
    B = 1 << 4, U = 1 << 5, V = 1 << 6, N = 1 << 7
};

CPU6502::CPU6502(){

}

void CPU6502::Reset(){
    A = 0;
    X = 0;
    Y = 0;
    SP = 0xFD;      // The stack lives in memory page 1 (`0x0100` - `0x01FF`).
    P = 0x24;

    uint16_t lo = bus->cpuRead(0xFFFC);
    uint16_t hi = bus->cpuRead(0xFFFD);
    PC = (hi << 8) | lo;    // Little endian
}

void CPU6502::LoadProgram(const std::vector<uint8_t>& program, uint16_t startAddr){
    for(int i = 0; i < program.size(); ++i){
        bus->cpuWrite(startAddr + i, program[i]);
    }
    PC = startAddr;
}

void CPU6502::SetFlag(uint8_t bit, bool value){
    if (value){
        P |= bit;
    } else {
        P &= ~bit;
    }
}

bool CPU6502::GetFlag(uint8_t bit){
    return (P & bit)  != 0;
}

void CPU6502::Clock(){
    uint8_t opcode = bus->cpuRead(PC++);
    // std::cout << std::hex << "PC=" << PC << " OPCODE=" << (int)opcode << "\n";
    Execute(opcode);
}

void CPU6502::Execute(uint8_t opcode){
    switch(opcode) {

        // LDA
        case 0xA9: {
            uint8_t value = bus->cpuRead(PC++);
            A = value;
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }
        case 0xE8: {
            X++;
            SetFlag(Z, X == 0);
            SetFlag(N, X & 0x80);
            break;
        }

        // break
        case 0x00: {
            SetFlag(B, true);
            std::cout << "BRK reached\n";
            break;
        }

        default:
            std::cerr << "Unknown opcode\n";
            break;
    }
}

void CPU6502::Run(){
    while(1){
        uint8_t opcode = bus->cpuRead(PC);
        if (opcode == 0x00){
            Clock();
            break;
        }
        Clock();
    }
}
