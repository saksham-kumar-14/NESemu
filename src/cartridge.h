#pragma once
#include<vector>
#include<string>
#include <cstdint>

using namespace std;

Class Cartridge {
public:
    bool romLoad = false;
    uint8_t mapperID = 0;
    uint8_t mirror = 0;

    vector<uint8_t> PRGMemory;
    vector<uint8_t> CHRMemory;

    Cartridge(const string& filename);
};
