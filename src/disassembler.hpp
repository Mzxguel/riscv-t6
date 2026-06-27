#ifndef RISCV_DISASSEMBLER_HPP
#define RISCV_DISASSEMBLER_HPP

#include <cstdint>
#include <string>

std::string disassemble(uint32_t instr, uint32_t addr);

#endif