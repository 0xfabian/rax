#include <iostream>
#include <fstream>
#include <assembler.h>

using namespace std;

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        cerr << "\e[93muse:\e[0m " << argv[0] << " file\n";
        exit(EXIT_FAILURE);
    }
    else if (argc > 2)
    {
        cerr << "\e[91merror:\e[0m more than one input file specified\n";
        exit(EXIT_FAILURE);
    }

    ifstream file(argv[1]);

    if (!file.is_open())
    {
        cerr << "\e[91merror:\e[0m could not open file\n";
        exit(EXIT_FAILURE);
    }

    vector<string> lines;
    string line;

    while (getline(file, line))
        lines.push_back(line);

    file.close();

    Assembler as;

    if (as.assemble(lines) != 0)
        return 1;

    return 0;
}