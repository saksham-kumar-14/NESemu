
# Cartridge
Its job is to load ROM along with formatting and organising data such that CPU and PPU can understand

class members-
- `romLoad`: `true` if file opened successfully
- `mapperID` : Genius Genius engineering done here. Tells the emulator which logic to emulate. eg, `Mapper 0` is `NROM`, `Mapper 1` is `MMC1`
- `mirror` : How to screen warps around
- `PRGMemory` : This holds the Game Code
- `CHRMemory` : This holds the Graphics

Procedure
1. Open the ROM file
2. Check if the header (16 bytes from the starting) is equal to official NES header ie `header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A`
3. header[4] tells how much gamecode is stored. 1 unit of Gamecode means 16 KBs
4. header[5] tells how much graphics is stored. 1 unit of Graphics means 8 KBs
5. Lets call header[6] and header[7], Flag6 and Flag7. We can get the mapperID mentioned above using as follow:
```
	Flag 6 : [XXXX (Low) | ....]
    Flag 7 : [XXXX (High)| ....]
    mapperID : [XXXX (High) | XXXX (Low)]
```
6. There is sometimes junk code of 512 bytes at the starting of header. which is called junk code (usually it is there for cheat codes). Bit 2 of Flag 6 tell is junk code is there or not. This junk code is called trainer.
7. `mirror = flag6 & 0x01;` Extract the last bit of flag6 to check mirror
8. Then load PRGMemory and CHRMemory.
9. Set `romLoad` to true

```
[.nes file on disk]

Header (16 bytes)
- "NES" + 0x1A
- Byte 4 - PRG count
- Byte 5 - CHR count
- Flag 6
- Flag 7

Trainer (512 bytes) in some cases iff 2nd byte of flag 6 is 1

PRG ROM (Game logic)
- Game Instruction
- Size: 16 * 1024 * PRG count
  
CHR ROM (Graphics)
- Sprites & Tiles
- Size: 8 * 1024 * CHR count
```

# CPU & BUS
## BUS
Does all the memory management like MMU.
- Memory map handled by Bus:
	- `0x0000 - 0x07FF` : 2KB internal RAM
	- `0x0800 - 0x1FFF` : Mirror of `0x0000 - 0x07FF`
	- `0x2000 - 0x3FFF` : PPU registers (mirrored every 8 bytes)
	- `0x8000 - 0xFFFF` : PRG ROM (from cartridge)
- Process
	- Make `std::array<uint8_t, RAMsize> RAM{};` and `memset` it with zeros
	- Connect cartridge
- `cpuWrite(uint16_t addr, uint8_t data)`
	- If `addr` is less than `0x1FFF`: then assign the provided data in that memory address in RAM, and take `&` with `0x07FF` so that mirroring can be taken care of
	- If memory is addr is `>= 0x0800` then `cart->PRGMemory[addr - 0x0800] = data`
- `cpuRead(uint16_t addr)`
	- If `addr` is less than `0x1FFF`: then return `RAM[addr & 0x07FF]`
	- else `return cart->PRGMemory[addr - 0x0800]`
## CPU
- **Registers** 
	- `A`: Accumulator. Almost all arithmetic operations happens here
	- `X` & `Y`: Used for loops and handling arrays/offsets
	- `SP (Stack Pointer)`: Points at a temporary area (stack) for function execution and stuff
	- `PC (Program Counter)`: Points to the memory address of the next instruction to be executed. It from from `0x0000` to `0xFFFF`
	- `P (Status Register)` : A collection of single 8 bit flags (Zero, carry, negative) that tells the CPU of the last instruction
	- `cycles` : Cycles for the current instruction. If cycles is 0 then only next instruction can be executed otherwise hell nah.
	- `total_cycles` : for debugging purposes 
	- *Note that only PC is `uint16_t`, others are `uint8_t`*
- `Reset()`: 
```c++
	A = X = Y = 0;  // clean registers
	SP = 0xFD;     // The stack lives in memory page 1 (`0x0100` - `0x01FF`). 
					// SP is not placed at 0x01FF due to some NES quirks
	P = 0x24;      // Binary: 0010 0100
				   // Bit 2 is set 1 to disable interrupts at startup
				   // Bit 5 is technically "unsed",
				   // it is physically wired to be inside CPU 6502

	uint16_t PC = (bus->cpuRead(0xFFFD) << 8) | bus->cpuRead(0xFFFC)  // It is little-endian so need to do that
```
- `LoadProgram(vector<uint8_t> & program, uint16_t startAddr)`: doing `bus->cpuWrite(startAddr, program)` the whole program
- `Clock()`: Fetch Opcode using `bus->cpuRead(PC)` and execute this opcode using `CPU::Execute()`
- `Run()`:  Endless game loop
- `integrateBus()`
- `getFlag(uint8_t bit)`: `return (P & bit)`
- `Execute(uint8_t opcode)`: opcode switch statement goes here
- `SetFlag(uint8_t bit, bool value)`: 
```c++
	if(value){
		P |= bit; // Turn ON
	}else{
		P &= ~bit; // Turn OFF
	}	
```

# 

# Tests

### Cartridge Test
```c++
#include <iostream>
#include "../src/cartridge.h"

int main() {
    Cartridge cart("HelloWorld.nes");

    if (!cart.romLoad) {
        std::cerr << "Failed to load cartridge\n";
        return 1;
    }

    std::cout << "ROM Loaded Successfully!\n";
    std::cout << "Mapper: " << (int)cart.mapperID << "\n";
    std::cout << "Mirror: " << (cart.mirror ? "Vertical" : "Horizontal") << "\n";
    std::cout << "PRG ROM: " << cart.PRGMemory.size() / 1024 << " KB\n";
    std::cout << "CHR ROM: " << cart.CHRMemory.size() / 1024 << " KB\n";
}
```

### CPU Test
```c++
std::vector<uint8_t> program = {
	0xA9, 0x05, // LDA #$05
	0xE8,       // INX
	0xE8,       // INX
	0x00        // BRK
};

bus.cpu.LoadProgram(program, 0x0000);
bus.cpu.Reset()
bus.cpu.Run()

// then check values of the registers if they are OK or not
```
### RAM Mirroring Test
```c++
#include "../src/bus.h"
#include <cassert>
#include <iostream>

int main() {
    Bus bus;

    bus.cpuWrite(0x0000, 0x42);
    bus.cpuWrite(0x07FF, 0x99);

    assert(bus.cpuRead(0x0000) == 0x42);
    assert(bus.cpuRead(0x0800) == 0x42);
    assert(bus.cpuRead(0x1000) == 0x42);
    assert(bus.cpuRead(0x1800) == 0x42);

    assert(bus.cpuRead(0x07FF) == 0x99);
    assert(bus.cpuRead(0x0FFF) == 0x99);
    assert(bus.cpuRead(0x17FF) == 0x99);
    assert(bus.cpuRead(0x1FFF) == 0x99);

    std::cout << ">> All RAM mirror tests passed!\n";
    return 0;
}
```
### Bus Integration Test
```c++
#include "../src/bus.h"
#include <iostream>
#include <cassert>
#include <vector>

int main() {
    Bus bus;

    // program: LDA #$42, INX, BRK
    std::vector<uint8_t> program = { 0xA9, 0x42, 0xE8, 0x00 };

    bus.cpu.LoadProgram(program, 0x0000);
    bus.cpu.Reset();

    std::cout << "Starting CPU Run...\n";
    bus.cpu.Run();

    assert(bus.cpu.A == 0x42);
    assert(bus.cpu.X == 0x01);

    std::cout << ">> CPU+Bus test passed! A=0x"
              << std::hex << (int)bus.cpu.A
              << " X=0x" << (int)bus.cpu.X << "\n";

    return 0;
}
```
