#pragma once

#include "types.hpp"
#include "reg.hpp"
#include <array>
#include <unordered_map>
#include <vector>

// reg = [idx * mul + base + disp]
struct SIB {
    Reg *idx;
    RegType idx_type;

    Reg *base;
    RegType base_type;

    u8 _ss, _base, _idx;
    u8 mul;
};

// reg = [rm + disp]
struct ModRM {
    void *rm;
    RegType rm_type;

    void *reg;
    RegType reg_type;

    u8 _mod, _reg, _rm;
    u8 disp;
    SIB sib;

    ModRM(u8 mod, u8 reg, u8 rm) {
        this->_mod = mod;
        this->_reg = reg;
        this->_rm = rm;

        this->rm = nullptr;
        this->reg = nullptr;
    }
    
    bool shouldUseSib() {
        return this->rm == nullptr;
    }
};

class CPU {
public:
    bool running;

    u8 curr_inst;
    std::unordered_map<const char *, u8> extra_info;
    
    Reg regs[17];
    u64 mm_regs[8];
    XMMReg xm_regs[16];
    u16 st_regs[6];
    u64 cr_regs[16];
    u32 db_regs[8];
    u32 tr_regs[8];

    Flags RFLAGS;

    GDTR GDTR;
    u16 LDTR;
    u16 TR;
    IDTR IDTR;
    IA32_EFER IA32_EFER;

    u32 FSBase  = 0xC0000100;
    u32 GSBase  = 0xC0000101;
    u32 KGSBase = 0xC0000102;

    CPU();
    void setupRegs();

    void run();
    void runStep();

    void HALT();

    ModRM *getModRM(RegType type);
    u64 getModRMPtr(ModRM *modrm, u32 &disp);

    u8 read();
    void write(u64 addr, u8 val);
    void writeReg(u64 addr, Reg *reg, RegType type);
    
    u8 getVal8();
    u16 getVal16();
    u32 getVal32();

    bool checkExceptions(u64 ptr, const std::vector<ExceptionType> &exceptions);

    void OP_00(); void OP_01(); void OP_02(); void OP_03(); void OP_04(); void OP_05(); void OP_06(); void OP_07();
    void OP_08(); void OP_09(); void OP_0A(); void OP_0B(); void OP_0C(); void OP_0D(); void OP_0E(); void OP_0F();
    void OP_10(); void OP_11(); void OP_12(); void OP_13(); void OP_14(); void OP_15(); void OP_16(); void OP_17();
    void OP_18(); void OP_19(); void OP_1A(); void OP_1B(); void OP_1C(); void OP_1D(); void OP_1E(); void OP_1F();
    void OP_20(); void OP_21(); void OP_22(); void OP_23(); void OP_24(); void OP_25(); void OP_26(); void OP_27();
    void OP_28(); void OP_29(); void OP_2A(); void OP_2B(); void OP_2C(); void OP_2D(); void OP_2E(); void OP_2F();
    void OP_30(); void OP_31(); void OP_32(); void OP_33(); void OP_34(); void OP_35(); void OP_36(); void OP_37();
    void OP_38(); void OP_39(); void OP_3A(); void OP_3B(); void OP_3C(); void OP_3D(); void OP_3E(); void OP_3F();
    void OP_40(); void OP_41(); void OP_42(); void OP_43(); void OP_44(); void OP_45(); void OP_46(); void OP_47();
    void OP_48(); void OP_49(); void OP_4A(); void OP_4B(); void OP_4C(); void OP_4D(); void OP_4E(); void OP_4F();
    void OP_50(); void OP_51(); void OP_52(); void OP_53(); void OP_54(); void OP_55(); void OP_56(); void OP_57();
    void OP_58(); void OP_59(); void OP_5A(); void OP_5B(); void OP_5C(); void OP_5D(); void OP_5E(); void OP_5F();
    void OP_60(); void OP_61(); void OP_62(); void OP_63(); void OP_64(); void OP_65(); void OP_66(); void OP_67();
    void OP_68(); void OP_69(); void OP_6A(); void OP_6B(); void OP_6C(); void OP_6D(); void OP_6E(); void OP_6F();
    void OP_70(); void OP_71(); void OP_72(); void OP_73(); void OP_74(); void OP_75(); void OP_76(); void OP_77();
    void OP_78(); void OP_79(); void OP_7A(); void OP_7B(); void OP_7C(); void OP_7D(); void OP_7E(); void OP_7F();
    void OP_80(); void OP_81(); void OP_82(); void OP_83(); void OP_84(); void OP_85(); void OP_86(); void OP_87();
    void OP_88(); void OP_89(); void OP_8A(); void OP_8B(); void OP_8C(); void OP_8D(); void OP_8E(); void OP_8F();
    void OP_90(); void OP_91(); void OP_92(); void OP_93(); void OP_94(); void OP_95(); void OP_96(); void OP_97();
    void OP_98(); void OP_99(); void OP_9A(); void OP_9B(); void OP_9C(); void OP_9D(); void OP_9E(); void OP_9F();
    void OP_A0(); void OP_A1(); void OP_A2(); void OP_A3(); void OP_A4(); void OP_A5(); void OP_A6(); void OP_A7();
    void OP_A8(); void OP_A9(); void OP_AA(); void OP_AB(); void OP_AC(); void OP_AD(); void OP_AE(); void OP_AF();
    void OP_B0(); void OP_B1(); void OP_B2(); void OP_B3(); void OP_B4(); void OP_B5(); void OP_B6(); void OP_B7();
    void OP_B8(); void OP_B9(); void OP_BA(); void OP_BB(); void OP_BC(); void OP_BD(); void OP_BE(); void OP_BF();
    void OP_C0(); void OP_C1(); void OP_C2(); void OP_C3(); void OP_C4(); void OP_C5(); void OP_C6(); void OP_C7();
    void OP_C8(); void OP_C9(); void OP_CA(); void OP_CB(); void OP_CC(); void OP_CD(); void OP_CE(); void OP_CF();
    void OP_D0(); void OP_D1(); void OP_D2(); void OP_D3(); void OP_D4(); void OP_D5(); void OP_D6(); void OP_D7();
    void OP_D8(); void OP_D9(); void OP_DA(); void OP_DB(); void OP_DC(); void OP_DD(); void OP_DE(); void OP_DF();
    void OP_E0(); void OP_E1(); void OP_E2(); void OP_E3(); void OP_E4(); void OP_E5(); void OP_E6(); void OP_E7();
    void OP_E8(); void OP_E9(); void OP_EA(); void OP_EB(); void OP_EC(); void OP_ED(); void OP_EE(); void OP_EF();
    void OP_F0(); void OP_F1(); void OP_F2(); void OP_F3(); void OP_F4(); void OP_F5(); void OP_F6(); void OP_F7();
    void OP_F8(); void OP_F9(); void OP_FA(); void OP_FB(); void OP_FC(); void OP_FD(); void OP_FE(); void OP_FF();

private:
    void determineModRMMod3(ModRM *modrm, RegType type);
    void determineModRMMod0to2(ModRM *modrm, RegType type);
    void determineModRMSib(ModRM *modrm, RegType type, u8 sib);

    static const std::array<void (CPU::*)(), 0x100> opcode_table;
    void initBind();

public:
    Reg *toReg(void *reg) {
        return reinterpret_cast<Reg *>(reg);
    }
    
    Reg *AX  = &regs[ 0];
    Reg *CX  = &regs[ 1];
    Reg *DX  = &regs[ 2];
    Reg *BX  = &regs[ 3];
    Reg *SI  = &regs[ 4];
    Reg *DI  = &regs[ 5];
    Reg *SP  = &regs[ 6];
    Reg *BP  = &regs[ 7];
    Reg *R8  = &regs[ 8];
    Reg *R9  = &regs[ 9];
    Reg *R10 = &regs[10];
    Reg *R11 = &regs[11];
    Reg *R12 = &regs[12];
    Reg *R13 = &regs[13];
    Reg *R14 = &regs[14];
    Reg *R15 = &regs[15];
    Reg *IP  = &regs[16];

    u16 *ES = &st_regs[0];
    u16 *CS = &st_regs[1];
    u16 *SS = &st_regs[2];
    u16 *DS = &st_regs[3];
    u16 *FS = &st_regs[4];
    u16 *GS = &st_regs[5];

    CR0 *CR0 = (struct CR0 *)&cr_regs[0];
    u64 *CR2 = &cr_regs[2];
    CR3 *CR3 = (struct CR3 *)&cr_regs[3];
    CR4 *CR4 = (struct CR4 *)&cr_regs[4];
    CR8 *CR8 = (struct CR8 *)&cr_regs[8];

    u32 *DR0 = &db_regs[0];
    u32 *DR1 = &db_regs[1];
    u32 *DR2 = &db_regs[2];
    u32 *DR3 = &db_regs[3];
    u32 *DR6 = &db_regs[6];
    DR7 *DR7 = (struct DR7 *)&db_regs[7];
};