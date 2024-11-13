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
    return false;
}

bool parse_operand(TokenStream& ts, Operand& op)
{
    return parse_register(ts, op) || parse_memory(ts, op) || parse_immediate(ts, op);
}

bool parse_register(TokenStream& ts, Operand& op)
{
    return false;
}

bool parse_memory(TokenStream& ts, Operand& op)
{
    return false;
}

bool parse_immediate(TokenStream& ts, Operand& op)
{
    return false;
}