#pragma once

#include "types.hpp"
#include "ram.hpp"
#include <variant>

enum RegType {
    R8,
    R8H,
    R16,
    R32,
    R64,

    ST,
    MM,
    XMM,
};

enum REXBit {
    B = 1,
    X = 2,
    R = 4,
    W = 8,
};

enum ExceptionType {
    DE, DB, NMI, BP, OF, BR, UD, NM, DF, CSO,
    TS, NP, SS, GP, PF, _0, MF, AC, MC, XM,
    VE, CP, _1, HV, VC, SX, _3,
};

union Reg {
    u64 r;
    u16 x;
    u32 e;
    struct {
        u8 l;
        u8 h;
    };

    Reg() { this->r = 0; }

    Reg(u64 ptr) {
        this->r = RAM::read(ptr);
    }

    std::variant<u8, u16, u32, u64> get(RegType type) const {
        switch (type) {
            case RegType::R8:  return this->l;
            case RegType::R8H: return this->h;
            case RegType::R16: return this->x;
            case RegType::R32: return this->e;
            case RegType::R64: return this->r;
            default: return 0U;
        }
    }

    void set(RegType type, std::variant<u8, u16, u32, u64> val) {
        switch (type) {
            case RegType::R8:  this->l = std::get<u8 >(val); break;
            case RegType::R8H: this->h = std::get<u8 >(val); break;
            case RegType::R16: this->x = std::get<u16>(val); break;
            case RegType::R32: this->r = 0; this->e = std::get<u32>(val); break;
            case RegType::R64: this->r = std::get<u64>(val); break;
            default: break;
        }
    }
};

template <typename... Ts>
u64 regToMaxSize(const std::variant<Ts...> &var) {
    return std::visit([](auto val) -> u64 {
        return static_cast<u64>(val);
    }, var);
}

union SignedReg {
    struct {
        s8 l;
        s8 h;
    };
    s16 x;
    s32 e;
    s64 r;

    SignedReg *operator=(const Reg *from) {
        *this = *reinterpret_cast<const SignedReg *>(from);
        return this;
    }
};

union XMMReg {
    n256 n256[2];
    n128 n128[4];
    f128 f128[4];
    u64  n64 [8];
    f64  f64 [8];
    u32  n32 [16];
    f32  f32 [16];
};

struct Flags {
    u8 cf   : 1;
    u8 _0   : 1 = 1;
    u8 pf   : 1;
    u8 _1   : 1 = 0;
    u8 af   : 1;
    u8 zf   : 1;
    u8 sf   : 1;
    u8 tf   : 1;
    u8 iF   : 1;
    u8 df   : 1;
    u8 of   : 1;
    u8 iopl : 2;
    u8 nt   : 1;
    u8 _2   : 1 = 0;
    u8 rf   : 1;
    u8 vm   : 1;
    u8 ac   : 1;
    u8 vif  : 1;
    u8 vip  : 1;
    u8 id   : 1;
    u8 _3   : 4 = 0;
    u8 _4       = 0;
    u32 _5      = 0;
};

struct SegReg {
    u16 selector;
    u32 base;
    u32 limit;
    u16 attr;
};

struct CR0 {
    u8 pe  : 1;
    u8 mp  : 1;
    u8 em  : 1;
    u8 ts  : 1;
    u8 et  : 1;
    u8 ne  : 1;
    u8 _0  : 2 = 0;
    u8 _1      = 0;
    u8 wp  : 1;
    u8 _2  : 1 = 0;
    u8 am  : 1;
    u8 _3  : 5 = 0;
    u8 _4  : 5 = 0;
    u8 nw  : 1;
    u8 cd  : 1;
    u8 pg  : 1;
    u32 _5 = 0;
};

struct _sub_cr3 {
    u8 _0  : 3 = 0;
    u8 pwt : 1;
    u8 pcd : 1;
    u8 _1  : 3 = 0;
    u8 _2      = 0;
};

struct CR3 {
    union {
        _sub_cr3 pcide0;
        u16 pcide1 : 12;
    };
    u8 pml4_0 : 4;
    u16 pml4_1;
    u32 pml4_2;

    u64 getPML4() {
        u64 pml4 = this->pml4_0;
        pml4 += (u64)this->pml4_1 << 4;
        pml4 += (u64)this->pml4_2 << 20;
        return pml4;
    }
};

struct CR4 {
    u8 vme : 1;
    u8 pvi : 1;
    u8 tsd : 1;
    u8 de  : 1;
    u8 pse : 1;
    u8 pae : 1;
    u8 mce : 1;
    u8 pge : 1;
    u8 pce : 1;
    u8 osfxsr : 1;
    u8 osxmmexcpt : 1;
    u8 umip : 1;
    u8 la57 : 1;
    u8 vmxe : 1;
    u8 smxe : 1;
    u8 _0 : 1;
    u8 fsgsbase : 1;
    u8 pcide : 1;
    u8 osxsave : 1;
    u8 _1 : 1;
    u8 smep : 1;
    u8 smap : 1;
    u8 pke : 1;
    u8 cet : 1;
    u8 pks : 1;
    u16 _2 = 0;
    u32 _3 = 0;
};

struct CR8 {
    u64 val;

    void setPriority(u8 priority) {
        this->val &= ~0xF;
        this->val |= priority & 0xF;
    }

    void setReserved(u64 reserved) {
        this->val &= 0xF;
        this->val |= reserved << 4;
    }
};

struct IA32_EFER {
    u8 sce : 1;
    u8 _0 : 7;
    u8 lme : 1;
    u8 _1 : 1;
    u8 lma : 1;
    u8 nxe : 1;
};

struct DR7 {
    u8 lbp_dr0 : 1;
    u8 gbp_dr0 : 1;
    u8 lbp_dr1 : 1;
    u8 gbp_dr1 : 1;
    u8 lbp_dr2 : 1;
    u8 gbp_dr2 : 1;
    u8 lbp_dr3 : 1;
    u8 gbp_dr3 : 1;

    u8 dr0_cond : 2;
    u8 dr0_size : 2;
    u8 dr1_cond : 2;
    u8 dr1_size : 2;
    u8 dr2_cond : 2;
    u8 dr2_size : 2;
    u8 dr3_cond : 2;
    u8 dr3_size : 2;

    bool get_enable(int reg) const {
        switch (reg) {
            case 0: return lbp_dr0 | gbp_dr0;
            case 1: return lbp_dr1 | gbp_dr1;
            case 2: return lbp_dr2 | gbp_dr2;
            case 3: return lbp_dr3 | gbp_dr3;
            default: return false;
        }
    }
};

struct GDTR {
    u16 size;
    u64 addr;
};

struct IDTR {
    u16 size;
    u64 addr;
};