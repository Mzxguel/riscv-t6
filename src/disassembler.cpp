#include "disassembler.hpp"
#include "cpu.hpp"

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <string>

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
    return sign_extend((bits(in, 31, 25) << 5) | bits(in, 11, 7), 12);
}
inline int32_t imm_B(uint32_t in) {
    uint32_t v = (bits(in,31,31)<<12)|(bits(in,7,7)<<11)|(bits(in,30,25)<<5)|(bits(in,11,8)<<1);
    return sign_extend(v, 13);
}
inline int32_t imm_U(uint32_t in) { return static_cast<int32_t>(in & 0xFFFFF000u); }
inline int32_t imm_J(uint32_t in) {
    uint32_t v = (bits(in,31,31)<<20)|(bits(in,19,12)<<12)|(bits(in,20,20)<<11)|(bits(in,30,21)<<1);
    return sign_extend(v, 21);
}

std::string r(uint32_t i) { return "x" + std::to_string(i); }

std::string fmt(const char* tpl, ...) {
    char buf[128];
    va_list ap; va_start(ap, tpl);
    std::vsnprintf(buf, sizeof(buf), tpl, ap);
    va_end(ap);
    return std::string(buf);
}

}

std::string disassemble(uint32_t in, uint32_t addr) {
    const uint32_t opcode = bits(in, 6, 0);
    const uint32_t rd     = bits(in, 11, 7);
    const uint32_t f3     = bits(in, 14, 12);
    const uint32_t rs1    = bits(in, 19, 15);
    const uint32_t rs2    = bits(in, 24, 20);
    const uint32_t f7     = bits(in, 31, 25);
    const uint32_t shamt  = bits(in, 24, 20);

    switch (opcode) {
    case 0x03: {
        const char* n = "?";
        switch (f3) { case 0:n="lb";break; case 1:n="lh";break; case 2:n="lw";break;
                      case 4:n="lbu";break; case 5:n="lhu";break; default: return ".word"; }
        return fmt("%-5s %s, %d(%s)", n, r(rd).c_str(), imm_I(in), r(rs1).c_str());
    }
    case 0x13: {
        switch (f3) {
            case 0: return fmt("%-5s %s, %s, %d", "addi", r(rd).c_str(), r(rs1).c_str(), imm_I(in));
            case 1: return fmt("%-5s %s, %s, %u", "slli", r(rd).c_str(), r(rs1).c_str(), shamt);
            case 2: return fmt("%-5s %s, %s, %d", "slti", r(rd).c_str(), r(rs1).c_str(), imm_I(in));
            case 3: return fmt("%-5s %s, %s, %d", "sltiu",r(rd).c_str(), r(rs1).c_str(), imm_I(in));
            case 4: return fmt("%-5s %s, %s, %d", "xori", r(rd).c_str(), r(rs1).c_str(), imm_I(in));
            case 5: return (f7==0x20) ? fmt("%-5s %s, %s, %u","srai",r(rd).c_str(),r(rs1).c_str(),shamt)
                                      : fmt("%-5s %s, %s, %u","srli",r(rd).c_str(),r(rs1).c_str(),shamt);
            case 6: return fmt("%-5s %s, %s, %d", "ori",  r(rd).c_str(), r(rs1).c_str(), imm_I(in));
            case 7: return fmt("%-5s %s, %s, %d", "andi", r(rd).c_str(), r(rs1).c_str(), imm_I(in));
        }
        break;
    }
    case 0x17: return fmt("%-5s %s, 0x%X", "auipc", r(rd).c_str(), (uint32_t)imm_U(in) >> 12);
    case 0x23: {
        const char* n = (f3==0)?"sb":(f3==1)?"sh":(f3==2)?"sw":nullptr;
        if (!n) break;
        return fmt("%-5s %s, %d(%s)", n, r(rs2).c_str(), imm_S(in), r(rs1).c_str());
    }
    case 0x33: {
        const char* n = "?";
        switch (f3) {
            case 0: n=(f7==0x20)?"sub":"add"; break;
            case 1: n="sll"; break; case 2: n="slt"; break; case 3: n="sltu"; break;
            case 4: n="xor"; break;
            case 5: n=(f7==0x20)?"sra":"srl"; break;
            case 6: n="or";  break; case 7: n="and";  break;
        }
        return fmt("%-5s %s, %s, %s", n, r(rd).c_str(), r(rs1).c_str(), r(rs2).c_str());
    }
    case 0x37: return fmt("%-5s %s, 0x%X", "lui", r(rd).c_str(), (uint32_t)imm_U(in) >> 12);
    case 0x63: {
        const char* n = "?";
        switch (f3) { case 0:n="beq";break; case 1:n="bne";break; case 4:n="blt";break;
                      case 5:n="bge";break; case 6:n="bltu";break; case 7:n="bgeu";break;
                      default: break; }
        uint32_t tgt = addr + imm_B(in);
        return fmt("%-5s %s, %s, 0x%X", n, r(rs1).c_str(), r(rs2).c_str(), tgt);
    }
    case 0x67: return fmt("%-5s %s, %d(%s)", "jalr", r(rd).c_str(), imm_I(in), r(rs1).c_str());
    case 0x6F: {
        uint32_t tgt = addr + imm_J(in);
        return fmt("%-5s %s, 0x%X", "jal", r(rd).c_str(), tgt);
    }
    case 0x73:
        if (in == 0x00000073) return "ecall";
        if (in == 0x00100073) return "ebreak";
        break;
    }
    return fmt(".word 0x%08X", in);
}