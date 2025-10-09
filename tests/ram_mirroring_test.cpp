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
