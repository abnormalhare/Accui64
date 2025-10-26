#include "../inc/alu.hpp"
#include "../inc/x64.hpp"
#include <variant>

template <typename Func>
void calcOp(CPU *cpu, RegType type, const Reg *a, const Reg *b, Reg *result, Func func) {
    auto va = a->get(type);
    auto vb = b->get(type);

    std::visit([&](auto lhs, auto rhs) {
        using T = decltype(lhs);
        using U = decltype(rhs);

        if constexpr(std::is_same_v<T, U>) {
            result->set(type, static_cast<T>(func(cpu, lhs, rhs)));
        }
    }, va, vb);
}

void add(CPU *cpu, RegType type, const Reg *a, const Reg *b, Reg *result) {
    calcOp(cpu, type, a, b, result, [](CPU *cpu, auto a, auto b) {
        using T = decltype(a);
        
        T res = a + b;
        constexpr int bits = sizeof(T) * 8;

        u64 topbit = (1ULL << (bits - 1));
        u8 parity = (res ^ (res >> 4));
        parity ^= parity >> 2;
        parity ^= parity >> 1;

        cpu->RFLAGS.cf = (res < a);
        cpu->RFLAGS.pf = !(parity & 1);
        cpu->RFLAGS.af = ((a ^ b ^ res) & 0x10) != 0;
        cpu->RFLAGS.zf = (res == 0);
        cpu->RFLAGS.sf = (res >> (bits - 1)) & 1;
        cpu->RFLAGS.of = (((a ^ b) & topbit) == 0) && (((a ^ res) & topbit) != 0);

        return res;
    });
}