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
    SegReg st_regs[6];
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

    bool HALT();

    ModRM *getModRM(RegType type);
    u64 getModRMPtr(ModRM *modrm, u32 &disp);

    u8 read();
    void write(u64 addr, u8 val);
    void writeReg(u64 addr, Reg *reg, RegType type);
    
    u8 getVal8();
    u16 getVal16();
    u32 getVal32();

    bool checkExceptions(u64 ptr, const std::vector<ExceptionType> &exceptions);

private:
    ModRM *getModRM16(RegType type);
    ModRM *getModRM32(RegType type);
    
    void determineModRMMod3(ModRM *modrm, RegType type);
    void determineModRMMod0to2(ModRM *modrm, RegType type);
    void determineModRMSib(ModRM *modrm, RegType type, u8 sib);

    static const std::array<bool (CPU::*)(), 0x100> opcode_table;
    static const std::array<bool (CPU::*)(), 0x100> opcode_table_0F;
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

    SegReg *ES = &st_regs[0];
    SegReg *CS = &st_regs[1];
    SegReg *SS = &st_regs[2];
    SegReg *DS = &st_regs[3];
    SegReg *FS = &st_regs[4];
    SegReg *GS = &st_regs[5];

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
    
    bool OP_00(); bool OP_01(); bool OP_02(); bool OP_03(); bool OP_04(); bool OP_05(); bool OP_06(); bool OP_07();
    bool OP_08(); bool OP_09(); bool OP_0A(); bool OP_0B(); bool OP_0C(); bool OP_0D(); bool OP_0E(); bool OP_0F();
    bool OP_10(); bool OP_11(); bool OP_12(); bool OP_13(); bool OP_14(); bool OP_15(); bool OP_16(); bool OP_17();
    bool OP_18(); bool OP_19(); bool OP_1A(); bool OP_1B(); bool OP_1C(); bool OP_1D(); bool OP_1E(); bool OP_1F();
    bool OP_20(); bool OP_21(); bool OP_22(); bool OP_23(); bool OP_24(); bool OP_25(); bool OP_26(); bool OP_27();
    bool OP_28(); bool OP_29(); bool OP_2A(); bool OP_2B(); bool OP_2C(); bool OP_2D(); bool OP_2E(); bool OP_2F();
    bool OP_30(); bool OP_31(); bool OP_32(); bool OP_33(); bool OP_34(); bool OP_35(); bool OP_36(); bool OP_37();
    bool OP_38(); bool OP_39(); bool OP_3A(); bool OP_3B(); bool OP_3C(); bool OP_3D(); bool OP_3E(); bool OP_3F();
    bool OP_40(); bool OP_41(); bool OP_42(); bool OP_43(); bool OP_44(); bool OP_45(); bool OP_46(); bool OP_47();
    bool OP_48(); bool OP_49(); bool OP_4A(); bool OP_4B(); bool OP_4C(); bool OP_4D(); bool OP_4E(); bool OP_4F();
    bool OP_50(); bool OP_51(); bool OP_52(); bool OP_53(); bool OP_54(); bool OP_55(); bool OP_56(); bool OP_57();
    bool OP_58(); bool OP_59(); bool OP_5A(); bool OP_5B(); bool OP_5C(); bool OP_5D(); bool OP_5E(); bool OP_5F();
    bool OP_60(); bool OP_61(); bool OP_62(); bool OP_63(); bool OP_64(); bool OP_65(); bool OP_66(); bool OP_67();
    bool OP_68(); bool OP_69(); bool OP_6A(); bool OP_6B(); bool OP_6C(); bool OP_6D(); bool OP_6E(); bool OP_6F();
    bool OP_70(); bool OP_71(); bool OP_72(); bool OP_73(); bool OP_74(); bool OP_75(); bool OP_76(); bool OP_77();
    bool OP_78(); bool OP_79(); bool OP_7A(); bool OP_7B(); bool OP_7C(); bool OP_7D(); bool OP_7E(); bool OP_7F();
    bool OP_80(); bool OP_81(); bool OP_82(); bool OP_83(); bool OP_84(); bool OP_85(); bool OP_86(); bool OP_87();
    bool OP_88(); bool OP_89(); bool OP_8A(); bool OP_8B(); bool OP_8C(); bool OP_8D(); bool OP_8E(); bool OP_8F();
    bool OP_90(); bool OP_91(); bool OP_92(); bool OP_93(); bool OP_94(); bool OP_95(); bool OP_96(); bool OP_97();
    bool OP_98(); bool OP_99(); bool OP_9A(); bool OP_9B(); bool OP_9C(); bool OP_9D(); bool OP_9E(); bool OP_9F();
    bool OP_A0(); bool OP_A1(); bool OP_A2(); bool OP_A3(); bool OP_A4(); bool OP_A5(); bool OP_A6(); bool OP_A7();
    bool OP_A8(); bool OP_A9(); bool OP_AA(); bool OP_AB(); bool OP_AC(); bool OP_AD(); bool OP_AE(); bool OP_AF();
    bool OP_B0(); bool OP_B1(); bool OP_B2(); bool OP_B3(); bool OP_B4(); bool OP_B5(); bool OP_B6(); bool OP_B7();
    bool OP_B8(); bool OP_B9(); bool OP_BA(); bool OP_BB(); bool OP_BC(); bool OP_BD(); bool OP_BE(); bool OP_BF();
    bool OP_C0(); bool OP_C1(); bool OP_C2(); bool OP_C3(); bool OP_C4(); bool OP_C5(); bool OP_C6(); bool OP_C7();
    bool OP_C8(); bool OP_C9(); bool OP_CA(); bool OP_CB(); bool OP_CC(); bool OP_CD(); bool OP_CE(); bool OP_CF();
    bool OP_D0(); bool OP_D1(); bool OP_D2(); bool OP_D3(); bool OP_D4(); bool OP_D5(); bool OP_D6(); bool OP_D7();
    bool OP_D8(); bool OP_D9(); bool OP_DA(); bool OP_DB(); bool OP_DC(); bool OP_DD(); bool OP_DE(); bool OP_DF();
    bool OP_E0(); bool OP_E1(); bool OP_E2(); bool OP_E3(); bool OP_E4(); bool OP_E5(); bool OP_E6(); bool OP_E7();
    bool OP_E8(); bool OP_E9(); bool OP_EA(); bool OP_EB(); bool OP_EC(); bool OP_ED(); bool OP_EE(); bool OP_EF();
    bool OP_F0(); bool OP_F1(); bool OP_F2(); bool OP_F3(); bool OP_F4(); bool OP_F5(); bool OP_F6(); bool OP_F7();
    bool OP_F8(); bool OP_F9(); bool OP_FA(); bool OP_FB(); bool OP_FC(); bool OP_FD(); bool OP_FE(); bool OP_FF();
    
    bool OP_0F_00(); bool OP_0F_01(); bool OP_0F_02(); bool OP_0F_03(); bool OP_0F_04(); bool OP_0F_05(); bool OP_0F_06(); bool OP_0F_07();
    bool OP_0F_08(); bool OP_0F_09(); bool OP_0F_0A(); bool OP_0F_0B(); bool OP_0F_0C(); bool OP_0F_0D(); bool OP_0F_0E(); bool OP_0F_0F();
    bool OP_0F_10(); bool OP_0F_11(); bool OP_0F_12(); bool OP_0F_13(); bool OP_0F_14(); bool OP_0F_15(); bool OP_0F_16(); bool OP_0F_17();
    bool OP_0F_18(); bool OP_0F_19(); bool OP_0F_1A(); bool OP_0F_1B(); bool OP_0F_1C(); bool OP_0F_1D(); bool OP_0F_1E(); bool OP_0F_1F();
    bool OP_0F_20(); bool OP_0F_21(); bool OP_0F_22(); bool OP_0F_23(); bool OP_0F_24(); bool OP_0F_25(); bool OP_0F_26(); bool OP_0F_27();
    bool OP_0F_28(); bool OP_0F_29(); bool OP_0F_2A(); bool OP_0F_2B(); bool OP_0F_2C(); bool OP_0F_2D(); bool OP_0F_2E(); bool OP_0F_2F();
    bool OP_0F_30(); bool OP_0F_31(); bool OP_0F_32(); bool OP_0F_33(); bool OP_0F_34(); bool OP_0F_35(); bool OP_0F_36(); bool OP_0F_37();
    bool OP_0F_38(); bool OP_0F_39(); bool OP_0F_3A(); bool OP_0F_3B(); bool OP_0F_3C(); bool OP_0F_3D(); bool OP_0F_3E(); bool OP_0F_3F();
    bool OP_0F_40(); bool OP_0F_41(); bool OP_0F_42(); bool OP_0F_43(); bool OP_0F_44(); bool OP_0F_45(); bool OP_0F_46(); bool OP_0F_47();
    bool OP_0F_48(); bool OP_0F_49(); bool OP_0F_4A(); bool OP_0F_4B(); bool OP_0F_4C(); bool OP_0F_4D(); bool OP_0F_4E(); bool OP_0F_4F();
    bool OP_0F_50(); bool OP_0F_51(); bool OP_0F_52(); bool OP_0F_53(); bool OP_0F_54(); bool OP_0F_55(); bool OP_0F_56(); bool OP_0F_57();
    bool OP_0F_58(); bool OP_0F_59(); bool OP_0F_5A(); bool OP_0F_5B(); bool OP_0F_5C(); bool OP_0F_5D(); bool OP_0F_5E(); bool OP_0F_5F();
    bool OP_0F_60(); bool OP_0F_61(); bool OP_0F_62(); bool OP_0F_63(); bool OP_0F_64(); bool OP_0F_65(); bool OP_0F_66(); bool OP_0F_67();
    bool OP_0F_68(); bool OP_0F_69(); bool OP_0F_6A(); bool OP_0F_6B(); bool OP_0F_6C(); bool OP_0F_6D(); bool OP_0F_6E(); bool OP_0F_6F();
    bool OP_0F_70(); bool OP_0F_71(); bool OP_0F_72(); bool OP_0F_73(); bool OP_0F_74(); bool OP_0F_75(); bool OP_0F_76(); bool OP_0F_77();
    bool OP_0F_78(); bool OP_0F_79(); bool OP_0F_7A(); bool OP_0F_7B(); bool OP_0F_7C(); bool OP_0F_7D(); bool OP_0F_7E(); bool OP_0F_7F();
    bool OP_0F_80(); bool OP_0F_81(); bool OP_0F_82(); bool OP_0F_83(); bool OP_0F_84(); bool OP_0F_85(); bool OP_0F_86(); bool OP_0F_87();
    bool OP_0F_88(); bool OP_0F_89(); bool OP_0F_8A(); bool OP_0F_8B(); bool OP_0F_8C(); bool OP_0F_8D(); bool OP_0F_8E(); bool OP_0F_8F();
    bool OP_0F_90(); bool OP_0F_91(); bool OP_0F_92(); bool OP_0F_93(); bool OP_0F_94(); bool OP_0F_95(); bool OP_0F_96(); bool OP_0F_97();
    bool OP_0F_98(); bool OP_0F_99(); bool OP_0F_9A(); bool OP_0F_9B(); bool OP_0F_9C(); bool OP_0F_9D(); bool OP_0F_9E(); bool OP_0F_9F();
    bool OP_0F_A0(); bool OP_0F_A1(); bool OP_0F_A2(); bool OP_0F_A3(); bool OP_0F_A4(); bool OP_0F_A5(); bool OP_0F_A6(); bool OP_0F_A7();
    bool OP_0F_A8(); bool OP_0F_A9(); bool OP_0F_AA(); bool OP_0F_AB(); bool OP_0F_AC(); bool OP_0F_AD(); bool OP_0F_AE(); bool OP_0F_AF();
    bool OP_0F_B0(); bool OP_0F_B1(); bool OP_0F_B2(); bool OP_0F_B3(); bool OP_0F_B4(); bool OP_0F_B5(); bool OP_0F_B6(); bool OP_0F_B7();
    bool OP_0F_B8(); bool OP_0F_B9(); bool OP_0F_BA(); bool OP_0F_BB(); bool OP_0F_BC(); bool OP_0F_BD(); bool OP_0F_BE(); bool OP_0F_BF();
    bool OP_0F_C0(); bool OP_0F_C1(); bool OP_0F_C2(); bool OP_0F_C3(); bool OP_0F_C4(); bool OP_0F_C5(); bool OP_0F_C6(); bool OP_0F_C7();
    bool OP_0F_C8(); bool OP_0F_C9(); bool OP_0F_CA(); bool OP_0F_CB(); bool OP_0F_CC(); bool OP_0F_CD(); bool OP_0F_CE(); bool OP_0F_CF();
    bool OP_0F_D0(); bool OP_0F_D1(); bool OP_0F_D2(); bool OP_0F_D3(); bool OP_0F_D4(); bool OP_0F_D5(); bool OP_0F_D6(); bool OP_0F_D7();
    bool OP_0F_D8(); bool OP_0F_D9(); bool OP_0F_DA(); bool OP_0F_DB(); bool OP_0F_DC(); bool OP_0F_DD(); bool OP_0F_DE(); bool OP_0F_DF();
    bool OP_0F_E0(); bool OP_0F_E1(); bool OP_0F_E2(); bool OP_0F_E3(); bool OP_0F_E4(); bool OP_0F_E5(); bool OP_0F_E6(); bool OP_0F_E7();
    bool OP_0F_E8(); bool OP_0F_E9(); bool OP_0F_EA(); bool OP_0F_EB(); bool OP_0F_EC(); bool OP_0F_ED(); bool OP_0F_EE(); bool OP_0F_EF();
    bool OP_0F_F0(); bool OP_0F_F1(); bool OP_0F_F2(); bool OP_0F_F3(); bool OP_0F_F4(); bool OP_0F_F5(); bool OP_0F_F6(); bool OP_0F_F7();
    bool OP_0F_F8(); bool OP_0F_F9(); bool OP_0F_FA(); bool OP_0F_FB(); bool OP_0F_FC(); bool OP_0F_FD(); bool OP_0F_FE(); bool OP_0F_FF();
};