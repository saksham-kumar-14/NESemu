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
    SP = 0xFD;
    P = 0x24;

    uint16_t lo = bus->cpuRead(0xFFFC);
    uint16_t hi = bus->cpuRead(0xFFFD);
    PC = (hi << 8) | lo;
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
    std::cout << std::hex << "PC=" << PC << " OPCODE=" << (int)opcode << "\n";
    Execute(opcode);
}

void CPU6502::Execute(uint8_t opcode) {
    switch (opcode) {

        //
        // --- Load/Store Instructions ---
        //

        // LDA #imm
        case 0xA9: {
            uint8_t value = bus->cpuRead(PC++);
            A = value;
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }

        // LDX #imm
        case 0xA2: {
            uint8_t value = bus->cpuRead(PC++);
            X = value;
            SetFlag(Z, X == 0);
            SetFlag(N, X & 0x80);
            break;
        }

        // LDY #imm
        case 0xA0: {
            uint8_t value = bus->cpuRead(PC++);
            Y = value;
            SetFlag(Z, Y == 0);
            SetFlag(N, Y & 0x80);
            break;
        }

        // STA $addr (absolute)
        case 0x8D: {
            uint16_t lo = bus->cpuRead(PC++);
            uint16_t hi = bus->cpuRead(PC++);
            uint16_t addr = (hi << 8) | lo;
            bus->cpuWrite(addr, A);
            break;
        }

        // STX $addr (absolute)
        case 0x8E: {
            uint16_t lo = bus->cpuRead(PC++);
            uint16_t hi = bus->cpuRead(PC++);
            uint16_t addr = (hi << 8) | lo;
            bus->cpuWrite(addr, X);
            break;
        }

        // STY $addr (absolute)
        case 0x8C: {
            uint16_t lo = bus->cpuRead(PC++);
            uint16_t hi = bus->cpuRead(PC++);
            uint16_t addr = (hi << 8) | lo;
            bus->cpuWrite(addr, Y);
            break;
        }

        //
        // --- Register Transfer ---
        //

        // TAX
        case 0xAA: {
            X = A;
            SetFlag(Z, X == 0);
            SetFlag(N, X & 0x80);
            break;
        }

        // TAY
        case 0xA8: {
            Y = A;
            SetFlag(Z, Y == 0);
            SetFlag(N, Y & 0x80);
            break;
        }

        // TXA
        case 0x8A: {
            A = X;
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }

        // TYA
        case 0x98: {
            A = Y;
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }

        //
        // --- Increment / Decrement ---
        //

        // INX
        case 0xE8: {
            X++;
            SetFlag(Z, X == 0);
            SetFlag(N, X & 0x80);
            break;
        }

        // INY
        case 0xC8: {
            Y++;
            SetFlag(Z, Y == 0);
            SetFlag(N, Y & 0x80);
            break;
        }

        // DEX
        case 0xCA: {
            X--;
            SetFlag(Z, X == 0);
            SetFlag(N, X & 0x80);
            break;
        }

        // DEY
        case 0x88: {
            Y--;
            SetFlag(Z, Y == 0);
            SetFlag(N, Y & 0x80);
            break;
        }

        //
        // --- Arithmetic ---
        //

        // ADC #imm
        case 0x69: {
            uint8_t value = bus->cpuRead(PC++);
            uint16_t sum = A + value + GetFlag(C);
            SetFlag(C, sum > 0xFF);
            SetFlag(Z, (sum & 0xFF) == 0);
            SetFlag(V, (~(A ^ value) & (A ^ sum) & 0x80));
            SetFlag(N, sum & 0x80);
            A = sum & 0xFF;
            break;
        }

        // SBC #imm
        case 0xE9: {
            uint8_t value = bus->cpuRead(PC++);
            uint16_t diff = A - value - (1 - GetFlag(C));
            SetFlag(C, diff < 0x100);
            SetFlag(Z, (diff & 0xFF) == 0);
            SetFlag(V, ((A ^ value) & (A ^ diff) & 0x80));
            SetFlag(N, diff & 0x80);
            A = diff & 0xFF;
            break;
        }

        //
        // --- Logical ---
        //

        // AND #imm
        case 0x29: {
            uint8_t value = bus->cpuRead(PC++);
            A &= value;
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }

        // ORA #imm
        case 0x09: {
            uint8_t value = bus->cpuRead(PC++);
            A |= value;
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }

        // EOR #imm
        case 0x49: {
            uint8_t value = bus->cpuRead(PC++);
            A ^= value;
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }

        //
        // --- Control ---
        //

        // NOP
        case 0xEA: {
            break;
        }

        // BRK
        case 0x00: {
            SetFlag(B, true);
            std::cout << "BRK reached\n";
            break;
        }

        // JMP Absolute
        case 0x4C: {
            uint16_t lo = bus->cpuRead(PC++);
            uint16_t hi = bus->cpuRead(PC++);
            PC = (hi << 8) | lo;
            break;
        }

        //
        // --- Default ---
        //

        default:
            std::cerr << "Unknown opcode: 0x" << std::hex << (int)opcode << "\n";
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
