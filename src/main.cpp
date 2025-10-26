#include "alu.cpp"
#include "x64.cpp"
#include "opcodes.cpp"

u8 RAM::data[0x10000000] = {
    5, 77, 32, 0,
};

int main() {
    CPU *cpu = new CPU();
    cpu->run();
}