#include "../inc/ram.hpp"
#include <immintrin.h>
#include <iostream>
#include <variant>
#include "../inc/x64.hpp"

CPU::CPU() {
    this->extra_info = std::unordered_map<const char *, u8>();
    this->extra_info.clear();

    this->running = true;

    this->setupRegs();
}

void CPU::setupRegs() {
    for (int i = 0; i < 0x10; i++) {
        this->regs[i] = 0;
    }
    this->IP->r = 0xFFF0;

    for (int i = 0; i < 6; i++) {
        this->st_regs[i].selector = 0;
        this->st_regs[i].limit = 0xFFFF;
        this->st_regs[i].base = 0;
        this->st_regs[i].attr = 0;
    }
    this->CS->selector = 0xF000;
    this->CS->base = 0xFFFF0000;

    RFLAGS._0 = 1;

    *CR0 = { 0 };
    CR0->et = 1;
    CR0->mp = 1;
    CR0->ne = 1;

    *CR2 = 0;
    *CR3 = { 0 };
    *CR4 = { 0 };

    *DR0 = 0;
    *DR1 = 0;
    *DR2 = 0;
    *DR3 = 0;
    *DR6 = 0xFFFF0FF0;
    
    *DR7 = { 0 };
    DR7->dr0_size = 1;

    LDTR = 0;
    TR = 0;
    GDTR.addr = 0;
    IDTR.addr = 0;

    for (int i = 0; i < 8; i++) {
        this->mm_regs[i] = 0;
    }
    for (int i = 0; i < 16; i++) {
        this->xm_regs[i].n256[0] = _mm256_setzero_si256();
        this->xm_regs[i].n256[1] = _mm256_setzero_si256();
    }

    IA32_EFER = { 0 };
}

void CPU::run() {
    this->extra_info.insert({"rex", 0x00});

    int i = 0;
    while (this->running) {
        std::cout << "---------------------------" << std::endl << std::endl;
        std::cout << "EIP: " << std::hex << (int)(CS->base + IP->e) << std::endl;

        this->runStep();
        
        std::cout << std::endl;
    }
}

void CPU::runStep() {
    this->curr_inst = this->read();

    bool dont_clear = (this->*CPU::opcode_table[this->curr_inst])();

    if (!dont_clear) {
        this->extra_info.clear();
        this->extra_info.insert({"rex", 0x00});
    }
}

inline u8 getMask(u8 val, u8 start, u8 end) {
    u8 mask = ((2 << start) - 1);
    if (end != 0) {
        mask -= ((2 << (end - 1)) - 1);
    }
    return (val & mask) >> end;
}

void CPU::determineModRMMod3(ModRM *modrm, RegType type) {
    u8 rmidx  = ((this->extra_info["rex"] & REXBit::B) << 3) | modrm->_rm ;
    u8 regidx = ((this->extra_info["rex"] & REXBit::R) << 3) | modrm->_reg;
    
    switch (type) {
        case RegType::R8: case RegType::R8H:
            modrm->reg_type = modrm->rm_type  = RegType::R8;

            if (modrm->_reg >= 4 && modrm->_reg < 8) {
                modrm->_reg -= 4; modrm->reg_type = RegType::R8H;
            }
            modrm->rm  = &this->regs[rmidx ];
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::R16: case RegType::R32: case RegType::R64:
            modrm->reg_type = modrm->rm_type  = RegType::R32;

            if (this->extra_info.contains("op")) { // TODO order
                modrm->reg_type = modrm->rm_type  = RegType::R16;
            }
            if (this->extra_info["rex"] & REXBit::W) {
                modrm->reg_type = modrm->rm_type  = RegType::R64;
            }
            modrm->rm  = &this->regs[rmidx ];
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::ST:
            modrm->reg_type = modrm->rm_type  = RegType::ST;

            modrm->rm  = &this->st_regs[modrm->_rm ];
            modrm->reg = &this->st_regs[modrm->_reg];
            break;
        
        case RegType::MM:
            modrm->reg_type = modrm->rm_type  = RegType::MM;
            
            modrm->rm  = &this->mm_regs[modrm->_rm ];
            modrm->reg = &this->mm_regs[modrm->_reg];
            break;
        
        case RegType::XMM:
            modrm->reg_type = modrm->rm_type  = RegType::XMM;
            
            modrm->rm  = &this->xm_regs[rmidx ];
            modrm->reg = &this->xm_regs[regidx];
            break;
    }
}

void CPU::determineModRMMod0to2(ModRM *modrm, RegType type) {
    u8 rmidx  = ((this->extra_info["rex"] & REXBit::B) << 3) | modrm->_rm ;
    u8 regidx = ((this->extra_info["rex"] & REXBit::R) << 3) | modrm->_reg;

    std::cout << "MODRM: MOD " << (int)modrm->_mod << " | REG " << (int)regidx << " | RM " << (int)rmidx << std::endl;

    modrm->rm_type = RegType::R64;
    if (this->extra_info.contains("ad")) {
        modrm->rm_type = RegType::R32;
    }

    switch (type) {
        case RegType::R8: case RegType::R8H:
            modrm->reg_type = RegType::R8;

            if (modrm->_reg >= 4 && modrm->_reg < 8 && !this->extra_info.contains("rex")) {
                modrm->_reg -= 4; modrm->reg_type = RegType::R8H;
            }
            modrm->rm  = &this->regs[rmidx ];
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::R16: case RegType::R32: case RegType::R64:
            modrm->reg_type = RegType::R32;

            if (this->extra_info.contains("op")) { // TODO order
                modrm->reg_type = RegType::R16;
            }
            if (this->extra_info["rex"] & REXBit::W) {
                modrm->reg_type = RegType::R64;
            }
            modrm->rm  = &this->regs[rmidx ];
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::ST:
            modrm->reg_type = RegType::ST;

            modrm->rm  = &this->st_regs[rmidx  & 7];
            modrm->reg = &this->st_regs[regidx & 7];
            break;
        
        case RegType::MM:
            modrm->reg_type = RegType::MM;
            
            modrm->rm  = &this->mm_regs[rmidx  & 7];
            modrm->reg = &this->mm_regs[regidx & 7];
            break;
        
        case RegType::XMM:
            modrm->reg_type = RegType::XMM;
            
            modrm->rm  = &this->xm_regs[rmidx ];
            modrm->reg = &this->xm_regs[regidx];
            break;
    }
}

void CPU::determineModRMSib(ModRM *modrm, RegType type, u8 sib) {
    modrm->sib._ss   = getMask(sib, 7, 6);
    modrm->sib._base = getMask(sib, 5, 3);
    modrm->sib._idx  = getMask(sib, 2, 0);
    
    u8 idxidx  = ((this->extra_info["rex"] & REXBit::X) << 3) | modrm->sib._idx;
    u8 baseidx = ((this->extra_info["rex"] & REXBit::B) << 3) | modrm->sib._base;
    u8 regidx = ((this->extra_info["rex"] & REXBit::R)  << 3) | modrm->_reg;

    modrm->sib.idx  = (idxidx == 4) ? nullptr : &this->regs[idxidx];
    modrm->sib.base = &this->regs[baseidx];
    modrm->sib.mul  = (modrm->sib._ss != 0) ? modrm->sib._ss * 2 : 1;

    if (modrm->sib._base == 5 && modrm->_mod == 0) {
        modrm->disp = 4;
        modrm->sib.base = nullptr;
    }
    

    modrm->sib.idx_type  = RegType::R64;
    modrm->sib.base_type = RegType::R64;
    if (this->extra_info.contains("ad")) {
        modrm->sib.idx_type  = RegType::R32;
        modrm->sib.base_type = RegType::R32;
    }

    switch (type) {
        case RegType::R8: case RegType::R8H:
            modrm->reg_type = RegType::R8;

            if (modrm->_reg >= 4 && modrm->_reg < 8 && this->extra_info["rex"] == 0x00) {
                modrm->_reg -= 4; modrm->reg_type = RegType::R8H;
            }
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::R16: case RegType::R32: case RegType::R64:
            modrm->reg_type = RegType::R32;

            if (this->extra_info.contains("op")) { // TODO order
                modrm->reg_type = RegType::R16;
            }
            if (this->extra_info["rex"] & REXBit::W) {
                modrm->reg_type = RegType::R64;
            }
            modrm->reg = &this->regs[regidx];
            break;
        
        case RegType::ST:
            modrm->reg_type = RegType::ST;
            modrm->reg = &this->st_regs[modrm->_reg];
            break;
        
        case RegType::MM:
            modrm->reg_type = RegType::MM;
            modrm->reg = &this->mm_regs[modrm->_reg];
            break;
        
        case RegType::XMM:
            modrm->reg_type = RegType::XMM;
            modrm->reg = &this->xm_regs[regidx];
            break;
    }
}

ModRM *CPU::getModRM16(RegType type) {
    u8 val = this->read();
    ModRM *modrm = new ModRM(
        getMask(val, 7, 6),
        getMask(val, 5, 3),
        getMask(val, 2, 0)
    );

    modrm->rm = nullptr;
    modrm->disp = modrm->_mod % 3;

    bool mod3 = false;
    if (modrm->_mod == 3) {
        mod3 = true;
        modrm->rm = &this->regs[modrm->_rm];

        goto detType;
    }

    switch (modrm->_rm) {
        case 0: modrm->sib.idx = BX; modrm->sib.base = SI; break;
        case 1: modrm->sib.idx = BX; modrm->sib.base = DI; break;
        case 2: modrm->sib.idx = BP; modrm->sib.base = SI; break;
        case 3: modrm->sib.idx = BP; modrm->sib.base = DI; break;
        case 4: modrm->rm = SI; break;
        case 5: modrm->rm = DI; break;
        case 6: modrm->rm = BP; break;
        case 7: modrm->rm = BX; break;
    }
    if (modrm->_mod == 0 && modrm->_rm == 6) {
        modrm->rm = nullptr;
        modrm->disp = 2;
    }
    
    modrm->rm_type  = RegType::R16;
    modrm->sib.idx_type  = RegType::R16;
    modrm->sib.base_type = RegType::R16;
    if (this->extra_info.contains("ad")) {
        modrm->rm_type  = RegType::R32;
        modrm->sib.idx_type  = RegType::R32;
        modrm->sib.base_type = RegType::R32;
    }

detType:
    switch (type) {
        default: break;

        case RegType::R8: case RegType::R8H:
            modrm->reg_type = RegType::R8;

            if (modrm->_reg >= 4 && modrm->_reg < 8) {
                modrm->_reg -= 4; modrm->reg_type = RegType::R8H;
            }
            modrm->reg = &this->regs[modrm->_reg];
            break;

        case RegType::R16: case RegType::R32:
            modrm->reg_type = RegType::R16;

            if (this->extra_info.contains("op")) {
                modrm->reg_type = RegType::R32;
            }
            modrm->reg = &this->regs[modrm->_reg];
    }

    if (mod3) {
        modrm->rm_type = modrm->reg_type;
    }

    return modrm;
}

ModRM *CPU::getModRM32(RegType type) {
    u8 val = this->read();
    ModRM *modrm = new ModRM(
        getMask(val, 7, 6),
        getMask(val, 5, 3),
        getMask(val, 2, 0)
    );

    switch (modrm->_mod) {
        default: modrm->disp = 0; break;
        case 1:  modrm->disp = 1; break;
        case 2:  modrm->disp = 4; break;
    }

    if (modrm->_mod == 3) {
        this->determineModRMMod3(modrm, type);
        return modrm;
    }

    if (modrm->_rm == 4) {
        this->determineModRMSib(modrm, type, this->read());
        return modrm;
    }

    if (modrm->_rm == 5 && modrm->_mod == 0) {
        modrm->rm = IP;
        modrm->disp = 4;
    }

    this->determineModRMMod0to2(modrm, type);
    return modrm;
}

ModRM *CPU::getModRM(RegType type) {
    if (!CR0->pe) {
        return this->getModRM16(type);
    } else {
        return this->getModRM32(type);
    }
}

u64 CPU::getModRMPtr(ModRM *modrm, u32 &disp) {
    u64 val = 0;

    if (!modrm->shouldUseSib()) {
        val += regToMaxSize(this->toReg(modrm->rm)->get(modrm->rm_type));
    } else {
        if (modrm->sib.idx) {
            val += regToMaxSize(this->toReg(modrm->sib.idx)->get(modrm->sib.idx_type));
            val *= modrm->sib.mul;
        }
        if (modrm->sib.base) {
            val += regToMaxSize(this->toReg(modrm->sib.base)->get(modrm->sib.base_type));
        }
    }

    switch (modrm->disp) {
        case 0: break;
        case 1: val += disp = this->getVal8();  break;
        case 2: val += disp = this->getVal16(); break;
        case 4: val += disp = this->getVal32(); break;
    }

    return val;
}

u8 CPU::read() {
    u8 ret = RAM::read(CS->base + IP->e);
    IP->x++;

    return ret;
}

void CPU::write(u64 addr, u8 val) {
    RAM::write(addr, val);
}

void CPU::writeReg(u64 addr, Reg *reg, RegType type) {
    auto val = reg->get(type);
    std::visit([&](auto write) {
        using T = decltype(write);

        for (int i = 0; i < sizeof(T); i++) {
            this->write(addr + i, write >> (i * 8));
        }
    }, val);
}

bool CPU::HALT() {
    this->running = false;
    return false;
}

static constexpr std::array<bool (CPU::*)(), 0x100> make_opcode_table() {
    std::array<bool (CPU::*)(), 0x100> t{};

    t[0x00] = &CPU::OP_00; t[0x01] = &CPU::OP_01; t[0x02] = &CPU::OP_02; t[0x03] = &CPU::OP_03;
    t[0x04] = &CPU::OP_04; t[0x05] = &CPU::OP_05; t[0x06] = &CPU::OP_06; t[0x07] = &CPU::OP_07;
    t[0x08] = &CPU::OP_08; t[0x09] = &CPU::OP_09; t[0x0A] = &CPU::OP_0A; t[0x0B] = &CPU::OP_0B;
    t[0x0C] = &CPU::OP_0C; t[0x0D] = &CPU::OP_0D; t[0x0E] = &CPU::OP_0E; t[0x0F] = &CPU::OP_0F;
    t[0x10] = &CPU::OP_10; t[0x11] = &CPU::OP_11; t[0x12] = &CPU::OP_12; t[0x13] = &CPU::OP_13;
    t[0x14] = &CPU::OP_14; t[0x15] = &CPU::OP_15; t[0x16] = &CPU::OP_16; t[0x17] = &CPU::OP_17;
    t[0x18] = &CPU::OP_18; t[0x19] = &CPU::OP_19; t[0x1A] = &CPU::OP_1A; t[0x1B] = &CPU::OP_1B;
    t[0x1C] = &CPU::OP_1C; t[0x1D] = &CPU::OP_1D; t[0x1E] = &CPU::OP_1E; t[0x1F] = &CPU::OP_1F;
    t[0x20] = &CPU::OP_20; t[0x21] = &CPU::OP_21; t[0x22] = &CPU::OP_22; t[0x23] = &CPU::OP_23;
    t[0x24] = &CPU::OP_24; t[0x25] = &CPU::OP_25; t[0x26] = &CPU::OP_26; t[0x27] = &CPU::OP_27;
    t[0x28] = &CPU::OP_28; t[0x29] = &CPU::OP_29; t[0x2A] = &CPU::OP_2A; t[0x2B] = &CPU::OP_2B;
    t[0x2C] = &CPU::OP_2C; t[0x2D] = &CPU::OP_2D; t[0x2E] = &CPU::OP_2E; t[0x2F] = &CPU::OP_2F;
    t[0x30] = &CPU::OP_30; t[0x31] = &CPU::OP_31; t[0x32] = &CPU::OP_32; t[0x33] = &CPU::OP_33;
    t[0x34] = &CPU::OP_34; t[0x35] = &CPU::OP_35; t[0x36] = &CPU::OP_36; t[0x37] = &CPU::OP_37;
    t[0x38] = &CPU::OP_38; t[0x39] = &CPU::OP_39; t[0x3A] = &CPU::OP_3A; t[0x3B] = &CPU::OP_3B;
    t[0x3C] = &CPU::OP_3C; t[0x3D] = &CPU::OP_3D; t[0x3E] = &CPU::OP_3E; t[0x3F] = &CPU::OP_3F;
    t[0x40] = &CPU::OP_40; t[0x41] = &CPU::OP_41; t[0x42] = &CPU::OP_42; t[0x43] = &CPU::OP_43;
    t[0x44] = &CPU::OP_44; t[0x45] = &CPU::OP_45; t[0x46] = &CPU::OP_46; t[0x47] = &CPU::OP_47;
    t[0x48] = &CPU::OP_48; t[0x49] = &CPU::OP_49; t[0x4A] = &CPU::OP_4A; t[0x4B] = &CPU::OP_4B;
    t[0x4C] = &CPU::OP_4C; t[0x4D] = &CPU::OP_4D; t[0x4E] = &CPU::OP_4E; t[0x4F] = &CPU::OP_4F;
    t[0x50] = &CPU::OP_50; t[0x51] = &CPU::OP_51; t[0x52] = &CPU::OP_52; t[0x53] = &CPU::OP_53;
    t[0x54] = &CPU::OP_54; t[0x55] = &CPU::OP_55; t[0x56] = &CPU::OP_56; t[0x57] = &CPU::OP_57;
    t[0x58] = &CPU::OP_58; t[0x59] = &CPU::OP_59; t[0x5A] = &CPU::OP_5A; t[0x5B] = &CPU::OP_5B;
    t[0x5C] = &CPU::OP_5C; t[0x5D] = &CPU::OP_5D; t[0x5E] = &CPU::OP_5E; t[0x5F] = &CPU::OP_5F;
    t[0x60] = &CPU::OP_60; t[0x61] = &CPU::OP_61; t[0x62] = &CPU::OP_62; t[0x63] = &CPU::OP_63;
    t[0x64] = &CPU::OP_64; t[0x65] = &CPU::OP_65; t[0x66] = &CPU::OP_66; t[0x67] = &CPU::OP_67;
    t[0x68] = &CPU::OP_68; t[0x69] = &CPU::OP_69; t[0x6A] = &CPU::OP_6A; t[0x6B] = &CPU::OP_6B;
    t[0x6C] = &CPU::OP_6C; t[0x6D] = &CPU::OP_6D; t[0x6E] = &CPU::OP_6E; t[0x6F] = &CPU::OP_6F;
    t[0x70] = &CPU::OP_70; t[0x71] = &CPU::OP_71; t[0x72] = &CPU::OP_72; t[0x73] = &CPU::OP_73;
    t[0x74] = &CPU::OP_74; t[0x75] = &CPU::OP_75; t[0x76] = &CPU::OP_76; t[0x77] = &CPU::OP_77;
    t[0x78] = &CPU::OP_78; t[0x79] = &CPU::OP_79; t[0x7A] = &CPU::OP_7A; t[0x7B] = &CPU::OP_7B;
    t[0x7C] = &CPU::OP_7C; t[0x7D] = &CPU::OP_7D; t[0x7E] = &CPU::OP_7E; t[0x7F] = &CPU::OP_7F;
    t[0x80] = &CPU::OP_80; t[0x81] = &CPU::OP_81; t[0x82] = &CPU::OP_82; t[0x83] = &CPU::OP_83;
    t[0x84] = &CPU::OP_84; t[0x85] = &CPU::OP_85; t[0x86] = &CPU::OP_86; t[0x87] = &CPU::OP_87;
    t[0x88] = &CPU::OP_88; t[0x89] = &CPU::OP_89; t[0x8A] = &CPU::OP_8A; t[0x8B] = &CPU::OP_8B;
    t[0x8C] = &CPU::OP_8C; t[0x8D] = &CPU::OP_8D; t[0x8E] = &CPU::OP_8E; t[0x8F] = &CPU::OP_8F;
    t[0x90] = &CPU::OP_90; t[0x91] = &CPU::OP_91; t[0x92] = &CPU::OP_92; t[0x93] = &CPU::OP_93;
    t[0x94] = &CPU::OP_94; t[0x95] = &CPU::OP_95; t[0x96] = &CPU::OP_96; t[0x97] = &CPU::OP_97;
    t[0x98] = &CPU::OP_98; t[0x99] = &CPU::OP_99; t[0x9A] = &CPU::OP_9A; t[0x9B] = &CPU::OP_9B;
    t[0x9C] = &CPU::OP_9C; t[0x9D] = &CPU::OP_9D; t[0x9E] = &CPU::OP_9E; t[0x9F] = &CPU::OP_9F;
    t[0xA0] = &CPU::OP_A0; t[0xA1] = &CPU::OP_A1; t[0xA2] = &CPU::OP_A2; t[0xA3] = &CPU::OP_A3;
    t[0xA4] = &CPU::OP_A4; t[0xA5] = &CPU::OP_A5; t[0xA6] = &CPU::OP_A6; t[0xA7] = &CPU::OP_A7;
    t[0xA8] = &CPU::OP_A8; t[0xA9] = &CPU::OP_A9; t[0xAA] = &CPU::OP_AA; t[0xAB] = &CPU::OP_AB;
    t[0xAC] = &CPU::OP_AC; t[0xAD] = &CPU::OP_AD; t[0xAE] = &CPU::OP_AE; t[0xAF] = &CPU::OP_AF;
    t[0xB0] = &CPU::OP_B0; t[0xB1] = &CPU::OP_B1; t[0xB2] = &CPU::OP_B2; t[0xB3] = &CPU::OP_B3;
    t[0xB4] = &CPU::OP_B4; t[0xB5] = &CPU::OP_B5; t[0xB6] = &CPU::OP_B6; t[0xB7] = &CPU::OP_B7;
    t[0xB8] = &CPU::OP_B8; t[0xB9] = &CPU::OP_B9; t[0xBA] = &CPU::OP_BA; t[0xBB] = &CPU::OP_BB;
    t[0xBC] = &CPU::OP_BC; t[0xBD] = &CPU::OP_BD; t[0xBE] = &CPU::OP_BE; t[0xBF] = &CPU::OP_BF;
    t[0xC0] = &CPU::OP_C0; t[0xC1] = &CPU::OP_C1; t[0xC2] = &CPU::OP_C2; t[0xC3] = &CPU::OP_C3;
    t[0xC4] = &CPU::OP_C4; t[0xC5] = &CPU::OP_C5; t[0xC6] = &CPU::OP_C6; t[0xC7] = &CPU::OP_C7;
    t[0xC8] = &CPU::OP_C8; t[0xC9] = &CPU::OP_C9; t[0xCA] = &CPU::OP_CA; t[0xCB] = &CPU::OP_CB;
    t[0xCC] = &CPU::OP_CC; t[0xCD] = &CPU::OP_CD; t[0xCE] = &CPU::OP_CE; t[0xCF] = &CPU::OP_CF;
    t[0xD0] = &CPU::OP_D0; t[0xD1] = &CPU::OP_D1; t[0xD2] = &CPU::OP_D2; t[0xD3] = &CPU::OP_D3;
    t[0xD4] = &CPU::OP_D4; t[0xD5] = &CPU::OP_D5; t[0xD6] = &CPU::OP_D6; t[0xD7] = &CPU::OP_D7;
    t[0xD8] = &CPU::OP_D8; t[0xD9] = &CPU::OP_D9; t[0xDA] = &CPU::OP_DA; t[0xDB] = &CPU::OP_DB;
    t[0xDC] = &CPU::OP_DC; t[0xDD] = &CPU::OP_DD; t[0xDE] = &CPU::OP_DE; t[0xDF] = &CPU::OP_DF;
    t[0xE0] = &CPU::OP_E0; t[0xE1] = &CPU::OP_E1; t[0xE2] = &CPU::OP_E2; t[0xE3] = &CPU::OP_E3;
    t[0xE4] = &CPU::OP_E4; t[0xE5] = &CPU::OP_E5; t[0xE6] = &CPU::OP_E6; t[0xE7] = &CPU::OP_E7;
    t[0xE8] = &CPU::OP_E8; t[0xE9] = &CPU::OP_E9; t[0xEA] = &CPU::OP_EA; t[0xEB] = &CPU::OP_EB;
    t[0xEC] = &CPU::OP_EC; t[0xED] = &CPU::OP_ED; t[0xEE] = &CPU::OP_EE; t[0xEF] = &CPU::OP_EF;
    t[0xF0] = &CPU::OP_F0; t[0xF1] = &CPU::OP_F1; t[0xF2] = &CPU::OP_F2; t[0xF3] = &CPU::OP_F3;
    t[0xF4] = &CPU::OP_F4; t[0xF5] = &CPU::OP_F5; t[0xF6] = &CPU::OP_F6; t[0xF7] = &CPU::OP_F7;
    t[0xF8] = &CPU::OP_F8; t[0xF9] = &CPU::OP_F9; t[0xFA] = &CPU::OP_FA; t[0xFB] = &CPU::OP_FB;
    t[0xFC] = &CPU::OP_FC; t[0xFD] = &CPU::OP_FD; t[0xFE] = &CPU::OP_FE; t[0xFF] = &CPU::OP_FF;
    return t;
}

const std::array<bool (CPU::*)(), 0x100> CPU::opcode_table = make_opcode_table();

u8 CPU::getVal8() {
    return this->read();
}

u16 CPU::getVal16() {
    return this->read() + (this->read() << 8);
}

u32 CPU::getVal32() {
    return this->read() + (this->read() << 8) + (this->read() << 16) + (this->read() << 24);
}

bool CPU::checkExceptions(u64 ptr, const std::vector<ExceptionType> &exceptions) {
    return false;
}