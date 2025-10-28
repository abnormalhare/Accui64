#pragma once

#include "types.hpp"
#include "reg.hpp"
#include "x64.hpp"


enum OpOrder {
    RM_R,
    RM_VAL,
    R_RM,
    R_VAL,
};

const char *getRegName(u8 idx, RegType type);
const char *getRegPtrName(RegType type);
void debugPrintMem(ModRM *modrm, u32 disp);
void debugPrintReg(ModRM *modrm, u32 disp);
void debugPrint(const char *name, ModRM *modrm, u32 disp, u64 val, OpOrder order);