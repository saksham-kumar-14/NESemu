#include "cpu.h"
#include <cstdint>
#include <ios>
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

    uint8_t rl = bus->cpuRead(0xFFFC);
    uint8_t rh = bus->cpuRead(0xFFFD);
    PC = (rh << 8) | rl;
    std::cout << "Reset vector: lo=" << std::hex << (int)rl << " hi=" << (int)rh << " PC=$"
              << PC << std::dec << "\n";
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
    uint16_t oldPC = PC;
    uint8_t opcode = bus->cpuRead(PC++);
    std::cout << "FETCH OPCODE " << std::hex << (int)opcode
              << " at PC=" << oldPC << std::endl;
    Execute(opcode);
}

void CPU6502::Execute(uint8_t opcode) {
    auto CMP = [&](uint8_t reg, uint8_t val) {
        uint16_t tmp = reg - val;
        SetFlag(C, reg >= val);
        SetFlag(Z, (tmp & 0xFF) == 0);
        SetFlag(N, tmp & 0x80);
    };

    std::cout << "EXECUTING OPCODE " << std::hex << (int)opcode
              << " at PC=" << (int)PC << std::endl;

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

        // LDA zp
        case 0xA5: {
            uint8_t addr = bus->cpuRead(PC++);
            A = bus->cpuRead(addr);
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }

        // LDA abs
        case 0xAD: {
            uint16_t lo = bus->cpuRead(PC++);
            uint16_t hi = bus->cpuRead(PC++);
            uint16_t addr = (hi << 8) | lo;
            A = bus->cpuRead(addr);
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }

        // STA abs
        case 0x8D: {
            uint16_t lo = bus->cpuRead(PC++);
            uint16_t hi = bus->cpuRead(PC++);
            uint16_t addr = (hi << 8) | lo;
            bus->cpuWrite(addr, A);
            break;
        }

        // STA zp
        case 0x85: {
            uint8_t addr = bus->cpuRead(PC++);
            bus->cpuWrite(addr, A);
            break;
        }

        //
        // --- Register Transfers ---
        //
        case 0xAA: X = A; SetFlag(Z, X == 0); SetFlag(N, X & 0x80); break; // TAX
        case 0xA8: Y = A; SetFlag(Z, Y == 0); SetFlag(N, Y & 0x80); break; // TAY
        case 0x8A: A = X; SetFlag(Z, A == 0); SetFlag(N, A & 0x80); break; // TXA
        case 0x98: A = Y; SetFlag(Z, A == 0); SetFlag(N, A & 0x80); break; // TYA

        //
        // --- Arithmetic ---
        //
        case 0x69: { // ADC #imm
            uint8_t value = bus->cpuRead(PC++);
            uint16_t sum = A + value + GetFlag(C);
            SetFlag(C, sum > 0xFF);
            SetFlag(Z, (sum & 0xFF) == 0);
            SetFlag(V, (~(A ^ value) & (A ^ sum) & 0x80));
            SetFlag(N, sum & 0x80);
            A = sum & 0xFF;
            break;
        }

        case 0xE9: { // SBC #imm
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
        case 0x29: { // AND #imm
            uint8_t value = bus->cpuRead(PC++);
            A &= value;
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }

        case 0x09: { // ORA #imm
            uint8_t value = bus->cpuRead(PC++);
            A |= value;
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }

        case 0x49: { // EOR #imm
            uint8_t value = bus->cpuRead(PC++);
            A ^= value;
            SetFlag(Z, A == 0);
            SetFlag(N, A & 0x80);
            break;
        }

        case 0x24: { // BIT zp
            uint8_t addr = bus->cpuRead(PC++);
            uint8_t val = bus->cpuRead(addr);
            SetFlag(Z, (A & val) == 0);
            SetFlag(V, val & 0x40);
            SetFlag(N, val & 0x80);
            break;
        }

        //
        // --- Stack Operations ---
        //
        case 0x48: bus->cpuWrite(0x0100 + SP--, A); break; // PHA
        case 0x68: A = bus->cpuRead(0x0100 + ++SP); SetFlag(Z, A == 0); SetFlag(N, A & 0x80); break; // PLA
        case 0x08: bus->cpuWrite(0x0100 + SP--, P | 0x10); break; // PHP
        case 0x28: P = bus->cpuRead(0x0100 + ++SP); break; // PLP

        //
        // --- Jumps/Subroutines ---
        //
        case 0x4C: { // JMP abs
            uint16_t lo = bus->cpuRead(PC++);
            uint16_t hi = bus->cpuRead(PC++);
            PC = (hi << 8) | lo;
            break;
        }

        case 0x6C: { // JMP indirect
            uint16_t ptrLo = bus->cpuRead(PC++);
            uint16_t ptrHi = bus->cpuRead(PC++);
            uint16_t ptr = (ptrHi << 8) | ptrLo;
            uint8_t lo = bus->cpuRead(ptr);
            uint8_t hi = bus->cpuRead((ptr & 0xFF00) | ((ptr + 1) & 0x00FF)); // page bug
            PC = (hi << 8) | lo;
            break;
        }

        case 0x20: { // JSR
            uint16_t lo = bus->cpuRead(PC++);
            uint16_t hi = bus->cpuRead(PC++);
            uint16_t target = (hi << 8) | lo;
            uint16_t returnAddr = PC - 1;
            bus->cpuWrite(0x0100 + SP--, (returnAddr >> 8) & 0xFF);
            bus->cpuWrite(0x0100 + SP--, returnAddr & 0xFF);
            PC = target;
            break;
        }

        case 0x60: { // RTS
            uint8_t lo = bus->cpuRead(0x0100 + ++SP);
            uint8_t hi = bus->cpuRead(0x0100 + ++SP);
            PC = ((hi << 8) | lo) + 1;
            break;
        }

        //
        // --- Branches ---
        //
        #define BRANCH(cond) { int8_t offset = bus->cpuRead(PC++); if (cond) PC += offset; }

        case 0xF0: BRANCH(GetFlag(Z)); break; // BEQ
        case 0xD0: BRANCH(!GetFlag(Z)); break; // BNE
        case 0x10: BRANCH(!GetFlag(N)); break; // BPL
        case 0x30: BRANCH(GetFlag(N)); break;  // BMI
        case 0xB0: BRANCH(GetFlag(C)); break;  // BCS
        case 0x90: BRANCH(!GetFlag(C)); break; // BCC
        case 0x70: BRANCH(GetFlag(V)); break;  // BVS
        case 0x50: BRANCH(!GetFlag(V)); break; // BVC

        #undef BRANCH

        //
        // --- Compare ---
        //
        case 0xC9: { uint8_t val = bus->cpuRead(PC++); CMP(A, val); break; } // CMP #imm
        case 0xE0: { uint8_t val = bus->cpuRead(PC++); CMP(X, val); break; } // CPX #imm
        case 0xC0: { uint8_t val = bus->cpuRead(PC++); CMP(Y, val); break; } // CPY #imm

        //
        // --- Misc ---
        //
        case 0xEA: break; // NOP
        case 0x00: SetFlag(B, true); std::cout << "BRK reached\n"; break;

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

void CPU6502::NMI(){
    uint16_t lo = bus->cpuRead(0xFFFA);
    uint16_t hi = bus->cpuRead(0xFFFB);
    PC = (hi << 8) | lo;

    SetFlag(I, true);
}
