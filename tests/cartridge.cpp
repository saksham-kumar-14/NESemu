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
