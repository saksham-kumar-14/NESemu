#include "bus.h"
#include <cstdint>
#include <iostream>


void Bus::cpuWrite(uint16_t addr, uint8_t val){
    if(addr < 0x2000){
        RAM[addr % 2048] = val;
    }else if(addr >= 0x8000){
        if (cart){
            std::cerr << "Write to PRG ROM ignored. (It is read-only) \n";
        }
    }else{
        // PPU reg will go here
    }
}

uint8_t Bus::cpuRead(uint16_t addr){
    if(addr < 0x2000){
        return RAM[addr % 2048];
    }else if(addr >= 0x8000){
        if(cart){
            uint8_t PRG_mapping_addr = addr - 0x8000;
            if(cart->PRGMemory.size() == 16384) {
                PRG_mapping_addr %= 16384;
            }
            return cart->PRGMemory[PRG_mapping_addr];
        }
    }

    // else
    // PPu reg
    return 0;
}
