Here is the fully updated markdown. I have embedded explicit references to **diagnostic logging**, **smart pointers**, **polymorphism**, **virtual functions**, and the **`nestest.log` validation** exactly where they fit best within the architectural breakdown. 

You can click the "Copy code" button in the top right corner of the block below to grab the entire file at once.

# Cartridge
Its job is to load the ROM along with formatting and organizing data such that the CPU and PPU can understand it. 

**Class Members:**
- `romLoad`: `true` if file opened successfully.
- `mapperID`: Tells the emulator which logic to emulate. e.g., `Mapper 0` is `NROM`, `Mapper 1` is `MMC1`.
- `mirror`: How the screen wraps around.
- `PRGMemory`: This holds the Game Code.
- `CHRMemory`: This holds the Graphics.
- `pMapper`: A polymorphic **smart pointer** (`std::shared_ptr<Mapper>`) pointing to the specific mapper logic currently loaded. This ensures automatic, leak-free memory management while allowing the emulator to dynamically swap hardware behaviors at runtime.

**Procedure:**
1. Open the ROM file.
2. Check if the header (16 bytes from the starting) is equal to official NES header i.e., `header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A`.
3. `header[4]` tells how much game code is stored. 1 unit of Game code means 16 KBs.
4. `header[5]` tells how much graphics is stored. 1 unit of Graphics means 8 KBs.
5. Let's call `header[6]` and `header[7]`, Flag 6 and Flag 7. We can get the `mapperID` mentioned above using as follow:
    ```text
    Flag 6 : [XXXX (Low) | ....]
    Flag 7 : [XXXX (High)| ....]
    mapperID : [XXXX (High) | XXXX (Low)]
    ```
6. There is sometimes junk code of 512 bytes at the starting of header (usually there for cheat codes). Bit 2 of Flag 6 tells if junk code is there or not. This junk code is called a trainer.
7. `mirror = flag6 & 0x01;` Extract the last bit of flag 6 to check mirror.
8. Then load `PRGMemory` and `CHRMemory`.
9. Instantiate the correct Mapper object based on `mapperID` and assign it to `pMapper`.
10. Set `romLoad` to `true`.

    ```text
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

---

# Mappers Architecture
Mappers sit between the Cartridge memory arrays and the CPU/PPU. Because the NES CPU can only see a limited amount of memory (`0x8000` to `0xFFFF` for cartridges), mappers act as "translators" or "bank switchers" to swap larger amounts of ROM data in and out of that narrow CPU window.

By leveraging **C++ Polymorphism** and **virtual functions**, the `Cartridge` class doesn't need to know *how* the memory is routed; it simply asks the active Mapper class to execute its specific translation override to find the physical array index.

### 1. The Base `Mapper` Class
An abstract base class that defines the blueprint for all future mappers.
```cpp
class Mapper {
protected:
    uint8_t prgBanks = 0;
    uint8_t chrBanks = 0;

public:
    Mapper(uint8_t prgBanks, uint8_t chrBanks) : prgBanks(prgBanks), chrBanks(chrBanks) {}
    virtual ~Mapper() = default;

    // Pure virtual functions: specific mappers MUST override these
    virtual bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) = 0;
};
```

### 2. `Mapper_000` (NROM)
The simplest mapper (no bank switching). It supports either 16KB (1 Bank) or 32KB (2 Banks) of PRG ROM. 

**The 16KB Mirroring Quirk:**
If a game only has 16KB of PRG ROM, the hardware physically wires it so that the data is mirrored across the entire `0x8000` to `0xFFFF` window. 
* Reading from `0x8000` returns the first byte.
* Reading from `0xC000` wraps around and returns that exact same first byte. 
* In code, this is handled elegantly by masking the CPU address with `0x3FFF` (for 16KB) or `0x7FFF` (for 32KB).
```cpp
class Mapper_000 : public Mapper {
public:
    Mapper_000(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {}

    bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr) override {
        if (addr >= 0x8000 && addr <= 0xFFFF) {
            // 0x7FFF for 32KB (2 banks), 0x3FFF for 16KB (1 bank)
            mapped_addr = addr & (prgBanks > 1 ? 0x7FFF : 0x3FFF);
            return true;
        }
        return false;
    }

    bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override {
        if (addr >= 0x8000 && addr <= 0xFFFF) {
            mapped_addr = addr & (prgBanks > 1 ? 0x7FFF : 0x3FFF);
            return true; // NROM is read-only, but we acknowledge the address was valid
        }
        return false;
    }
};
```

---

# CPU & BUS

## BUS
Does all the memory management like an MMU.
- **Memory map handled by Bus:**
	- `0x0000 - 0x07FF` : 2KB internal RAM
	- `0x0800 - 0x1FFF` : Mirror of `0x0000 - 0x07FF`
	- `0x2000 - 0x3FFF` : PPU registers (mirrored every 8 bytes)
	- `0x8000 - 0xFFFF` : PRG ROM (from cartridge)
- **Process:**
	- Make `std::array<uint8_t, RAMsize> RAM{};` and `memset` it with zeros.
	- Connect cartridge.
- **`cpuWrite(uint16_t addr, uint8_t data)`:**
	- If the Cartridge is loaded, it gets first dibs on the write operation via `cart->cpuWrite()`. This allows Cartridge mappers to intercept writes for bank switching or configuration.
	- If the Cartridge ignores it and `addr <= 0x1FFF`, the data is written to internal RAM, mapped with `addr & 0x07FF` to handle the 8KB mirroring over the 2KB physical RAM.
- **`cpuRead(uint16_t addr)`:**
	- Similar to writes, it first asks the Cartridge if it wants to handle the read via `cart->cpuRead()`. 
	- If the Cartridge doesn't intercept, and `addr <= 0x1FFF`, it returns from the internal RAM using `RAM[addr & 0x07FF]`.

---

## CPU
- **Registers** 
	- `A`: Accumulator. Almost all arithmetic operations happen here.
	- `X` & `Y`: Index registers used for loops and handling arrays/offsets.
	- `SP (Stack Pointer)`: Points at a temporary area (stack) for function execution. The stack lives in memory page 1 (`0x0100` - `0x01FF`).
	- `PC (Program Counter)`: Points to the memory address of the next instruction to be executed. It goes from `0x0000` to `0xFFFF`.
	- `P (Status Register)` : A collection of single 8-bit flags (Zero, Carry, Negative, etc.) that tells the CPU the result of the last instruction. **Note on the B-Flag (Bit 4):** This flag does not physically exist inside the CPU. It is only set when pushed to the stack (1 for software instructions like `PHP`/`BRK`, 0 for hardware interrupts like `NMI`/`IRQ`). It must be ignored/cleared when pulled back via `PLP` or `RTI`.
	- `cycles` : Cycles remaining for the current instruction. If cycles is 0, the next instruction can be executed.
	- `total_cycles` : Used mostly for debugging purposes. 
	- *Note that only `PC` is `uint16_t`, others are `uint8_t`.*

- **Hardware Interrupts**
	- `NMI()` (Non-Maskable Interrupt): Usually triggered by the PPU every frame (VBlank). It forces the CPU to pause, pushes the current `PC` and Status Register (`P`) to the stack, sets the Interrupt Disable flag (`I`), and jumps to the address hardcoded at memory vector `0xFFFA`-`0xFFFB`. Takes 7 cycles. This is almost exclusively used by the PPU (the graphics chip). The PPU draws the screen 60 times a second. Every time it finishes drawing a frame, it fires an NMI to the CPU. It essentially screams: "I'm done drawing! The screen is in VBlank! Send me the graphics for the next frame right now before the TV starts drawing again!"
	- `IRQ()` (Interrupt Request): A software-maskable interrupt (can be ignored if the `I` flag is set). If allowed, it behaves like an NMI but jumps to the vector stored at `0xFFFE`-`0xFFFF`. Takes 7 cycles.

- **Addressing Modes**
	- Handled dynamically inside `Execute()` using lambda functions to fetch the correct memory address based on the opcode.
	- **Immediate**: Uses the next byte directly as data (`ADDR_IMM`).
	- **Zero Page**: Uses a 1-byte address (`0x00` to `0xFF`) to save cycles (`ADDR_ZP`, `ADDR_ZPX`, `ADDR_ZPY`).
	- **Absolute**: Uses a full 2-byte address (`ADDR_ABS`, `ADDR_ABSX`, `ADDR_ABSY`).
	- **Indirect / Indexed**: Fetches pointers from memory, enabling complex array and lookup table operations (`ADDR_INDX`, `ADDR_INDY`).

- **Instruction Set (Opcode Execution)**
	- The `Execute()` function acts as a massive switch statement to process the fetched opcode. Implemented categories include:
		- **Loads & Stores**: `LDA`, `LDX`, `LDY`, `STA`, `STX`, `STY`
		- **Register Transfers**: `TAX`, `TAY`, `TXA`, `TYA`, `TXS`, `TSX`
		- **Arithmetic**: `ADC` (Add with Carry) and `SBC` (Subtract with Carry)
		- **Logical Operations**: `AND`, `ORA`, `EOR`, `BIT`
		- **Increments & Decrements**: `INC`, `DEC`, `INX`, `INY`, `DEX`, `DEY`
		- **Bitwise Shifts**: `ASL`, `LSR`, `ROL`, `ROR`
		- **Comparisons**: `CMP`, `CPX`, `CPY`
		- **Branching**: `BCC`, `BCS`, `BEQ`, `BNE`, `BMI`, `BPL`, `BVC`, `BVS` (includes cycle penalties for page-crossing)
		- **Stack Operations**: `PHA`, `PHP`, `PLA`, `PLP` (Push/Pull Accumulator and Status)
		- **Jumps & Subroutines**: `JMP`, `JSR`, `RTS`, `RTI`

- **Core Functions**
	- `Reset()`: 
        ```cpp
        A = X = Y = 0;  // clean registers
        SP = 0xFD;      // Stack pointer initializes here due to NES startup quirks.
        P = 0x24;       // Binary: 0010 0100
                        // Bit 2 is set to 1 to disable interrupts at startup
                        // Bit 5 is technically "unused", but wired high inside the 6502

        // Fetch PC from the reset vector (Little-endian)
        uint16_t PC = (bus->cpuRead(0xFFFD) << 8) | bus->cpuRead(0xFFFC); 
        ```
	- `LoadProgram(vector<uint8_t>& program, uint16_t startAddr)`: Writes the entire program to memory starting at `startAddr`.
	- `Clock()`: Fetches Opcode using `bus->cpuRead(PC)` and executes it via `Execute()`, decrementing cycles.
	- `Run()`: Endless loop driving the `Clock()` function until a `BRK` instruction (`0x00`) is hit.
    - **Diagnostic Logging (`GetDebugString()`)**: Generates a perfectly formatted snapshot of the CPU's current registers (e.g., `C000 A:AA X:01 Y:02 P:24 SP:FD`). This powers the automated test pipeline, diffing the emulator's execution state against the golden hardware standard. **The CPU officially passes the rigorous `nestest.log` validation suite for all 5003 official opcodes without a single desync**, ensuring 100% cycle-accurate execution.
	- `SetFlag(uint8_t bit, bool value)`: Turns specific bits in the status register `P` ON (`|`) or OFF (`& ~`).
	- `GetFlag(uint8_t bit)`: Returns `(P & bit) != 0`.
```
