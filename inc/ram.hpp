#pragma once

#include "types.hpp"

class RAM {
public:
    static u8 data[0x10000000];

    static u8 read(u64 addr) {
        return RAM::data[addr & 0xFFFFFFF];
    }

    static void write(u64 addr, u8 val) {
        RAM::data[addr & 0xFFFFFFF] = val;
    }
};