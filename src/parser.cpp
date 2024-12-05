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

    ts.advance();

    if (ts.match(EOS))
        return true;

    Operand op;

    if (!parse_operand(ts, op))
        throw runtime_error("expected operand after mnemonic");

    inst.operands.push_back(op);

    while (ts.match(COMMA))
    {
        ts.advance();

        if (!parse_operand(ts, op))
            throw runtime_error("expected operand after comma");

        inst.operands.push_back(op);
    }

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
    {"rax", 0}, {"rcx", 1}, {"rdx", 2}, {"rbx", 3}, {"rsp", 4}, {"rbp", 5}, {"rsi", 6}, {"rdi", 7},
    {"eax", 0}, {"ecx", 1}, {"edx", 2}, {"ebx", 3}, {"esp", 4}, {"ebp", 5}, {"esi", 6}, {"edi", 7},
    {"ax",  0}, {"cx",  1}, {"dx",  2}, {"bx",  3}, {"sp",  4}, {"bp",  5}, {"si",  6}, {"di",  7},
    {"al",  0}, {"cl",  1}, {"dl",  2}, {"bl",  3}, {"spl", 4}, {"bpl", 5}, {"sil", 6}, {"dil", 7},
                                                    {"ah",  4}, {"ch",  5}, {"dh",  6}, {"bh",  7},

    {"r8",  8}, {"r9",  9}, {"r10",  10}, {"r11",  11}, {"r12",  12}, {"r13",  13}, {"r14",  14}, {"r15",  15},
    {"r8d", 8}, {"r9d", 9}, {"r10d", 10}, {"r11d", 11}, {"r12d", 12}, {"r13d", 13}, {"r14d", 14}, {"r15d", 15},
    {"r8w", 8}, {"r9w", 9}, {"r10w", 10}, {"r11w", 11}, {"r12w", 12}, {"r13w", 13}, {"r14w", 14}, {"r15w", 15},
    {"r8b", 8}, {"r9b", 9}, {"r10b", 10}, {"r11b", 11}, {"r12b", 12}, {"r13b", 13}, {"r14b", 14}, {"r15b", 15},
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
    Constant constant;

    if (!parse_constant_sum(ts, constant))
        return false;

    op.type = 1;
    op.imm = constant.offset;
    op.symbol = constant.symbol;

    return true;
}

uint64_t prefix_stoull(const string& str)
{
    if (str.size() > 2 && str[0] == '0' && (str[1] == 'b' || str[1] == 'x'))
    {
        int base = (str[1] == 'b') ? 2 : 16;

        return stoull(str.substr(2), nullptr, base);
    }

    return stoull(str);
}

bool parse_constant_atom(TokenStream& ts, Constant& c)
{
    if (ts.match(NUMERIC))
    {
        c.offset = prefix_stoull(ts[0].str);
        ts.advance();

        return true;
    }

    if (ts.match(REGULAR))
    {
        c.symbol = ts[0].str;
        ts.advance();

        return true;
    }

    return false;
}

bool parse_constant_unary(TokenStream& ts, Constant& c)
{
    bool negative = false;

    if (ts.match(MINUS))
    {
        negative = true;
        ts.advance();
    }

    if (!parse_constant_atom(ts, c))
    {
        if (negative)
            throw runtime_error("expected constant expression after -");

        return false;
    }

    if (negative)
        c = -c;

    return true;
}

bool parse_constant_sum(TokenStream& ts, Constant& c)
{
    if (!parse_constant_unary(ts, c))
        return false;

    while (ts.match_any({ PLUS, MINUS }))
    {
        TokenType op = ts[0].type;

        ts.advance();

        Constant rhs;

        if (!parse_constant_unary(ts, rhs))
            throw runtime_error("expected constant expression after + or -");

        c = (op == PLUS) ? c + rhs : c - rhs;
    }

    return true;
}