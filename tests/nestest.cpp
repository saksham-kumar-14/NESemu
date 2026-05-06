#include "../src/cpu.h"
#include "../src/bus.h"
#include "../src/cartridge.h"
#include <iostream>
#include <fstream>
#include <string>

int main() {
    Cartridge cart("nestest.nes");
    Bus bus;
    bus.ConnectCartridge(&cart);
    bus.cpu.Reset();

    // Force PC to the automated test start point
    bus.cpu.PC = 0xC000;

    // Open the official golden log
    std::ifstream logFile("nestest.log");
    if (!logFile.is_open()) {
        std::cerr << "ERROR: Could not open nestest.log!\n";
        return 1;
    }

    // Open our own output log file
    std::ofstream myLogFile("my_nestest.log");
    if (!myLogFile.is_open()) {
        std::cerr << "ERROR: Could not create my_nestest.log!\n";
        return 1;
    }

    std::string line;
    int lineNum = 1;

    // Run exactly 5003 times (The end of the official opcode tests)
    for (int i = 0; i < 5003; ++i) {
        std::getline(logFile, line);

        // Dynamically extract the expected state from the golden log
        std::string expected_PC = line.substr(0, 4);
        std::string expected_A  = line.substr(line.find("A:") + 2, 2);
        std::string expected_X  = line.substr(line.find("X:") + 2, 2);
        std::string expected_Y  = line.substr(line.find("Y:") + 2, 2);
        std::string expected_P  = line.substr(line.find("P:") + 2, 2);
        std::string expected_SP = line.substr(line.find("SP:") + 3, 2);

        // Format our CPU's current state
        char myState[128];
        snprintf(myState, sizeof(myState), "%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X",
            bus.cpu.PC, bus.cpu.A, bus.cpu.X, bus.cpu.Y, bus.cpu.P, bus.cpu.SP);

        // Write our current state to our output log
        myLogFile << myState << "\n";

        // also write in terminal
        std::cout << myState << '\n';

        char expState[128];
        snprintf(expState, sizeof(expState), "%s A:%s X:%s Y:%s P:%s SP:%s",
            expected_PC.c_str(), expected_A.c_str(), expected_X.c_str(),
            expected_Y.c_str(), expected_P.c_str(), expected_SP.c_str());

        // Compare
        if (std::string(myState) != std::string(expState)) {
            std::cerr << "\nCPU DESYNC DETECTED\n";
            std::cerr << "Failed at instruction line: " << lineNum << "\n";
            std::cerr << "Expected : " << expState << "\n";
            std::cerr << "Actual   : " << myState << "\n";
            return 1;
        }

        // Clock the CPU until the current instruction finishes
        do {
            bus.cpu.Clock();
        } while (bus.cpu.cycles != 0);

        lineNum++;
    }

    std::cout << "SUCCESS! All 5003 official opcodes match nestest.log perfectly!\n";
    std::cout << "Log saved to my_nestest.log\n";
    return 0;
}
