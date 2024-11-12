#include <iostream>
#include <fstream>

#include "rax.h"

using namespace std;

int main(int argc, char** argv)
{
    // if (argc == 1)
    // {
    //     cerr << "\e[93muse:\e[0m " << argv[0] << " file\n";
    //     return 1;
    // }

    // if (argc > 2)
    // {
    //     cerr << "\e[91merror:\e[0m more than one input file specified\n";
    //     return 1;
    // }

    // ifstream file(argv[1]);

    // if (!file.is_open())
    // {
    //     cerr << "\e[91merror:\e[0m could not open file\n";
    //     exit(EXIT_FAILURE);
    // }

    // Assembler rax;
    // string line;

    // while (getline(file, line))
    //     rax.assemble(line);

    // file.close();

    // rax.output();

    Assembler rax;

    Instruction inst;
    inst.mnemonic = "cli";
    inst.operands = {};

    rax.encode(inst);

    return 0;
}