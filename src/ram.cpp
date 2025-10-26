#include "../inc/ram.hpp"
#include <cstdlib>
#include <fstream>

namespace RAM {

u8 *data;

void load(const char *filename) {
    std::ifstream rom(filename);
    if (rom.fail()) return;

    data = (u8 *)malloc(0x100000 * sizeof(u8));
    if (!data) {
        rom.close();
        return;
    }

    rom.seekg(0, std::ios::end);
    size_t size = rom.tellg();
    rom.seekg(0, std::ios::beg);

    rom.read(reinterpret_cast<char *>(data), size);

    rom.close();
}

u8 read(u64 addr) {
    return data[addr & 0xFFFFFFF];
}

void write(u64 addr, u8 val) {
    data[addr & 0xFFFFFFF] = val;
}

} // namespace RAM