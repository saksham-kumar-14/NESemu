#include <cstdint>
class PPU{
public:
    uint8_t cpuRead(uint16_t addr);
    void cpuWrite(uint16_t addr, uint8_t data);

private:
    // registers
    uint8_t status;
    uint8_t control;
    uint8_t mask;

    // memory + buffers
    uint8_t vram[2048];     // Nametables
    uint8_t palette[32];    // Color palettes
};
