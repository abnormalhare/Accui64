#pragma once

#include "types.hpp"

namespace RAM {

extern u8 *data;

void load(const char *filename);
u8 read(u64 addr);
void write(u64 addr, u8 val);

};