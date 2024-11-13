#include <unordered_map>

#include "parser.h"

using namespace std;

bool parse_label(TokenStream& ts, string& label)
{
    if (ts.match({ REGULAR, COLON }))
    {
        label = ts[0].str;
        ts.advance(2);

        return true;
    }

    return false;
}

bool parse_instruction(TokenStream& ts, Instruction& inst)
{
    if (!ts.match(REGULAR))
        return false;

    inst.menmonic = ts[0].str;

    Operand op;

    do
    {
        ts.advance();

        if (parse_operand(ts, op))
            inst.operands.push_back(op);
        else
            break;

    } while (ts.match(COMMA));

    if (!ts.match(EOS))
        throw runtime_error("junk after instruction");

    return true;
}

bool parse_operand(TokenStream& ts, Operand& op)
{
    return parse_register(ts, op) || parse_memory(ts, op) || parse_immediate(ts, op);
}

unordered_map<string, int> register_map =
{
    {"rax", 0},
    {"rcx", 1},
    {"rdx", 2},
    {"rbx", 3},
    {"rsp", 0},
    {"rbp", 1},
    {"rsi", 2},
    {"rdi", 3},
};

bool parse_register(TokenStream& ts, Operand& op)
{
    if (!ts.match(REGULAR))
        return false;

    auto it = register_map.find(ts[0].str);

    if (it == register_map.end())
        return false;

    op.type = 2;
    op.reg = it->second;

    ts.advance();

    return true;
}

int is_size(const string& str)
{
    if (str == "byte")
        return 1;

    if (str == "word")
        return 2;

    if (str == "dword")
        return 4;

    if (str == "qword")
        return 8;

    return 0;
}

bool parse_memory_prefix(TokenStream& ts, int& size)
{
    if (!ts.match(REGULAR))
        return false;

    int res = is_size(ts[0].str);

    if (!res)
        return false;

    size = res;

    ts.advance();

    if (ts.match(REGULAR) && ts[0].str == "ptr")
        ts.advance();

    return true;
}

bool parse_memory(TokenStream& ts, Operand& op)
{
    int size = 0;
    bool has_size = parse_memory_prefix(ts, size);

    if (!ts.match(OPEN_BRACKET))
    {
        if (has_size)
            throw runtime_error("expected [ after memory prefix");
        else
            return false;
    }

    size_t i = 0;

    while (ts[i].type != EOS)
    {
        if (ts[i].type == CLOSE_BRACKET)
        {
            op.type = 3;

            if (has_size)
                op.type |= size << 8;

            ts.advance(i + 1);

            return true;
        }

        i++;
    }

    throw runtime_error("expected ] after effective address");
}

bool parse_immediate(TokenStream& ts, Operand& op)
{
    // should really be a constant expression

    if (!ts.match(NUMERIC))
        return false;

    op.type = 1;
    op.imm = 0;

    ts.advance();

    return true;
}