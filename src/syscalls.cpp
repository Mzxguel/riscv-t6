#include "syscalls.hpp"

#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>

namespace {
constexpr int A0 = 10;
constexpr int A1 = 11;
constexpr int A7 = 17;
}

StepStatus handle_ecall(CPU& cpu) {
    uint32_t service = cpu.reg(A7);

    switch (service) {

    case 1: {
        std::cout << static_cast<int32_t>(cpu.reg(A0));
        std::cout.flush();
        return StepStatus::Ecall;
    }

    case 4: {
        uint32_t addr = cpu.reg(A0);
        for (uint8_t c; (c = cpu.mem().read_byte(addr)) != 0; ++addr)
            std::cout << static_cast<char>(c);
        std::cout.flush();
        return StepStatus::Ecall;
    }

    case 5: {
        int32_t v = 0;
        std::cin >> v;
        cpu.set_reg(A0, static_cast<uint32_t>(v));
        return StepStatus::Ecall;
    }

    case 8: {
        uint32_t buf = cpu.reg(A0);
        int32_t  len = static_cast<int32_t>(cpu.reg(A1));
        std::string line;
        std::getline(std::cin, line);
        int32_t i = 0;
        for (; i < len - 1 && i < static_cast<int32_t>(line.size()); ++i)
            cpu.mem().write_byte(buf + i, static_cast<uint8_t>(line[i]));
        if (len > 0) cpu.mem().write_byte(buf + i, 0);  // null terminator
        return StepStatus::Ecall;
    }

    case 10:
        return StepStatus::Exit;

    case 11: {
        std::cout << static_cast<char>(cpu.reg(A0) & 0xFF);
        std::cout.flush();
        return StepStatus::Ecall;
    }

    case 12: {
        int c = std::getchar();
        cpu.set_reg(A0, static_cast<uint32_t>(c));
        return StepStatus::Ecall;
    }

    case 17:
        cpu.set_exit_code(static_cast<int>(cpu.reg(A0)));
        return StepStatus::Exit;

    case 34: {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "0x%08X", cpu.reg(A0));
        std::cout << buf;
        std::cout.flush();
        return StepStatus::Ecall;
    }

    case 35: {
        uint32_t v = cpu.reg(A0);
        std::string s;
        for (int i = 31; i >= 0; --i) s += ((v >> i) & 1) ? '1' : '0';
        std::cout << "0b" << s;
        std::cout.flush();
        return StepStatus::Ecall;
    }

    case 36: {
        std::cout << cpu.reg(A0);
        std::cout.flush();
        return StepStatus::Ecall;
    }

    default:
        return StepStatus::EcallUnknown;
    }
}