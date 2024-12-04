#include <iostream>
#include <fstream>

#include "parser.h"

using namespace std;

int main(int argc, char** argv)
{
    string input;

    while (true)
    {
        cout << "> ";

        if (!getline(cin, input) || input == "q")
            break;

        TokenStream ts(input);

        string label;

        if (parse_label(ts, label))
            cout << "label: " << label << endl;

        Instruction inst;

        try
        {
            if (parse_instruction(ts, inst))
            {
                cout << "mnemonic: " << inst.menmonic << endl;

                for (size_t i = 0; i < inst.operands.size(); i++)
                {
                    if (inst.operands[i].type == 1)
                    {
                        cout << "imm: " << (int64_t)inst.operands[i].imm;

                        if (!inst.operands[i].symbol.empty())
                            cout << " (" << inst.operands[i].symbol << ")";

                        cout << endl;
                    }
                    else if (inst.operands[i].type == 2)
                        cout << "reg: " << inst.operands[i].reg << endl;
                    else
                        cout << "operand " << i << ": " << inst.operands[i].type << endl;
                }
            }
        }
        catch (const exception& e)
        {
            cerr << "\e[91merror:\e[0m " << e.what() << '\n';
        }
    }

    return 0;
}