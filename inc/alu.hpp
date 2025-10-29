#pragma once

#include "reg.hpp"
#include "x64.hpp"
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

void add(CPU *cpu, RegType type, const Reg *a, const Reg *b, Reg *result);
void shl(CPU *cpu, RegType type, const Reg *a, const Reg *b, Reg *result);
void xorF(CPU *cpu, RegType type, const Reg *a, const Reg *b, Reg *result);