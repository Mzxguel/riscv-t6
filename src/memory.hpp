#ifndef RISCV_MEMORY_HPP
#define RISCV_MEMORY_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>

class Memory {
public:
    static constexpr uint32_t PAGE_SIZE = 4096;

    uint8_t read_byte(uint32_t addr) const {
        auto it = pages_.find(addr / PAGE_SIZE);
        if (it == pages_.end()) return 0;
        return it->second[addr % PAGE_SIZE];
    }

    void write_byte(uint32_t addr, uint8_t value) {
        auto& page = pages_[addr / PAGE_SIZE];
        if (page.empty()) page.resize(PAGE_SIZE, 0);
        page[addr % PAGE_SIZE] = value;
    }

    uint16_t read_half(uint32_t addr) const {
        return  static_cast<uint16_t>(read_byte(addr)) |
               (static_cast<uint16_t>(read_byte(addr + 1)) << 8);
    }

    void write_half(uint32_t addr, uint16_t value) {
        write_byte(addr,     static_cast<uint8_t>(value & 0xFF));
        write_byte(addr + 1, static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    uint32_t read_word(uint32_t addr) const {
        return  static_cast<uint32_t>(read_byte(addr)) |
               (static_cast<uint32_t>(read_byte(addr + 1)) << 8)  |
               (static_cast<uint32_t>(read_byte(addr + 2)) << 16) |
               (static_cast<uint32_t>(read_byte(addr + 3)) << 24);
    }

    void write_word(uint32_t addr, uint32_t value) {
        write_byte(addr,     static_cast<uint8_t>(value & 0xFF));
        write_byte(addr + 1, static_cast<uint8_t>((value >> 8)  & 0xFF));
        write_byte(addr + 2, static_cast<uint8_t>((value >> 16) & 0xFF));
        write_byte(addr + 3, static_cast<uint8_t>((value >> 24) & 0xFF));
    }

    void load_bytes(uint32_t addr, const std::vector<uint8_t>& data) {
        for (size_t i = 0; i < data.size(); ++i)
            write_byte(addr + static_cast<uint32_t>(i), data[i]);
    }

    void clear() { pages_.clear(); }

private:
    std::unordered_map<uint32_t, std::vector<uint8_t>> pages_;
};

#endif