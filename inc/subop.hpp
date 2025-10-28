#pragma once

#include "x64.hpp"

typedef bool (*SubOpFunc)(CPU *, ModRM *);

// C1 ops
bool OP_C1_0(CPU *cpu, ModRM *modrm);
bool OP_C1_1(CPU *cpu, ModRM *modrm);
bool OP_C1_2(CPU *cpu, ModRM *modrm);
bool OP_C1_3(CPU *cpu, ModRM *modrm);
bool OP_C1_4(CPU *cpu, ModRM *modrm);
bool OP_C1_5(CPU *cpu, ModRM *modrm);
bool OP_C1_6(CPU *cpu, ModRM *modrm);
bool OP_C1_7(CPU *cpu, ModRM *modrm);

extern SubOpFunc subop_c1_table[8];