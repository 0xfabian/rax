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

string clean_line(const string& line)
{
    string ret = line;

    remove_comment(ret);

    replace_substring(ret, ":", " : ");
    replace_substring(ret, ",", " , ");
    replace_substring(ret, "[", " [ ");
    replace_substring(ret, "]", " ] ");
    replace_substring(ret, "+", " + ");
    replace_substring(ret, "-", " - ");
    replace_substring(ret, "*", " * ");

    return ret;
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

const Token TokenStream::eos = { EOS, "" };

TokenStream::TokenStream(const std::string& line)
{
    string cleaned = clean_line(line);

    istringstream iss(cleaned);
    string str;

    while (iss >> str)
    {
        TokenType type = REGULAR;

        if (is_number(str))
            type = NUMERIC;
        else if (str == ":")
            type = COLON;
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
}

bool TokenStream::match(TokenType type)
{
    return operator[](0).type == type;
}

bool TokenStream::match(const std::vector<TokenType>& types)
{
    for (size_t i = 0; i < types.size(); i++)
        if (operator[](i).type != types[i])
            return false;

    return true;
}

bool TokenStream::match_any(const std::vector<TokenType>& types)
{
    for (auto type : types)
        if (match(type))
            return true;

    return false;
}

void TokenStream::advance(size_t n)
{
    pos = min(pos + n, tokens.size());
}

const Token& TokenStream::operator[](size_t i) const
{
    size_t index = pos + i;

    if (index < tokens.size())
        return tokens[index];

    return eos;
}

TokenStream::operator bool() const
{
    return pos < tokens.size();
}