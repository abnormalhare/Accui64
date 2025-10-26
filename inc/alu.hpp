#pragma once

#include "reg.hpp"
#include "x64.hpp"

template <typename Func>
void calcOp(CPU *cpu, RegType type, const Reg *a, const Reg *b, Reg *result, Func func);

void add(CPU *cpu, RegType type, const Reg *a, const Reg *b, Reg *result);