#include "cpu.h"
#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include "bus.h"

enum FLAGS {
    C = 1 << 0, // Carry
    Z = 1 << 1, // Zero
    I = 1 << 2, // Interrupt Disable
    D = 1 << 3, // Decimal (Not used on NES)
    B = 1 << 4, // Break
    U = 1 << 5, // Unused (Always 1)
    V = 1 << 6, // Overflow
    N = 1 << 7  // Negative
};

CPU6502::CPU6502(){

}

void CPU6502::Reset(){
    A = 0;
    X = 0;
    Y = 0;
    SP = 0xFD;      // The stack lives in memory page 1 (`0x0100` - `0x01FF`).
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
    // if cycle is 0 => previous instruction is finished
    if(cycles == 0){
        uint8_t opcode = bus->cpuRead(PC++);
        cycles = Execute(opcode);
    }
    --cycles;
    ++total_cycles;
}

// Returns the CPU cycles it will take to run
uint8_t CPU6502::Execute(uint8_t opcode){

    // addressing mode helper functions
    auto READ_PC_8 = [&](){
        return bus->cpuRead(PC++);
    };
    auto READ_PC_16 = [&](){
        uint16_t lo = bus->cpuRead(PC++);
        uint16_t hi = bus->cpuRead(PC++);
        return (uint16_t)((hi << 8) | lo);
    };

    auto ADDR_IMM   = [&]() { return PC++; }; // Immediate: Data at PC
    // zero page modes
    auto ADDR_ZP    = [&]() { return (uint16_t)READ_PC_8(); };   //
    auto ADDR_ZPX   = [&]() { return (uint16_t)((READ_PC_8() + X) & 0xFF); };
    auto ADDR_ZPY   = [&]() { return (uint16_t)((READ_PC_8() + Y) & 0xFF); };
    // absolute modes
    auto ADDR_ABS   = [&]() { return READ_PC_16(); };
    auto ADDR_ABSX  = [&]() { return (uint16_t)(READ_PC_16() + X); };
    auto ADDR_ABSY  = [&]() { return (uint16_t)(READ_PC_16() + Y); };
    // Indexed Indirect
    auto ADDR_INDX = [&]() {
        uint8_t ptr = (READ_PC_8() + X) & 0xFF;
        uint16_t lo = bus->cpuRead(ptr);
        uint16_t hi = bus->cpuRead((ptr + 1) & 0xFF);
        return (uint16_t)((hi << 8) | lo);
    };
    // Indirect Indexed
    auto ADDR_INDY = [&]() {
        uint8_t ptr = READ_PC_8();
        uint16_t lo = bus->cpuRead(ptr);
        uint16_t hi = bus->cpuRead((ptr + 1) & 0xFF);
        return (uint16_t)(((hi << 8) | lo) + Y);
    };

    /* OPERATION helpers */
    auto UPDATE_ZN = [&](uint8_t res) {
        SetFlag(Z, res == 0);
        SetFlag(N, res & 0x80);
    };

    auto OP_LD = [&](uint8_t &reg, uint16_t addr){
        reg = bus->cpuRead(addr);
        UPDATE_ZN(reg);
    };

    auto OP_ST = [&](uint8_t&reg, uint16_t addr){
        bus->cpuWrite(addr, reg);
    };

    auto OP_ADC = [&](uint16_t addr){
        uint8_t val = bus->cpuRead(addr);
        uint16_t sum = A + val + GetFlag(C);
        SetFlag(C, sum > 0xFF);
        SetFlag(Z, (sum & 0xFF) == 0);
        SetFlag(V, (~(A ^ val) & (A ^ sum)) & 0x80);
        SetFlag(N, sum & 0x80);
        A = sum & 0xFF;
    };

    auto OP_SBC = [&](uint16_t addr){
        uint8_t tval = bus->cpuRead(addr);
        uint16_t val = tval ^ 0xFF;
        uint16_t sum = A + val + GetFlag(C);
        SetFlag(C, sum > 0xFF);
        SetFlag(Z, (sum & 0xFF) == 0);
        SetFlag(V, (sum ^ A) & (sum ^ val) & 0x80);
        SetFlag(N, sum & 0x80);
        A = sum & 0xFF;
    };

    auto OP_AND = [&](uint16_t addr){
       A &= bus->cpuRead(addr);
       UPDATE_ZN(A);
    };
    auto OP_ORA = [&](uint16_t addr){
        A |= bus->cpuRead(addr);
        UPDATE_ZN(A);
    };
    auto OP_EOR = [&](uint16_t addr){
        A ^= bus->cpuRead(addr);
        UPDATE_ZN(A);
    };

    auto OP_CMP = [&](uint8_t reg, uint16_t addr){
        uint8_t val = bus->cpuRead(addr);
        SetFlag(C, reg >= val);
        SetFlag(Z, reg == val);
        SetFlag(N, (reg - val) & 0x80);
    };

    auto OP_MEM_MOD = [&](uint16_t addr, int change){
        uint8_t val = bus->cpuRead(addr);
        val += change;
        bus->cpuWrite(addr, val);
        UPDATE_ZN(val);
    };

    auto OP_ASL = [&](uint16_t addr, bool modeAcc) {
        uint8_t val = modeAcc ? A : bus->cpuRead(addr);
        SetFlag(C, val & 0x80);
        val <<= 1;
        UPDATE_ZN(val);
        if (modeAcc) A = val; else bus->cpuWrite(addr, val);
    };

    auto OP_LSR = [&](uint16_t addr, bool modeAcc) {
        uint8_t val = modeAcc ? A : bus->cpuRead(addr);
        SetFlag(C, val & 0x01);
        val >>= 1;
        UPDATE_ZN(val);
        if (modeAcc) A = val; else bus->cpuWrite(addr, val);
    };

    auto OP_ROL = [&](uint16_t addr, bool modeAcc) {
        uint8_t val = modeAcc ? A : bus->cpuRead(addr);
        uint8_t oldC = GetFlag(C) ? 1 : 0;
        SetFlag(C, val & 0x80);
        val = (val << 1) | oldC;
        UPDATE_ZN(val);
        if (modeAcc) A = val; else bus->cpuWrite(addr, val);
    };

    auto OP_ROR = [&](uint16_t addr, bool modeAcc) {
        uint8_t val = modeAcc ? A : bus->cpuRead(addr);
        uint8_t oldC = GetFlag(C) ? 0x80 : 0;
        SetFlag(C, val & 0x01);
        val = (val >> 1) | oldC;
        UPDATE_ZN(val);
        if (modeAcc) A = val; else bus->cpuWrite(addr, val);
    };

    auto OP_BRANCH = [&](bool condition) {
        int8_t offset = (int8_t)READ_PC_8();
        uint8_t cycles_taken = 2;
        if (condition) {
            cycles_taken++;
            uint16_t target = PC + offset;
            if ((target & 0xFF00) != (PC & 0xFF00)) cycles_taken++; // page crossed
            PC = target;
        }
        return cycles_taken;
    };

    auto PUSH = [&](uint8_t val){
        bus->cpuWrite(0x0100 + SP--, val);
    };
    auto PULL = [&](){
        return bus->cpuRead(0x0100 + ++SP);
    };

    auto PAGE_CROSS = [](uint16_t a, uint16_t b) {  // check for page boundary crossing
        return (a & 0xFF00) != (b & 0xFF00);
    };

    switch(opcode) {

        // NOP
        case 0xEA: return 2;

        // LOAD (LDA, LDX, LDY)
        case 0xA9: OP_LD(A, ADDR_IMM());  return 2;
        case 0xA5: OP_LD(A, ADDR_ZP());   return 3;
        case 0xB5: OP_LD(A, ADDR_ZPX());  return 4;
        case 0xAD: OP_LD(A, ADDR_ABS());  return 4;
        case 0xBD: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + X; OP_LD(A, target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0xB9: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + Y; OP_LD(A, target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0xA1: OP_LD(A, ADDR_INDX()); return 6;
        case 0xB1: {
            uint8_t ptr = READ_PC_8();
            uint16_t base = bus->cpuRead(ptr) | (bus->cpuRead((ptr + 1) & 0xFF) << 8);
            uint16_t target = base + Y; OP_LD(A, target);
            return 5 + PAGE_CROSS(base, target);
        }

        case 0xA2: OP_LD(X, ADDR_IMM());  return 2;
        case 0xA6: OP_LD(X, ADDR_ZP());   return 3;
        case 0xB6: OP_LD(X, ADDR_ZPY());  return 4;
        case 0xAE: OP_LD(X, ADDR_ABS());  return 4;
        case 0xBE: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + Y; OP_LD(X, target);
            return 4 + PAGE_CROSS(addr, target);
        }

        case 0xA0: OP_LD(Y, ADDR_IMM());  return 2;
        case 0xA4: OP_LD(Y, ADDR_ZP());   return 3;
        case 0xB4: OP_LD(Y, ADDR_ZPX());  return 4;
        case 0xAC: OP_LD(Y, ADDR_ABS());  return 4;
        case 0xBC: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + X; OP_LD(Y, target);
            return 4 + PAGE_CROSS(addr, target);
        }

        // STORE (STA, STX, STY)
        case 0x85: OP_ST(A, ADDR_ZP());   return 3;
        case 0x95: OP_ST(A, ADDR_ZPX());  return 4;
        case 0x8D: OP_ST(A, ADDR_ABS());  return 4;
        case 0x9D: OP_ST(A, ADDR_ABSX()); return 5;
        case 0x99: OP_ST(A, ADDR_ABSY()); return 5;
        case 0x81: OP_ST(A, ADDR_INDX()); return 6;
        case 0x91: OP_ST(A, ADDR_INDY()); return 6;

        case 0x86: OP_ST(X, ADDR_ZP());   return 3;
        case 0x96: OP_ST(X, ADDR_ZPY());  return 4;
        case 0x8E: OP_ST(X, ADDR_ABS());  return 4;

        case 0x84: OP_ST(Y, ADDR_ZP());   return 3;
        case 0x94: OP_ST(Y, ADDR_ZPX());  return 4;
        case 0x8C: OP_ST(Y, ADDR_ABS());  return 4;

        // TRANSFERS
        case 0xAA: X = A; UPDATE_ZN(X); return 2;
        case 0xA8: Y = A; UPDATE_ZN(Y); return 2;
        case 0x8A: A = X; UPDATE_ZN(A); return 2;
        case 0x98: A = Y; UPDATE_ZN(A); return 2;
        case 0x9A: SP = X;              return 2;
        case 0xBA: X = SP; UPDATE_ZN(X);return 2;

        // ARITHMETIC (ADC / SBC)
        case 0x69: OP_ADC(ADDR_IMM());  return 2;
        case 0x65: OP_ADC(ADDR_ZP());   return 3;
        case 0x75: OP_ADC(ADDR_ZPX());  return 4;
        case 0x6D: OP_ADC(ADDR_ABS());  return 4;
        case 0x7D: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + X; OP_ADC(target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0x79: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + Y;
            OP_ADC(target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0x61: OP_ADC(ADDR_INDX()); return 6;
        case 0x71: {
            uint8_t ptr = READ_PC_8();
            uint16_t base = bus->cpuRead(ptr) | (bus->cpuRead((ptr + 1) & 0xFF) << 8);
            uint16_t target = base + Y;
            OP_ADC(target);
            return 5 + PAGE_CROSS(base, target);
        }

        case 0xE9: OP_SBC(ADDR_IMM());  return 2;
        case 0xE5: OP_SBC(ADDR_ZP());   return 3;
        case 0xF5: OP_SBC(ADDR_ZPX());  return 4;
        case 0xED: OP_SBC(ADDR_ABS());  return 4;
        case 0xFD: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + X; OP_SBC(target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0xF9: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + Y; OP_SBC(target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0xE1: OP_SBC(ADDR_INDX()); return 6;
        case 0xF1: {
            uint8_t ptr = READ_PC_8();
            uint16_t base = bus->cpuRead(ptr) | (bus->cpuRead((ptr + 1) & 0xFF) << 8);
            uint16_t target = base + Y;
            OP_SBC(target);
            return 5 + PAGE_CROSS(base, target);
        }

        // LOGICAL (AND, ORA, EOR, BIT)
        case 0x29: OP_AND(ADDR_IMM());  return 2;
        case 0x25: OP_AND(ADDR_ZP());   return 3;
        case 0x35: OP_AND(ADDR_ZPX());  return 4;
        case 0x2D: OP_AND(ADDR_ABS());  return 4;
        case 0x3D: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + X;
            OP_AND(target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0x39: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + Y;
            OP_AND(target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0x21: OP_AND(ADDR_INDX()); return 6;
        case 0x31: {
            uint8_t ptr = READ_PC_8();
            uint16_t base = bus->cpuRead(ptr) | (bus->cpuRead((ptr + 1) & 0xFF) << 8);
            uint16_t target = base + Y;
            OP_AND(target);
            return 5 + PAGE_CROSS(base, target);
        }

        case 0x09: OP_ORA(ADDR_IMM());  return 2;
        case 0x05: OP_ORA(ADDR_ZP());   return 3;
        case 0x15: OP_ORA(ADDR_ZPX());  return 4;
        case 0x0D: OP_ORA(ADDR_ABS());  return 4;
        case 0x1D: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + X;
            OP_ORA(target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0x19: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + Y;
            OP_ORA(target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0x01: OP_ORA(ADDR_INDX()); return 6;
        case 0x11: {
            uint8_t ptr = READ_PC_8();
            uint16_t base = bus->cpuRead(ptr) | (bus->cpuRead((ptr + 1) & 0xFF) << 8);
            uint16_t target = base + Y;
            OP_ORA(target);
            return 5 + PAGE_CROSS(base, target);
        }

        case 0x49: OP_EOR(ADDR_IMM());  return 2;
        case 0x45: OP_EOR(ADDR_ZP());   return 3;
        case 0x55: OP_EOR(ADDR_ZPX());  return 4;
        case 0x4D: OP_EOR(ADDR_ABS());  return 4;
        case 0x5D: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + X;
            OP_EOR(target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0x59: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + Y;
            OP_EOR(target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0x41: OP_EOR(ADDR_INDX()); return 6;
        case 0x51: {
            uint8_t ptr = READ_PC_8();
            uint16_t base = bus->cpuRead(ptr) | (bus->cpuRead((ptr + 1) & 0xFF) << 8);
            uint16_t target = base + Y;
            OP_EOR(target);
            return 5 + PAGE_CROSS(base, target);
        }

        case 0x24: { // BIT ZP
            uint8_t val = bus->cpuRead(ADDR_ZP());
            SetFlag(Z, (A & val) == 0); SetFlag(N, val & 0x80); SetFlag(V, val & 0x40);
            return 3;
        }
        case 0x2C: { // BIT ABS
            uint8_t val = bus->cpuRead(ADDR_ABS());
            SetFlag(Z, (A & val) == 0); SetFlag(N, val & 0x80); SetFlag(V, val & 0x40);
            return 4;
        }

        // INC / DEC
        case 0xE6: OP_MEM_MOD(ADDR_ZP(), 1);   return 5;
        case 0xF6: OP_MEM_MOD(ADDR_ZPX(), 1);  return 6;
        case 0xEE: OP_MEM_MOD(ADDR_ABS(), 1);  return 6;
        case 0xFE: OP_MEM_MOD(ADDR_ABSX(), 1); return 7;
        case 0xC6: OP_MEM_MOD(ADDR_ZP(), -1);  return 5;
        case 0xD6: OP_MEM_MOD(ADDR_ZPX(), -1); return 6;
        case 0xCE: OP_MEM_MOD(ADDR_ABS(), -1); return 6;
        case 0xDE: OP_MEM_MOD(ADDR_ABSX(), -1);return 7;

        case 0xE8: X++; UPDATE_ZN(X); return 2;
        case 0xC8: Y++; UPDATE_ZN(Y); return 2;
        case 0xCA: X--; UPDATE_ZN(X); return 2;
        case 0x88: Y--; UPDATE_ZN(Y); return 2;

        // SHIFTS (ASL, LSR, ROL, ROR)
        case 0x0A: OP_ASL(0, true);        return 2;
        case 0x06: OP_ASL(ADDR_ZP(), false);   return 5;
        case 0x16: OP_ASL(ADDR_ZPX(), false);  return 6;
        case 0x0E: OP_ASL(ADDR_ABS(), false);  return 6;
        case 0x1E: OP_ASL(ADDR_ABSX(), false); return 7;

        case 0x4A: OP_LSR(0, true);        return 2;
        case 0x46: OP_LSR(ADDR_ZP(), false);   return 5;
        case 0x56: OP_LSR(ADDR_ZPX(), false);  return 6;
        case 0x4E: OP_LSR(ADDR_ABS(), false);  return 6;
        case 0x5E: OP_LSR(ADDR_ABSX(), false); return 7;

        case 0x2A: OP_ROL(0, true);        return 2;
        case 0x26: OP_ROL(ADDR_ZP(), false);   return 5;
        case 0x36: OP_ROL(ADDR_ZPX(), false);  return 6;
        case 0x2E: OP_ROL(ADDR_ABS(), false);  return 6;
        case 0x3E: OP_ROL(ADDR_ABSX(), false); return 7;

        case 0x6A: OP_ROR(0, true);        return 2;
        case 0x66: OP_ROR(ADDR_ZP(), false);   return 5;
        case 0x76: OP_ROR(ADDR_ZPX(), false);  return 6;
        case 0x6E: OP_ROR(ADDR_ABS(), false);  return 6;
        case 0x7E: OP_ROR(ADDR_ABSX(), false); return 7;

        // COMPARE (CMP, CPX, CPY)
        case 0xC9: OP_CMP(A, ADDR_IMM());  return 2;
        case 0xC5: OP_CMP(A, ADDR_ZP());   return 3;
        case 0xD5: OP_CMP(A, ADDR_ZPX());  return 4;
        case 0xCD: OP_CMP(A, ADDR_ABS());  return 4;
        case 0xDD: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + X;
            OP_CMP(A, target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0xD9: {
            uint16_t addr = ADDR_ABS();
            uint16_t target = addr + Y;
            OP_CMP(A, target);
            return 4 + PAGE_CROSS(addr, target);
        }
        case 0xC1: OP_CMP(A, ADDR_INDX()); return 6;
        case 0xD1: {
            uint8_t ptr = READ_PC_8();
            uint16_t base = bus->cpuRead(ptr) | (bus->cpuRead((ptr + 1) & 0xFF) << 8);
            uint16_t target = base + Y;
            OP_CMP(A, target);
            return 5 + PAGE_CROSS(base, target);
        }

        case 0xE0: OP_CMP(X, ADDR_IMM()); return 2;
        case 0xE4: OP_CMP(X, ADDR_ZP());  return 3;
        case 0xEC: OP_CMP(X, ADDR_ABS()); return 4;

        case 0xC0: OP_CMP(Y, ADDR_IMM()); return 2;
        case 0xC4: OP_CMP(Y, ADDR_ZP());  return 3;
        case 0xCC: OP_CMP(Y, ADDR_ABS()); return 4;

        // BRANCHES
        case 0x90: return OP_BRANCH(!GetFlag(C));
        case 0xB0: return OP_BRANCH(GetFlag(C));
        case 0xF0: return OP_BRANCH(GetFlag(Z));
        case 0xD0: return OP_BRANCH(!GetFlag(Z));
        case 0x30: return OP_BRANCH(GetFlag(N));
        case 0x10: return OP_BRANCH(!GetFlag(N));
        case 0x50: return OP_BRANCH(!GetFlag(V));
        case 0x70: return OP_BRANCH(GetFlag(V));

        // STACK
        case 0x48: PUSH(A); return 3;
        case 0x08: PUSH(P | 0x30); return 3;
        case 0x68: A = PULL(); UPDATE_ZN(A); return 4;
        case 0x28: P = PULL(); P |= U; return 4;

        // JUMPS & SUBROUTINES
        case 0x4C: { // JMP ABS
            PC = ADDR_ABS();
            return 3;
        }
        case 0x6C: { // JMP INDIRECT (Buggy on 6502)
            uint16_t ptr = ADDR_ABS();
            uint16_t lo = bus->cpuRead(ptr);
            uint16_t hi = bus->cpuRead((ptr & 0xFF00) | ((ptr + 1) & 0x00FF));
            PC = (hi << 8) | lo;
            return 5;
        }
        case 0x20: { // JSR
            uint16_t target = ADDR_ABS();
            uint16_t ret = PC - 1;
            PUSH((ret >> 8) & 0xFF);
            PUSH(ret & 0xFF);
            PC = target;
            return 6;
        }
        case 0x60: { // RTS
            uint16_t lo = PULL();
            uint16_t hi = PULL();
            PC = ((hi << 8) | lo) + 1;
            return 6;
        }
        case 0x40: { // RTI
            P = PULL(); P |= U;
            uint16_t lo = PULL();
            uint16_t hi = PULL();
            PC = (hi << 8) | lo;
            return 6;
        }

        // FLAGS & SYSTEM
        case 0x18: SetFlag(C, false); return 2;
        case 0x38: SetFlag(C, true);  return 2;
        case 0x58: SetFlag(I, false); return 2;
        case 0x78: SetFlag(I, true);  return 2;
        case 0xB8: SetFlag(V, false); return 2;
        case 0xD8: SetFlag(D, false); return 2;
        case 0xF8: SetFlag(D, true);  return 2;

        // break
        case 0x00: {
            PC++;
            PUSH((PC >> 8) & 0xFF);
            PUSH(PC & 0xFF);
            PUSH(P | B | U);
            SetFlag(I, true); // BRK sets Interrupt disable
            uint16_t lo = bus->cpuRead(0xFFFE);
            uint16_t hi = bus->cpuRead(0xFFFF);
            PC = (hi << 8) | lo;
            std::cout << "BRK reached\n";
            return 7;
        }

        default:
            std::cerr << "Unknown opcode\n";
            return 2;
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
