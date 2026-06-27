#ifndef RISCV_SYSCALLS_HPP
#define RISCV_SYSCALLS_HPP

#include "cpu.hpp"

StepStatus handle_ecall(CPU& cpu);

#endif