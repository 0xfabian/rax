#include <iostream>
#include <fstream>

#include "tokenizer.h"

using namespace std;

int main(int argc, char** argv)
{
    TokenStream ts("mov rax, qword ptr [rbx + 3 * rbx + _start - 10]");

    while (ts)
    {
        cout << ts[0].str << "\n";

        ts.advance();
    }
}