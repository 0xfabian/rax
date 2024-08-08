#include <iostream>
#include <fstream>

#include <tokenizer.h>
#include <assembler.h>

using namespace std;

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        cerr << "use: " << argv[0] << " file\n";
        exit(EXIT_FAILURE);
    }

    ifstream file(argv[1]);

    if (!file.is_open())
    {
        cerr << "error: could not open file '" << argv[1] << "'\n";
        exit(EXIT_FAILURE);
    }

    Assembler as;
    string line;
    size_t line_number = 1;
    size_t errors = 0;

    while (getline(file, line))
    {
        vector<Token> tokens = get_tokens(line);

        try
        {
            as.parse_tokens(tokens);
        }
        catch (const std::exception& e)
        {
            errors++;
            std::cerr << line_number << ": \e[91merror:\e[0m " << e.what() << '\n';
        }

        line_number++;
    }

    file.close();

    if (!errors)
        as.dump();

    return 0;
}