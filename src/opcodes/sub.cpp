#include "../../inc/alu.hpp"
#include "../../inc/subop.hpp"
#include "../../inc/debug.hpp"
#include <iostream>

bool OP_C1_0(CPU *cpu, ModRM *modrm) {
    std::cout << "UNIMPLEMENTED OPCODE 0xC1 /0" << std::endl;
    
    return false;
}

bool OP_C1_1(CPU *cpu, ModRM *modrm) {
    std::cout << "UNIMPLEMENTED OPCODE 0xC1 /1" << std::endl;
    
    return false;
}

bool OP_C1_2(CPU *cpu, ModRM *modrm) {
    std::cout << "UNIMPLEMENTED OPCODE 0xC1 /2" << std::endl;
    
    return false;
}

bool OP_C1_3(CPU *cpu, ModRM *modrm) {
    std::cout << "UNIMPLEMENTED OPCODE 0xC1 /3" << std::endl;
    
    return false;
}

bool OP_C1_4(CPU *cpu, ModRM *modrm) {
    u32 disp;
    u64 ptr = cpu->getModRMPtr(modrm, disp);
    Reg *dst = (modrm->_mod == 3) ? cpu->toReg(modrm->rm) : new Reg(ptr);
    u8 sft = cpu->getVal8();
    Reg src = Reg();
    src.l = sft;

    shl(cpu, modrm->reg_type, dst, &src, dst);
    debugPrint("SHL", modrm, disp, sft, RM_VAL);

    if (modrm->_mod != 3) {
        cpu->writeReg(ptr, dst, modrm->reg_type);
        delete dst;
    }

    return false;
}

bool OP_C1_5(CPU *cpu, ModRM *modrm) {
    std::cout << "UNIMPLEMENTED OPCODE 0xC1 /5" << std::endl;
    
    return false;
}

bool OP_C1_6(CPU *cpu, ModRM *modrm) {
    std::cout << "UNIMPLEMENTED OPCODE 0xC1 /6" << std::endl;
    
    return false;
}

bool OP_C1_7(CPU *cpu, ModRM *modrm) {
    std::cout << "UNIMPLEMENTED OPCODE 0xC1 /7" << std::endl;
    
    return false;
}

SubOpFunc subop_c1_table[8] = {
    OP_C1_0, OP_C1_1, OP_C1_2, OP_C1_3,
    OP_C1_4, OP_C1_5, OP_C1_6, OP_C1_7
};