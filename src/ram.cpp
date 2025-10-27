#include "../inc/ram.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace RAM {

u8 *data;

void load(const char *filename) {
    std::ifstream rom(filename);
    if (!rom) {
        return;
    }

    data = (u8 *)malloc(0x100000000 * sizeof(u8));
    if (!data) {
        rom.close();
        return;
    }

    rom.seekg(0, std::ios::end);
    size_t size = rom.tellg();
    rom.seekg(0, std::ios::beg);
    
    std::cout << "File Size: 0x" << std::hex << (int)size << std::endl;

    if (rom.read(reinterpret_cast<char *>(data + 0x100000000 - size), size)) {
        return;
    }

    rom.close();
}

u8 read(u64 addr) {
    return data[addr & 0xFFFFFFFF];
}

void write(u64 addr, u8 val) {
    data[addr & 0xFFFFFFFF] = val;
}

} // namespace RAM