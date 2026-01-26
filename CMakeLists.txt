cmake_minimum_required(VERSION 3.10)
project(NES_Emulator VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
find_package(SDL2 REQUIRED)

set(SOURCES
    tests/cartridge.cpp
    src/cartridge.cpp
    src/bus.cpp
    src/cpu.cpp
)
add_executable(nes_emu ${SOURCES})
target_include_directories(nes_emu PRIVATE
    src
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(nes_emu PRIVATE
    ${SDL2_LIBRARIES}
)
