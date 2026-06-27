#include "loader.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

namespace {

uint16_t rd16(const std::vector<uint8_t>& b, size_t o) {
    return  static_cast<uint16_t>(b[o]) | (static_cast<uint16_t>(b[o + 1]) << 8);
}
uint32_t rd32(const std::vector<uint8_t>& b, size_t o) {
    return  static_cast<uint32_t>(b[o])        | (static_cast<uint32_t>(b[o + 1]) << 8) |
           (static_cast<uint32_t>(b[o + 2]) << 16) | (static_cast<uint32_t>(b[o + 3]) << 24);
}

LoadResult load_elf(Memory& mem, const std::vector<uint8_t>& f) {
    LoadResult r;
    r.was_elf = true;

    if (f.size() < 52) { r.error = "ELF demasiado corto"; return r; }
    if (f[4] != 1)     { r.error = "Solo se soporta ELF de 32 bits (ELFCLASS32)"; return r; }
    if (f[5] != 1)     { r.error = "Solo se soporta ELF little-endian"; return r; }

    uint32_t e_entry  = rd32(f, 24);
    uint32_t e_phoff  = rd32(f, 28);
    uint16_t e_phentsize = rd16(f, 42);
    uint16_t e_phnum     = rd16(f, 44);

    uint32_t loaded = 0;
    for (uint16_t i = 0; i < e_phnum; ++i) {
        size_t ph = e_phoff + static_cast<size_t>(i) * e_phentsize;
        if (ph + 32 > f.size()) break;
        uint32_t p_type   = rd32(f, ph + 0);
        uint32_t p_offset = rd32(f, ph + 4);
        uint32_t p_vaddr  = rd32(f, ph + 8);
        uint32_t p_filesz = rd32(f, ph + 16);
        uint32_t p_memsz  = rd32(f, ph + 20);
        if (p_type != 1) continue;

        for (uint32_t k = 0; k < p_filesz && (p_offset + k) < f.size(); ++k)
            mem.write_byte(p_vaddr + k, f[p_offset + k]);
        loaded += p_memsz;
    }

    r.ok = true;
    r.entry = e_entry;
    r.size = loaded;
    return r;
}

}
LoadResult load_program(Memory& mem, const std::string& path, uint32_t base_addr) {
    LoadResult r;
    std::ifstream in(path, std::ios::binary);
    if (!in) { r.error = "No se pudo abrir el archivo: " + path; return r; }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    if (data.empty()) { r.error = "El archivo esta vacio"; return r; }

    if (data.size() >= 4 && data[0] == 0x7F && data[1] == 'E' &&
        data[2] == 'L' && data[3] == 'F') {
        return load_elf(mem, data);
    }

    mem.load_bytes(base_addr, data);
    r.ok = true;
    r.entry = base_addr;
    r.size = static_cast<uint32_t>(data.size());
    return r;
}
