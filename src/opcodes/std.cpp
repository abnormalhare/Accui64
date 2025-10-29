#include "../../inc/alu.hpp"
#include "../../inc/x64.hpp"
#include "../../inc/debug.hpp"

#include "../../inc/subop.hpp"

#include <ios>
#include <vector>

#include "0F.cpp"

bool CPU::OP_00() {
    ModRM *modrm = this->getModRM(RegType::R8);
    u32 disp;
    u64 ptr = this->getModRMPtr(modrm, disp);
    Reg *dst = new Reg(ptr);
    Reg *src = this->toReg(modrm->reg);

    if (this->checkExceptions(ptr, { ExceptionType::SS, GP, PF, AC, UD })) {
        return false;
    }

    add(this, modrm->reg_type, dst, src, dst);
    this->writeReg(ptr, dst, modrm->reg_type);

    debugPrint("ADD", modrm, disp, 0, RM_R);

    delete dst;
    return false;
}

bool CPU::OP_01() {
    ModRM *modrm = this->getModRM(RegType::R32);
    u32 disp;
    u64 ptr = this->getModRMPtr(modrm, disp);
    Reg *dst = new Reg(ptr);
    Reg *src = this->toReg(modrm->reg);

    if (this->checkExceptions(ptr, (const std::vector<ExceptionType>){ ExceptionType::SS, GP, PF, AC, UD })) {
        return false;
    }

    add(this, modrm->reg_type, dst, src, dst);
    this->writeReg(ptr, dst, modrm->reg_type);

    debugPrint("ADD", modrm, disp, 0, RM_R);

    delete dst;
    return false;
}

bool CPU::OP_02() {
    ModRM *modrm = this->getModRM(RegType::R8);
    u32 disp;
    Reg *dst = this->toReg(modrm->reg);
    Reg *src = new Reg(this->getModRMPtr(modrm, disp));

    add(this, modrm->reg_type, dst, src, dst);

    debugPrint("ADD", modrm, disp, 0, R_RM);

    delete src;
    return false;
}

bool CPU::OP_03() {
    ModRM *modrm = this->getModRM(RegType::R32);
    u32 disp;
    Reg *dst = this->toReg(modrm->reg);
    Reg *src = new Reg(this->getModRMPtr(modrm, disp));

    add(this, modrm->reg_type, dst, src, dst);

    debugPrint("ADD", modrm, disp,0,  R_RM);

    delete src;
    return false;
}

bool CPU::OP_04() {
    Reg *dst = AX;
    Reg *src = new Reg();
    src->l = this->getVal8();

    calcOp(this, RegType::R8, dst, src, dst, [](CPU *cpu, auto a, auto b) {
        return a + b;
    });

    std::cout << "ADD AL, " << (int)src->l << std::endl;  // Print AL since we're operating on 8-bit

    delete src;
    return false;
}

bool CPU::OP_05() {
    Reg *dst = AX;
    Reg *src = new Reg();
    RegType src_type;
    u32 val;

    if (this->extra_info.contains("op")) {
        src_type = RegType::R16;
        src->x = val = this->getVal16();
    } else if (this->extra_info["rex"] & REXBit::W) {
        src_type = RegType::R64;
        src->r = val = this->getVal32();
    } else {
        src_type = RegType::R32;
        src->e = val = this->getVal32();
    }

    calcOp(this, src_type, dst, src, dst, [](CPU *cpu, auto a, auto b) {
        return a + b;
    });

    std::cout << "ADD " << getRegName(0, src_type) << ", " << std::hex << val << std::endl;

    delete src;
    return false;
}

bool CPU::OP_0F() {
    return (this->*CPU::opcode_table_0F[this->read()])();
}

bool CPU::OP_29() {
    if (!CR0->pe) {
        ModRM *modrm = this->getModRM(RegType::R16);
        u32 disp;
        u64 ptr = this->getModRMPtr(modrm, disp);
        Reg *dst = new Reg(ptr);
        Reg *src = this->toReg(modrm->reg);

        sub(this, modrm->reg_type, dst, src, dst);
        this->writeReg(ptr, dst, modrm->reg_type);

        debugPrint("SUB", modrm, disp, 0, RM_R);

        delete dst;
    }
    return false;
}

bool CPU::OP_31() {
    if (!CR0->pe) {
        ModRM *modrm = this->getModRM(RegType::R16);
        u32 disp;
        u64 ptr = this->getModRMPtr(modrm, disp);
        Reg *dst = new Reg(ptr);
        Reg *src = this->toReg(modrm->reg);

        xorF(this, modrm->reg_type, dst, src, dst);
        this->writeReg(ptr, dst, modrm->reg_type);

        debugPrint("XOR", modrm, disp, 0, RM_R);

        delete dst;
    }

    return false;
}

bool CPU::OP_66() {
    this->extra_info.insert({"op", 1});

    return true;
}

bool CPU::OP_89() {
    if (!CR0->pe) {
        ModRM *modrm = this->getModRM(RegType::R16);
        u32 disp;
        u64 ptr = this->getModRMPtr(modrm, disp);
        Reg *dst = (modrm->_mod == 3) ? this->toReg(modrm->rm) : new Reg(ptr);
        Reg *src = this->toReg(modrm->reg);

        dst->set(modrm->reg_type, src->get(modrm->reg_type));

        debugPrint("MOV", modrm, disp, 0, RM_R);
        if (modrm->_mod != 3) {
            this->writeReg(ptr, dst, modrm->reg_type);
            delete dst;
        }
    }
    return false;
}

bool CPU::OP_8C() {
    if (!CR0->pe) {
        ModRM *modrm = this->getModRM(RegType::R16);
        u32 disp;
        u64 ptr = this->getModRMPtr(modrm, disp);
        Reg *dst = (modrm->_mod == 3) ? this->toReg(modrm->rm) : new Reg(ptr);
        SegReg *src = &this->st_regs[modrm->_reg];

        if (modrm->_mod != 3) {
            modrm->reg_type = RegType::R16;
        }

        switch (modrm->reg_type) {
            default: break;
            
            case RegType::R16: dst->set(modrm->reg_type, (u16)src->selector); break;
            case RegType::R32: dst->set(modrm->reg_type, src->selector); break;
        }
        

        modrm->reg_type = RegType::ST;
        debugPrint("MOV", modrm, disp, 0, RM_R);
        if (modrm->_mod != 3) {
            this->writeReg(ptr, dst, modrm->reg_type);
            delete dst;
        }
    }
    return false;
}

bool CPU::OP_BB() {
    if (!CR0->pe) {
        if (!this->extra_info.contains("op")) {
            u16 val = this->getVal16();
            BX->set(RegType::R16, val);
            
            std::cout << "MOV BX, " << std::hex << std::uppercase << (int)val << std::endl;
        } else {
            u32 val = this->getVal32();
            BX->set(RegType::R32, val);
            
            std::cout << "MOV EBX, " << std::hex << std::uppercase << (int)val << std::endl;
        }
    }
    return false;
}

bool CPU::OP_C1() {
    ModRM *modrm = this->getModRM(RegType::R16);
    return subop_c1_table[modrm->_reg](this, modrm);
}

bool CPU::OP_E9() {
    if (!CR0->pe) { // 16-bit signed jump: JMP YYXX / E9 XX YY
        s16 jumpVal = (s16)this->getVal16();

        IP->x = (s16)IP->x + jumpVal;

        std::cout << "JMP " << std::hex << jumpVal << std::endl;
    }
    return false;
}

bool CPU::OP_FA() {
    if (!CR0->pe) { // allowed
        RFLAGS.iF = 0;
    } else if (RFLAGS.iopl >= (CS->selector & 0b11)) {
        RFLAGS.iF = 0;
    } else if (CR4->vme || CR4->pvi) {
        RFLAGS.vif = 0;
    } else {
        // GP(0)
    }

    std::cout << "CLI" << std::endl;

    return false;
}

#define STUB_OP(hex) \
bool CPU::OP_##hex() { std::cout << "UNIMPLEMENTED OPCODE 0x" #hex << std::endl; return this->HALT(); }

STUB_OP(06)STUB_OP(07)STUB_OP(08)STUB_OP(09)STUB_OP(0A)STUB_OP(0B)STUB_OP(0C)STUB_OP(0D)STUB_OP(0E)
STUB_OP(10)STUB_OP(11)STUB_OP(12)STUB_OP(13)STUB_OP(14)STUB_OP(15)STUB_OP(16)STUB_OP(17)STUB_OP(18)
STUB_OP(19)STUB_OP(1A)STUB_OP(1B)STUB_OP(1C)STUB_OP(1D)STUB_OP(1E)STUB_OP(1F)STUB_OP(20)STUB_OP(21)
STUB_OP(22)STUB_OP(23)STUB_OP(24)STUB_OP(25)STUB_OP(26)STUB_OP(27)STUB_OP(28)STUB_OP(2A)
STUB_OP(2B)STUB_OP(2C)STUB_OP(2D)STUB_OP(2E)STUB_OP(2F)STUB_OP(30)STUB_OP(32)STUB_OP(33)STUB_OP(34)
STUB_OP(35)STUB_OP(36)STUB_OP(37)STUB_OP(38)STUB_OP(39)STUB_OP(3A)STUB_OP(3B)STUB_OP(3C)STUB_OP(3D)
STUB_OP(3E)STUB_OP(3F)STUB_OP(40)STUB_OP(41)STUB_OP(42)STUB_OP(43)STUB_OP(44)STUB_OP(45)STUB_OP(46)
STUB_OP(47)STUB_OP(48)STUB_OP(49)STUB_OP(4A)STUB_OP(4B)STUB_OP(4C)STUB_OP(4D)STUB_OP(4E)STUB_OP(4F)
STUB_OP(50)STUB_OP(51)STUB_OP(52)STUB_OP(53)STUB_OP(54)STUB_OP(55)STUB_OP(56)STUB_OP(57)STUB_OP(58)
STUB_OP(59)STUB_OP(5A)STUB_OP(5B)STUB_OP(5C)STUB_OP(5D)STUB_OP(5E)STUB_OP(5F)STUB_OP(60)STUB_OP(61)
STUB_OP(62)STUB_OP(63)STUB_OP(64)STUB_OP(65)STUB_OP(67)STUB_OP(68)STUB_OP(69)STUB_OP(6A)STUB_OP(6B)
STUB_OP(6C)STUB_OP(6D)STUB_OP(6E)STUB_OP(6F)STUB_OP(70)STUB_OP(71)STUB_OP(72)STUB_OP(73)STUB_OP(74)
STUB_OP(75)STUB_OP(76)STUB_OP(77)STUB_OP(78)STUB_OP(79)STUB_OP(7A)STUB_OP(7B)STUB_OP(7C)STUB_OP(7D)
STUB_OP(7E)STUB_OP(7F)STUB_OP(80)STUB_OP(81)STUB_OP(82)STUB_OP(83)STUB_OP(84)STUB_OP(85)STUB_OP(86)
STUB_OP(87)STUB_OP(88)STUB_OP(8A)STUB_OP(8B)STUB_OP(8D)STUB_OP(8E)STUB_OP(8F)STUB_OP(90)
STUB_OP(91)STUB_OP(92)STUB_OP(93)STUB_OP(94)STUB_OP(95)STUB_OP(96)STUB_OP(97)STUB_OP(98)STUB_OP(99)
STUB_OP(9A)STUB_OP(9B)STUB_OP(9C)STUB_OP(9D)STUB_OP(9E)STUB_OP(9F)STUB_OP(A0)STUB_OP(A1)STUB_OP(A2)
STUB_OP(A3)STUB_OP(A4)STUB_OP(A5)STUB_OP(A6)STUB_OP(A7)STUB_OP(A8)STUB_OP(A9)STUB_OP(AA)STUB_OP(AB)
STUB_OP(AC)STUB_OP(AD)STUB_OP(AE)STUB_OP(AF)STUB_OP(B0)STUB_OP(B1)STUB_OP(B2)STUB_OP(B3)STUB_OP(B4)
STUB_OP(B5)STUB_OP(B6)STUB_OP(B7)STUB_OP(B8)STUB_OP(B9)STUB_OP(BA)STUB_OP(BC)STUB_OP(BD)
STUB_OP(BE)STUB_OP(BF)STUB_OP(C0)STUB_OP(C2)STUB_OP(C3)STUB_OP(C4)STUB_OP(C5)STUB_OP(C6)
STUB_OP(C7)STUB_OP(C8)STUB_OP(C9)STUB_OP(CA)STUB_OP(CB)STUB_OP(CC)STUB_OP(CD)STUB_OP(CE)STUB_OP(CF)
STUB_OP(D0)STUB_OP(D1)STUB_OP(D2)STUB_OP(D3)STUB_OP(D4)STUB_OP(D5)STUB_OP(D6)STUB_OP(D7)STUB_OP(D8)
STUB_OP(D9)STUB_OP(DA)STUB_OP(DB)STUB_OP(DC)STUB_OP(DD)STUB_OP(DE)STUB_OP(DF)STUB_OP(E0)STUB_OP(E1)
STUB_OP(E2)STUB_OP(E3)STUB_OP(E4)STUB_OP(E5)STUB_OP(E6)STUB_OP(E7)STUB_OP(E8)STUB_OP(EA)STUB_OP(EB)
STUB_OP(EC)STUB_OP(ED)STUB_OP(EE)STUB_OP(EF)STUB_OP(F0)STUB_OP(F1)STUB_OP(F2)STUB_OP(F3)STUB_OP(F4)
STUB_OP(F5)STUB_OP(F6)STUB_OP(F7)STUB_OP(F8)STUB_OP(F9)STUB_OP(FB)STUB_OP(FC)STUB_OP(FD)STUB_OP(FE)
STUB_OP(FF)

#undef STUB_OP