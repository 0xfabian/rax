#include <iostream>
#include <fstream>

#include "parser.h"

using namespace std;

int main(int argc, char** argv)
{
    string input;

    while (getline(cin, input))
    {
        cout << "> ";

        if (input == "q")
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
                    cout << "operand " << i << ": " << inst.operands[i].type << endl;
            }
        }
        catch (const exception& e)
        {
            cerr << "\e[91merror:\e[0m " << e.what() << '\n';
        }
    }

    return 0;
}