#include "../inc/alu.hpp"
#include "../inc/x64.hpp"

template<typename T>
static u8 get_parity(T res) {
    res ^= res >> 4;
    res ^= res >> 2;
    res ^= res >> 1;
    return !(res & 1);
}

void add(CPU *cpu, RegType type, const Reg *a, const Reg *b, Reg *result) {
    calcOp(cpu, type, a, b, result, [](CPU *cpu, auto a, auto b) {
        using T = decltype(a);
        
        T res = a + b;
        constexpr int bits = sizeof(T) * 8;

        u64 topbit = (1ULL << (bits - 1));

        cpu->RFLAGS.cf = (res < a);
        cpu->RFLAGS.pf = get_parity<T>(res);
        cpu->RFLAGS.af = ((a ^ b ^ res) & 0x10) != 0;
        cpu->RFLAGS.zf = (res == 0);
        cpu->RFLAGS.sf = (res >> (bits - 1)) & 1;
        cpu->RFLAGS.of = (((a ^ b) & topbit) == 0) && (((a ^ res) & topbit) != 0);

        return res;
    });
}

void sub(CPU *cpu, RegType type, const Reg *a, const Reg *b, Reg *result) {
    calcOp(cpu, type, a, b, result, [](CPU *cpu, auto a, auto b) {
        using T = decltype(a);
        
        T res = a - b;
        constexpr int bits = sizeof(T) * 8;

        u64 topbit = (1ULL << (bits - 1));

        cpu->RFLAGS.cf = (a < b);
        cpu->RFLAGS.pf = get_parity<T>(res);
        cpu->RFLAGS.af = ((a ^ b ^ res) & 0x10) != 0;
        cpu->RFLAGS.zf = (res == 0);
        cpu->RFLAGS.sf = (res >> (bits - 1)) & 1;
        cpu->RFLAGS.of = (((a ^ b) & topbit) == 0) && (((a ^ res) & topbit) != 0);

        return res;
    });
}

void xorF(CPU *cpu, RegType type, const Reg *a, const Reg *b, Reg *result) {
    calcOp(cpu, type, a, b, result, [](CPU *cpu, auto a, auto b) {
        using T = decltype(a);
        
        T res = a ^ b;
        constexpr int bits = sizeof(T) * 8;

        u64 topbit = (1ULL << (bits - 1));

        cpu->RFLAGS.cf = 0;
        cpu->RFLAGS.pf = get_parity<T>(res);
        cpu->RFLAGS.af = 0;
        cpu->RFLAGS.zf = (res == 0);
        cpu->RFLAGS.sf = (res >> (bits - 1)) & 1;
        cpu->RFLAGS.of = 0;

        return res;
    });
}

void shl(CPU *cpu, RegType type, const Reg *a, const Reg *b, Reg *result) {
    calcOp(cpu, type, a, b, result, [](CPU *cpu, auto a, auto b) {
        using T = decltype(a);
        constexpr int bits = sizeof(T) * 8;
        
        T res = a << (b - 1);
        u8 topbit = getMask(res, bits - 1, bits - 1) & 1;
        res <<= 1;

        cpu->RFLAGS.cf = topbit;
        cpu->RFLAGS.pf = get_parity<T>(res);
        cpu->RFLAGS.af = 0;
        cpu->RFLAGS.zf = (res == 0);
        cpu->RFLAGS.sf = getMask(res, bits - 1, bits - 1) & 1;
        if (b == 1) {
            cpu->RFLAGS.of = (cpu->RFLAGS.sf == cpu->RFLAGS.cf);
        }

        return res;
    });
}