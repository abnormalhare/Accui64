#include "../inc/x64.hpp"
#include "../inc/debug.hpp"
#include <iostream>

static const char *r8_names[0x10] = {
    "AL",    "CL",    "DL",    "BL",
    "SPL",   "BPL",   "SIL",   "DIL",
    "R8B",   "R9B",  "R10B", "R11B",
   "R12B", "R13B", "R14B", "R15B",
};
static const char *r8h_names[0x10] = {
    "AH",    "CH",     "DH",    "BH",
    0
};
static const char *r16_names[0x10] = {
    "AX",    "CX",    "DX",    "BX",
    "SP",    "BP",    "SI",   "DI",
    "R8W",   "R9W",  "R10W", "R11W",
   "R12W", "R13W", "R14W", "R15W",
};
static const char *r32_names[0x10] = {
    "EAX",   "ECX",   "EDX",   "EBX",
    "ESP",   "EBP",   "ESI",   "EDI",
    "R8D",   "R9D",  "R10D", "R11D",
   "R12D", "R13D", "R14D", "R15D",
};
static const char *r64_names[0x10] = {
    "RAX",  "RCX",  "RDX",  "RBX",
    "RSP",  "RBP",  "RSI",  "RDI",
    "R8",   "R9",  "R10", "R11",
   "R12", "R13", "R14", "R15",
};
static const char *st_names[0x6] = {
    "ES", "CS", "SS", "DS", "FS", "GS",
};
static const char *mm_names[0x8] = {
    "MM0", "MM1", "MM2", "MM3",
    "MM4", "MM5", "MM6", "MM7",
};
static const char *xmm_names[0x8] = {
    "XMM0", "XMM1", "XMM2", "XMM3",
    "XMM4", "XMM5", "XMM6", "XMM7",
};

const char *getRegName(u8 idx, RegType type) {
    switch (type) {
        default: return "";

        case RegType::R8:  return r8_names [idx];
        case RegType::R8H: return r8h_names[idx];
        case RegType::R16: return r16_names[idx];
        case RegType::R32: return r32_names[idx];
        case RegType::R64: return r64_names[idx];
        case RegType::ST:  return st_names [idx];
        case RegType::MM:  return mm_names [idx];
        case RegType::XMM: return xmm_names[idx];
    }
}

const char *getRegPtrName(RegType type) {
    switch (type) {
        default: return "";

        case RegType::R8:
        case RegType::R8H: return "BYTE PTR";
        case RegType::R16: return "WORD PTR";
        case RegType::R32: return "DWORD PTR";
        case RegType::R64: return "QWORD PTR";
        case RegType::ST:  return "";
        case RegType::MM:  return "MMWORD PTR";
        case RegType::XMM: return "XMMWORD PTR";
    }
}

void debugPrintMem(ModRM *modrm, u32 disp) {
    if (modrm->_mod == 3) {
        std::cout << getRegName(modrm->_rm, modrm->rm_type);
        return;
    }

    std::cout << getRegPtrName(modrm->reg_type) << " [";

    if (!modrm->shouldUseSib()) {
        if (modrm->rm) {
            std::cout << getRegName(modrm->_rm, modrm->rm_type);
        }
    } else {
        if (modrm->sib.idx) {
            std::cout << getRegName(modrm->sib._idx, modrm->sib.idx_type) << "*" << modrm->sib.mul;
        }
        if (modrm->sib.base) {
            if (modrm->sib.idx) {
                std::cout << " + ";
            }
            std::cout << getRegName(modrm->sib._base, modrm->sib.base_type);
        }
    }
    if (modrm->disp != 0) {
        if (modrm->rm || modrm->sib.idx || modrm->sib.idx) {
            std::cout << " + ";
        }
        std::cout << disp;
    }
    std::cout << "]";
}

void debugPrintReg(ModRM *modrm, u32 disp) {
    std::cout << getRegName(modrm->_reg, modrm->reg_type);
}

void debugPrint(const char *name, ModRM *modrm, u32 disp, OpOrder order) {
    std::cout << name << " ";

    switch (order) {
        case RM_R:
            debugPrintMem(modrm, disp);
            std::cout << ", ";
            debugPrintReg(modrm, disp);
            break;
        
        case R_RM:
            debugPrintReg(modrm, disp);
            std::cout << ", ";
            debugPrintMem(modrm, disp);
            break;
        
        case R_VAL: break;
    }

    std::cout << std::endl;
}