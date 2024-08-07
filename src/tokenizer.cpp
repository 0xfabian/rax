#include <tokenizer.h>

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

void clean_line(string& line)
{
    size_t pos = line.find(";");

    if (pos != string::npos)
        line = line.substr(0, pos);

    replace_substring(line, ",", " , ");
    replace_substring(line, "[", " [ ");
    replace_substring(line, "]", " ] ");
    replace_substring(line, "+", " + ");
    replace_substring(line, "-", " - ");
    replace_substring(line, "*", " * ");
}

vector<string> register_map =
{
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "rip"
};

vector<string> simd_register_map =
{
    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
    "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"
};

vector<string> size_map =
{
    "byte", "word", "dword", "qword", "xmmword"
};

int get_register_size(const string& str)
{
    if (str.front() == 'r')
        return 8;

    if (str.front() == 'e')
        return 4;

    if (str.size() == 2)
        return (str.back() == 'l' || str.back() == 'h') ? 1 : 2;

    return 0;
}

int get_register_index(const string& str)
{
    for (int i = 0; i < register_map.size(); i++)
        if (str == register_map[i])
            return i;

    return -1;
}

bool is_register(const string& str)
{
    return get_register_index(str) != -1;
}

bool is_simd_register(const string& str)
{
    for (auto& reg : simd_register_map)
        if (str == reg)
            return true;

    return false;
}

bool is_size(const string& str)
{
    for (auto& sz : size_map)
        if (str == sz)
            return true;

    return false;
}

const char* digits = "0123456789abcdef";

bool is_number(const string& str)
{
    int base = 10;
    int i = 0;
    int j;

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
        for (j = 0; j < base; j++)
            if (str[i] == digits[j])
                break;

        if (j == base)
            return false;
    }

    return true;
}

bool is_label(const string& str)
{
    if (str.empty() || isdigit(str[0]))
        return false;

    for (auto ch : str)
        if (ch != '.' && ch != '_' && !isalnum(ch))
            return false;

    return true;
}

bool is_label_def(const string& str)
{
    if (str.back() != ':')
        return false;

    string label = str.substr(0, str.size() - 1);

    return is_label(label);
}

vector<Token> get_tokens(string line)
{
    clean_line(line);

    istringstream iss(line);
    vector<Token> tokens;
    string str;

    while (iss >> str)
    {
        TokenType type = OTHER;

        if (is_register(str))
            type = REGISTER;
        else if (is_simd_register(str))
            type = SIMD_REGISTER;
        else if (is_size(str))
            type = SIZE;
        else if (is_number(str))
            type = NUMERIC;
        else if (is_label_def(str))
            type = LABEL_DEFINITION;
        else if (is_label(str))
            type = LABEL;
        else if (str == "[")
            type = OPEN_BRACKET;
        else if (str == "]")
            type = CLOSE_BRACKET;
        else if (str == ",")
            type = COMMA;
        else if (str == "+" || str == "-")
            type = PLUS_MINUS;
        else if (str == "*")
            type = TIMES;

        tokens.push_back({ type, str });
    }

    return tokens;
}