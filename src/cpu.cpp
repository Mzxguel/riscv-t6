#include "cpu.hpp"
#include "syscalls.hpp"

#include <cstdint>

const char* const kAbiNames[32] = {
    "zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

namespace {

inline uint32_t bits(uint32_t v, int hi, int lo) {
    return (v >> lo) & ((1u << (hi - lo + 1)) - 1);
}

inline int32_t sign_extend(uint32_t v, int n) {
    uint32_t m = 1u << (n - 1);
    return static_cast<int32_t>((v ^ m) - m);
}

inline int32_t imm_I(uint32_t in) { return sign_extend(bits(in, 31, 20), 12); }

inline int32_t imm_S(uint32_t in) {
    uint32_t v = (bits(in, 31, 25) << 5) | bits(in, 11, 7);
    return sign_extend(v, 12);
}

inline int32_t imm_B(uint32_t in) {
    uint32_t v = (bits(in, 31, 31) << 12) |
                 (bits(in, 7, 7)   << 11) |
                 (bits(in, 30, 25) << 5)  |
                 (bits(in, 11, 8)  << 1);
    return sign_extend(v, 13);
}

inline int32_t imm_U(uint32_t in) {
    return static_cast<int32_t>(in & 0xFFFFF000u);
}

inline int32_t imm_J(uint32_t in) {
    uint32_t v = (bits(in, 31, 31) << 20) |
                 (bits(in, 19, 12) << 12) |
                 (bits(in, 20, 20) << 11) |
                 (bits(in, 30, 21) << 1);
    return sign_extend(v, 21);
}

}

StepStatus CPU::step() {
    uint32_t instr = mem_.read_word(pc_);
    last_instr_ = instr;
    StepStatus st = execute(instr);
    if (st == StepStatus::Ok || st == StepStatus::Ecall)
        ++instr_count_;
    regs_[0] = 0;
    return st;
}

StepStatus CPU::execute(uint32_t in) {
    const uint32_t opcode = bits(in, 6, 0);
    const uint32_t rd     = bits(in, 11, 7);
    const uint32_t funct3 = bits(in, 14, 12);
    const uint32_t rs1    = bits(in, 19, 15);
    const uint32_t rs2    = bits(in, 24, 20);
    const uint32_t funct7 = bits(in, 31, 25);

    uint32_t next_pc = pc_ + 4;

    switch (opcode) {

    case 0x03: {
        uint32_t addr = reg(rs1) + imm_I(in);
        uint32_t val = 0;
        switch (funct3) {
            case 0x0: val = sign_extend(mem_.read_byte(addr), 8);  break; // lb
            case 0x1: val = sign_extend(mem_.read_half(addr), 16); break; // lh
            case 0x2: val = mem_.read_word(addr);                  break; // lw
            case 0x4: val = mem_.read_byte(addr);                  break; // lbu
            case 0x5: val = mem_.read_half(addr);                  break; // lhu
            default:  return StepStatus::IllegalInstr;
        }
        set_reg(rd, val);
        break;
    }

    case 0x13: {
        int32_t  imm  = imm_I(in);
        uint32_t a    = reg(rs1);
        uint32_t shamt = bits(in, 24, 20);
        uint32_t val = 0;
        switch (funct3) {
            case 0x0: val = a + imm; break;                                  // addi
            case 0x1: val = a << shamt; break;                               // slli
            case 0x2: val = (static_cast<int32_t>(a) < imm) ? 1 : 0; break;  // slti
            case 0x3: val = (a < static_cast<uint32_t>(imm)) ? 1 : 0; break; // sltiu
            case 0x4: val = a ^ imm; break;                                  // xori
            case 0x5:
                if (funct7 == 0x20) val = static_cast<int32_t>(a) >> shamt;   // srai
                else                val = a >> shamt;                         // srli
                break;
            case 0x6: val = a | imm; break;                                  // ori
            case 0x7: val = a & imm; break;                                  // andi
            default:  return StepStatus::IllegalInstr;
        }
        set_reg(rd, val);
        break;
    }

    case 0x17:
        set_reg(rd, pc_ + imm_U(in));
        break;

    case 0x23: {
        uint32_t addr = reg(rs1) + imm_S(in);
        uint32_t val  = reg(rs2);
        switch (funct3) {
            case 0x0: mem_.write_byte(addr, static_cast<uint8_t>(val));  break; // sb
            case 0x1: mem_.write_half(addr, static_cast<uint16_t>(val)); break; // sh
            case 0x2: mem_.write_word(addr, val);                        break; // sw
            default:  return StepStatus::IllegalInstr;
        }
        break;
    }

    case 0x33: {
        uint32_t a = reg(rs1);
        uint32_t b = reg(rs2);
        uint32_t sh = b & 0x1F;
        uint32_t val = 0;
        switch (funct3) {
            case 0x0: val = (funct7 == 0x20) ? (a - b) : (a + b); break;       // sub/add
            case 0x1: val = a << sh; break;                                    // sll
            case 0x2: val = (static_cast<int32_t>(a) < static_cast<int32_t>(b)) ? 1 : 0; break; // slt
            case 0x3: val = (a < b) ? 1 : 0; break;                            // sltu
            case 0x4: val = a ^ b; break;                                      // xor
            case 0x5: val = (funct7 == 0x20) ? (static_cast<int32_t>(a) >> sh) // sra
                                             : (a >> sh); break;               // srl
            case 0x6: val = a | b; break;                                      // or
            case 0x7: val = a & b; break;                                      // and
            default:  return StepStatus::IllegalInstr;
        }
        set_reg(rd, val);
        break;
    }

    case 0x37:
        set_reg(rd, imm_U(in));
        break;

    case 0x63: {
        uint32_t a = reg(rs1), b = reg(rs2);
        bool taken = false;
        switch (funct3) {
            case 0x0: taken = (a == b); break;                                              // beq
            case 0x1: taken = (a != b); break;                                              // bne
            case 0x4: taken = (static_cast<int32_t>(a) <  static_cast<int32_t>(b)); break;  // blt
            case 0x5: taken = (static_cast<int32_t>(a) >= static_cast<int32_t>(b)); break;  // bge
            case 0x6: taken = (a <  b); break;                                              // bltu
            case 0x7: taken = (a >= b); break;                                              // bgeu
            default:  return StepStatus::IllegalInstr;
        }
        if (taken) next_pc = pc_ + imm_B(in);
        break;
    }

    case 0x67: {
        uint32_t target = (reg(rs1) + imm_I(in)) & ~1u;
        set_reg(rd, pc_ + 4);
        next_pc = target;
        break;
    }

    case 0x6F:
        set_reg(rd, pc_ + 4);
        next_pc = pc_ + imm_J(in);
        break;

    case 0x73: {
        if (in == 0x00000073) {
            StepStatus st = handle_ecall(*this);
            pc_ = next_pc;
            return st;
        }
        if (in == 0x00100073) { pc_ = next_pc; return StepStatus::Ok; }
        return StepStatus::IllegalInstr;
    }

    default:
        return StepStatus::IllegalInstr;
    }

    pc_ = next_pc;
    return StepStatus::Ok;
}