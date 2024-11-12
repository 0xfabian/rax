#include "tokenizer.h"

using namespace std;

void replace_substring(string& str, const string& old, const string& rep)
{
    if (old.empty())
        return;

    size_t pos = str.find(old);

    while (pos != string::npos)
    {
        str.replace(pos, old.length(), rep);
        pos = str.find(old, pos + rep.length());
    }
}

void remove_comment(string& line)
{
    size_t pos = line.find(";");

    if (pos != string::npos)
        line = line.substr(0, pos);
}

void clean_line(string& line)
{
    remove_comment(line);

    replace_substring(line, ":", " : ");
    replace_substring(line, ",", " , ");
    replace_substring(line, "[", " [ ");
    replace_substring(line, "]", " ] ");
    replace_substring(line, "+", " + ");
    replace_substring(line, "-", " - ");
    replace_substring(line, "*", " * ");
}

const char* digits = "0123456789abcdef";

bool is_number(const string& str)
{
    int base = 10;
    size_t i = 0;

    if (str[0] == '0')
    {
        if (str.size() == 1)
            return true;

        if (str[1] != 'b' && str[1] != 'x')
            return false;

        base = (str[1] == 'b') ? 2 : 16;
        i = 2;
    }

    for (; i < str.size(); i++)
    {
        if (str[i] == '_')
            continue;

        int j;

        for (j = 0; j < base; j++)
            if (str[i] == digits[j])
                break;

        if (j == base)
            return false;
    }

    return true;
}

vector<Token> get_tokens(string line)
{
    clean_line(line);

    istringstream iss(line);
    vector<Token> tokens;
    string str;

    while (iss >> str)
    {
        TokenType type = REGULAR;

        if (is_number(str))
            type = NUMERIC;
        else if (str == ":")
            type = COMMA;
        else if (str == "[")
            type = OPEN_BRACKET;
        else if (str == "]")
            type = CLOSE_BRACKET;
        else if (str == ",")
            type = COMMA;
        else if (str == "+")
            type = PLUS;
        else if (str == "-")
            type = MINUS;
        else if (str == "*")
            type = TIMES;

        tokens.push_back({ type, str });
    }

    return tokens;
}