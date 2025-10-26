#include "alu.cpp"
#include "x64.cpp"
#include "opcodes.cpp"
#include "ram.cpp"

int main(int argc, char *argv[]) {

    if (argc < 2) {
        std::cout << "USAGE: accui64.exe [FILENAME]" << std::endl;
        return 1;
    }

    RAM::load(argv[1]);
    if (RAM::data == nullptr) return 1;

    CPU *cpu = new CPU();
    cpu->run();

    return 0;
}