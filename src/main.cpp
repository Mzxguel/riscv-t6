#include <cctype>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "cpu.hpp"
#include "disassembler.hpp"
#include "loader.hpp"
#include "syscalls.hpp"

namespace {

bool parse_u32(const std::string& s, uint32_t& out) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        long long v;
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            v = std::stoll(s.substr(2), &pos, 16), pos += 2;
        else
            v = std::stoll(s, &pos, 0);
        if (pos != s.size()) return false;
        out = static_cast<uint32_t>(v);
        return true;
    } catch (...) { return false; }
}

int parse_reg(const std::string& s) {
    if (s.size() >= 2 && (s[0] == 'x' || s[0] == 'X')) {
        uint32_t n;
        if (parse_u32(s.substr(1), n) && n < 32) return static_cast<int>(n);
    }
    for (int i = 0; i < 32; ++i)
        if (s == kAbiNames[i]) return i;
    uint32_t n;
    if (parse_u32(s, n) && n < 32) return static_cast<int>(n);
    return -1;
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> t;
    std::istringstream iss(line);
    std::string w;
    while (iss >> w) t.push_back(w);
    return t;
}

void print_pc(const CPU& cpu) {
    std::printf("pc = 0x%08X\n", cpu.pc());
}

void print_reg(const CPU& cpu, int i) {
    std::printf("x%-2d (%-4s) = 0x%08X   (%d)\n",
                i, kAbiNames[i], cpu.reg(i), static_cast<int32_t>(cpu.reg(i)));
}

void print_all_regs(const CPU& cpu) {
    print_pc(cpu);
    for (int i = 0; i < 32; ++i) print_reg(cpu, i);
}

void print_mem(const CPU& cpu, uint32_t start, uint32_t end) {
    if (end < start) std::swap(start, end);
    std::printf("Memoria (0x%08X-0x%08X):\n", start, end);
    uint32_t addr = start & ~0xFu;
    for (; addr <= end; addr += 16) {
        std::printf("0x%08X: ", addr);
        std::string ascii;
        for (int j = 0; j < 16; ++j) {
            uint32_t a = addr + j;
            if (a < start || a > end) { std::printf("   "); ascii += ' '; continue; }
            uint8_t b = cpu.mem().read_byte(a);
            std::printf("%02X ", b);
            ascii += (b >= 32 && b < 127) ? static_cast<char>(b) : '.';
        }
        std::printf(" |%s|\n", ascii.c_str());
    }
}

void disasm_range(const CPU& cpu, uint32_t start, int count) {
    for (int i = 0; i < count; ++i) {
        uint32_t addr = start + i * 4;
        uint32_t instr = cpu.mem().read_word(addr);
        std::string marker = (addr == cpu.pc()) ? "=>" : "  ";
        std::printf("%s 0x%08X: %08X    %s\n",
                    marker.c_str(), addr, instr, disassemble(instr, addr).c_str());
    }
}

const char* status_str(StepStatus s) {
    switch (s) {
        case StepStatus::Ok:           return "ok";
        case StepStatus::Ecall:        return "ecall";
        case StepStatus::Exit:         return "exit";
        case StepStatus::IllegalInstr: return "instruccion ilegal";
        case StepStatus::EcallUnknown: return "ecall desconocido";
    }
    return "?";
}

void print_help() {
    std::cout <<
    "Comandos disponibles:\n"
    "  load <archivo> [base]   Carga un programa. base por defecto 0x0\n"
    "  step [n]                Ejecuta n instrucciones\n"
    "  run [max]               Ejecuta hasta exit/ilegal/break o hasta max instrucciones\n"
    "  pc                      Muestra el PC\n"
    "  regs [r1 r2 ...]        Muestra registros\n"
    "  mem <ini> [fin]         Muestra memoria en el rango [ini, fin]\n"
    "  dis [addr] [n]          Desensambla n instrucciones desde addr\n"
    "  setreg <reg> <valor>    Escribe un registro\n"
    "  setmem <addr> <valor>   Escribe un word en memoria\n"
    "  break <addr>            Coloca un breakpoint en addr\n"
    "  break clear             Elimina todos los breakpoints\n"
    "  reset                   Reinicia el estado \n"
    "  help                    Muestra esta ayuda\n"
    "  exit | quit             Sale del simulador\n";
}

}

int main(int argc, char** argv) {
    CPU cpu;
    std::string loaded_path;
    std::vector<uint32_t> breakpoints;

    std::cout << "=== Simulador RISC-V (RV32I) ===\n";

    if (argc >= 2) {
        LoadResult r = load_program(cpu.mem(), argv[1]);
        if (r.ok) {
            cpu.set_pc(r.entry);
            loaded_path = argv[1];
            std::printf("\"%s\" cargado a memoria (%u bytes%s). Entrada = 0x%08X\n",
                        argv[1], r.size, r.was_elf ? ", ELF" : "", r.entry);
        } else {
            std::cout << "Error al cargar: " << r.error << "\n";
        }
    } else {
        std::cout << "Use 'load <archivo>' para cargar un programa, o 'help'.\n";
    }

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) { std::cout << "\n"; break; }
        auto tok = tokenize(line);
        if (tok.empty()) continue;
        const std::string& cmd = tok[0];

        if (cmd == "exit" || cmd == "quit") {
            std::cout << "See you next time...\n";
            break;
        }
        else if (cmd == "help") {
            print_help();
        }
        else if (cmd == "load") {
            if (tok.size() < 2) { std::cout << "Uso: load <archivo> [base]\n"; continue; }
            uint32_t base = 0;
            if (tok.size() >= 3 && !parse_u32(tok[2], base)) {
                std::cout << "Base invalida\n"; continue;
            }
            LoadResult r = load_program(cpu.mem(), tok[1], base);
            if (r.ok) {
                cpu.reset();
                cpu.set_pc(r.entry);
                loaded_path = tok[1];
                std::printf("\"%s\" cargado a memoria (%u bytes%s). Entrada = 0x%08X\n",
                            tok[1].c_str(), r.size, r.was_elf ? ", ELF" : "", r.entry);
            } else {
                std::cout << "Error: " << r.error << "\n";
            }
        }
        else if (cmd == "pc") {
            print_pc(cpu);
        }
        else if (cmd == "step") {
            uint32_t n = 1;
            if (tok.size() >= 2 && !parse_u32(tok[1], n)) { std::cout << "n invalido\n"; continue; }
            for (uint32_t i = 0; i < n; ++i) {
                uint32_t at = cpu.pc();
                uint32_t instr = cpu.mem().read_word(at);
                StepStatus s = cpu.step();
                std::printf("0x%08X: %08X   %s\n", at, instr, disassemble(instr, at).c_str());
                if (s == StepStatus::Exit) {
                    std::printf("Program exited with code %d.\n", cpu.exit_code());
                    break;
                }
                if (s == StepStatus::IllegalInstr || s == StepStatus::EcallUnknown) {
                    std::printf("** %s en 0x%08X **\n", status_str(s), at);
                    break;
                }
            }
        }
        else if (cmd == "run") {
            uint32_t max = 10'000'000;
            if (tok.size() >= 2 && !parse_u32(tok[1], max)) { std::cout << "max invalido\n"; continue; }
            uint32_t executed = 0;
            bool stopped = false;
            while (executed < max) {
                uint32_t at = cpu.pc();
                // breakpoint?
                bool hit = false;
                for (uint32_t bp : breakpoints) if (bp == at) hit = true;
                if (hit && executed > 0) {
                    std::printf("Breakpoint en 0x%08X\n", at);
                    stopped = true; break;
                }
                uint32_t instr = cpu.mem().read_word(at);
                StepStatus s = cpu.step();
                ++executed;
                if (s == StepStatus::Exit) {
                    std::printf("Program exited with code %d.  (%u instrucciones)\n",
                                cpu.exit_code(), executed);
                    stopped = true; break;
                }
                if (s == StepStatus::IllegalInstr || s == StepStatus::EcallUnknown) {
                    std::printf("** %s en 0x%08X (instr 0x%08X) **\n",
                                status_str(s), at, instr);
                    stopped = true; break;
                }
                if (cpu.pc() == at) {
                    std::printf("Programa detenido en bucle en 0x%08X.  (%u instrucciones)\n",
                                at, executed);
                    stopped = true; break;
                }
            }
            if (!stopped)
                std::printf("Detenido: limite de %u instrucciones alcanzado.\n", max);
        }
        else if (cmd == "regs") {
            if (tok.size() == 1) { print_all_regs(cpu); }
            else {
                for (size_t i = 1; i < tok.size(); ++i) {
                    int idx = parse_reg(tok[i]);
                    if (idx < 0) std::printf("Registro invalido: %s\n", tok[i].c_str());
                    else         print_reg(cpu, idx);
                }
            }
        }
        else if (cmd == "mem") {
            if (tok.size() < 2) { std::cout << "Uso: mem <ini> [fin]\n"; continue; }
            uint32_t a, b;
            if (!parse_u32(tok[1], a)) { std::cout << "Direccion invalida\n"; continue; }
            b = a + 15;
            if (tok.size() >= 3 && !parse_u32(tok[2], b)) { std::cout << "Direccion invalida\n"; continue; }
            print_mem(cpu, a, b);
        }
        else if (cmd == "dis") {
            uint32_t start = cpu.pc();
            uint32_t n = 8;
            if (tok.size() >= 2 && !parse_u32(tok[1], start)) { std::cout << "addr invalido\n"; continue; }
            if (tok.size() >= 3 && !parse_u32(tok[2], n))     { std::cout << "n invalido\n"; continue; }
            disasm_range(cpu, start, static_cast<int>(n));
        }
        else if (cmd == "setreg") {
            if (tok.size() < 3) { std::cout << "Uso: setreg <reg> <valor>\n"; continue; }
            int idx = parse_reg(tok[1]);
            uint32_t v;
            if (idx < 0)              { std::cout << "Registro invalido\n"; continue; }
            if (!parse_u32(tok[2], v)) { std::cout << "Valor invalido\n"; continue; }
            cpu.set_reg(idx, v);
            print_reg(cpu, idx);
        }
        else if (cmd == "setmem") {
            if (tok.size() < 3) { std::cout << "Uso: setmem <addr> <valor>\n"; continue; }
            uint32_t a, v;
            if (!parse_u32(tok[1], a)) { std::cout << "Direccion invalida\n"; continue; }
            if (!parse_u32(tok[2], v)) { std::cout << "Valor invalido\n"; continue; }
            cpu.mem().write_word(a, v);
            print_mem(cpu, a, a + 3);
        }
        else if (cmd == "break") {
            if (tok.size() < 2) { std::cout << "Uso: break <addr> | break clear\n"; continue; }
            if (tok[1] == "clear") { breakpoints.clear(); std::cout << "Breakpoints eliminados\n"; continue; }
            uint32_t a;
            if (!parse_u32(tok[1], a)) { std::cout << "Direccion invalida\n"; continue; }
            breakpoints.push_back(a);
            std::printf("Breakpoint en 0x%08X\n", a);
        }
        else if (cmd == "reset") {
            cpu.reset();
            if (!loaded_path.empty()) {
                LoadResult r = load_program(cpu.mem(), loaded_path);
                if (r.ok) cpu.set_pc(r.entry);
            }
            std::cout << "Estado reiniciado.\n";
        }
        else {
            std::cout << "Comando desconocido: " << cmd << "  (escribe 'help')\n";
        }
    }
    return 0;
}