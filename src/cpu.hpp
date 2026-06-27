#ifndef RISCV_CPU_HPP
#define RISCV_CPU_HPP

#include <array>
#include <cstdint>
#include <string>

#include "memory.hpp"

enum class StepStatus {
    Ok,            
    Ecall,
    Exit,          
    IllegalInstr,  
    EcallUnknown   
};

class CPU {
public:
    static constexpr uint32_t RESET_PC = 0x00000000;

    CPU() { reset(); }

    void reset() {
        pc_ = RESET_PC;
        regs_.fill(0);
        exit_code_ = 0;
        instr_count_ = 0;
        last_instr_ = 0;
    }

    uint32_t  pc()  const { return pc_; }
    void      set_pc(uint32_t v) { pc_ = v; }

    uint32_t  reg(int i) const { return regs_[i & 31]; }
    void      set_reg(int i, uint32_t v) {
        i &= 31;
        if (i != 0) regs_[i] = v;
    }

    Memory&        mem()       { return mem_; }
    const Memory&  mem() const { return mem_; }

    int       exit_code()    const { return exit_code_; }
    void      set_exit_code(int c) { exit_code_ = c; }
    uint64_t  instr_count()  const { return instr_count_; }
    uint32_t  last_instr()   const { return last_instr_; }

    StepStatus step();

private:
    uint32_t                   pc_;
    std::array<uint32_t, 32>   regs_;
    Memory                     mem_;
    int                        exit_code_ = 0;
    uint64_t                   instr_count_ = 0;
    uint32_t                   last_instr_ = 0;

    StepStatus execute(uint32_t instr);
};

extern const char* const kAbiNames[32];

#endif