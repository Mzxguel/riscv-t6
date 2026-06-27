#ifndef RISCV_LOADER_HPP
#define RISCV_LOADER_HPP

#include <cstdint>
#include <string>

#include "memory.hpp"

struct LoadResult {
    bool     ok = false;
    uint32_t entry = 0;
    uint32_t size = 0;
    bool     was_elf = false;
    std::string error;
};

LoadResult load_program(Memory& mem, const std::string& path,
                        uint32_t base_addr = 0x00000000);

#endif