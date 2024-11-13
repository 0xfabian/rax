#include <iostream>
#include <fstream>

#include "parser.h"

using namespace std;

int main(int argc, char** argv)
{
    TokenStream ts("main: mov rax, qword ptr [rbx + 3 * rbx + _start - 10]");

    string label;

    if (parse_label(ts, label))
        cout << "label: " << label << endl;

    Instruction inst;

    if (parse_instruction(ts, inst))
    {
        cout << "mnemonic: " << inst.menmonic << endl;

        for (size_t i = 0; i < inst.operands.size(); i++)
            cout << "operand " << i << ": " << inst.operands[i].type << endl;
    }
}