cmake_minimum_required(VERSION 3.10)
project(NES_Emulator VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SDL2 REQUIRED)

add_library(nes_core
    src/cartridge.cpp
    src/bus.cpp
    src/cpu.cpp
    src/ppu.cpp
)
target_include_directories(nes_core PUBLIC src)

enable_testing()

function(add_nes_test test_name source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE nes_core)

    add_test(NAME ${test_name} COMMAND ${test_name})

    add_custom_command(
        TARGET ${test_name} POST_BUILD
        COMMAND ${test_name}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running ${test_name} automatically during build..."
    )
endfunction()

# Tests
add_nes_test(test_cpu tests/cpu.cpp)
add_nes_test(test_bus_integration tests/bus_integration.cpp)
add_nes_test(test_cartridge tests/cartridge.cpp)
add_nes_test(test_ram_mirroring tests/ram_mirroring.cpp)
add_nes_test(test_mapper_000 tests/mapper_000.cpp)
add_nes_test(test_ppu_pallete tests/pallete.cpp)
add_nes_test(test_ppu tests/ppu.cpp)

# nes test golden log verification
add_executable(run_nestest tests/nestest.cpp)
target_link_libraries(run_nestest PRIVATE nes_core)
add_custom_command(
    TARGET run_nestest POST_BUILD
    COMMAND run_nestest
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running nestest.nes and verifying official opcodes..."
)
