cmake_minimum_required(VERSION 3.10)
project(NES_Emulator VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SDL2 QUIET)

# Core library
add_library(nes_core
    src/cartridge.cpp
    src/bus.cpp
    src/cpu.cpp
)
target_include_directories(nes_core PUBLIC src)

# Enable testing
enable_testing()

# Helper function to add tests
function(add_nes_test test_name source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE nes_core)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Add existing tests
add_nes_test(test_cpu tests/cpu.cpp)
add_nes_test(test_bus_integration tests/bus_integration.cpp)
add_nes_test(test_cartridge tests/cartridge.cpp)
add_nes_test(test_ram_mirroring tests/ram_mirroring.cpp)

# Keep the nes_emu executable if it had a main, but right now it doesn't.
# We will create a dummy nes_emu if we want, or just remove it until src/main.cpp is ready.
# add_executable(nes_emu src/main.cpp)
# target_link_libraries(nes_emu PRIVATE nes_core ${SDL2_LIBRARIES})