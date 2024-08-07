#include <assembler.h>

using namespace std;

void Assembler::parse_tokens(const vector<Token>& tokens)
{
    if (tokens.empty())
        return;

    cursor = tokens.begin();
    end = tokens.end();

    if (cursor->str == "section")
    {
        cursor++;

        if (cursor == end)
            throw runtime_error("expected name after section");

        if (cursor->type != LABEL)
            throw runtime_error("invalid section name '" + cursor->str + "'");

        if (cursor + 1 != end)
            throw runtime_error("junk after section " + cursor->str);

        sections.push_back({ cursor->str, {} });
    }
    else if (cursor->str == "db")
    {
        cursor++;

        if (sections.empty())
            throw runtime_error("db must be used inside a section");

        if (cursor == end)
            throw runtime_error("expected constant after db");

        while (cursor != end)
        {
            try
            {
                sections.back().bytes.push_back(stoi(cursor->str, nullptr, 16));
                cursor++;
            }
            catch (...)
            {
                throw runtime_error("unable to parse constant '" + cursor->str + "'");
            }
        }
    }
    else
    {
        for (auto& pattern : patterns)
        {
            cursor = tokens.begin();

            if (match_pattern(pattern))
                return;
        }

        throw runtime_error("unable to match instruction");
    }
}

void Assembler::dump()
{
    for (auto& sec : sections)
    {
        printf("section %s\n{\n    ", sec.name.c_str());

        for (int i = 0; i < sec.bytes.size(); i++)
        {
            if (i > 0 && i % 16 == 0)
                printf("\n    ");

            printf("%02x ", sec.bytes[i]);
        }

        printf("\n}\n");

        if (&sec != &sections.back())
            printf("\n");
    }
}

bool Assembler::match_seq(const std::vector<TokenType>& types)
{
    if (end - cursor < types.size())
        return false;

    for (int i = 0; i < types.size(); i++)
        if ((cursor + i)->type != types[i])
            return false;

    return true;
}

vector<string> split(const string& str)
{
    istringstream iss(str);
    vector<string> words;
    string word;

    while (iss >> word)
        words.push_back(word);

    return words;
}

Pattern parse_pattern(const string& pattern)
{
    Pattern ret;
    vector<string> words = split(pattern);

    ret.mnemonic = words[0];

    int i = 1;

    while (words[i] != ":")
        ret.operands.push_back(words[i++]);

    i++;

    ret.encoding = words[i++];

    while (i < words.size())
        ret.bytes.push_back(words[i++]);

    return ret;
}

bool Assembler::match_pattern(const string& pattern)
{
    Pattern p = parse_pattern(pattern);
    vector<Operand> operands;

    // doesn't handle __

    if (cursor->str != p.mnemonic)
        return false;

    cursor++;

    for (auto& op : p.operands)
        if (cursor == end || !match_operand(op, operands))
            return false;

    printf("%s\n", pattern.c_str());

    return true;
}

int get_operand_size(const string& op)
{
    switch (op.back())
    {
    case '8':   return 1;
    case '6':   return 2;
    case '2':   return 4;
    case '4':   return 8;
    default:    return 0;
    }
}

int get_signed_immediate_size(int64_t value)
{
    if (value == (int8_t)value)
        return 1;

    if (value == (int16_t)value)
        return 2;

    if (value == (int32_t)value)
        return 4;

    return 8;
}

int get_immediate_size(uint64_t value)
{
    if (value == (uint8_t)value)
        return 1;

    if (value == (uint16_t)value)
        return 2;

    if (value == (uint32_t)value)
        return 4;

    return 8;
}

bool Assembler::match_operand(const string& op, vector<Operand>& operands)
{
    int op_size = get_operand_size(op);

    if (is_register(op))
    {
        if (cursor->str == op)
        {
            cursor++;
            return true;
        }

        return false;
    }

    if (op == "r8" || op == "r16" || op == "r32" || op == "r64")
        return match_register(op_size, operands);

    if (op == "rm8" || op == "rm16" || op == "rm32" || op == "rm64")
        return match_register(op_size, operands) || match_prefix_memory(op_size, operands);

    if (op == "m8" || op == "m16" || op == "m32" || op == "m64")
        return match_prefix_memory(op_size, operands);

    if (op == "imm8" || op == "imm16" || op == "imm32" || op == "imm64")
        return match_immediate(op_size, operands);

    // else if (op == "rel8" || op == "rel32")
    // {

    // }
    // else if (op == "m")
    // {

    // }
    // else if (op == "m128")
    // {

    // }
    // else if (op == "xmm")
    // {

    // }

    return false;
}

bool Assembler::match_register(int size, vector<Operand>& operands)
{
    if (cursor->type != REGISTER)
        return false;

    if (cursor->str == "rip")
        return false;

    if (get_register_size(cursor->str) != size)
        return false;

    Operand op;
    op.is_register = true;
    op.size = size;
    op.base_reg = get_register_index(cursor->str);

    operands.push_back(op);

    cursor++;

    return true;
}

bool Assembler::match_memory_no_offset(int size, vector<Operand>& operands)
{
    // [ base ]
    if (!match_seq({ OPEN_BRACKET, REGISTER, CLOSE_BRACKET }))
        return false;

    Token reg = *(cursor + 1);

    if (get_register_size(reg.str) != 8)
        throw runtime_error("expected 64-bit register but found " + reg.str);

    Operand op;
    op.is_memory = true;
    op.is_sib = false;
    op.offset = 0;
    op.offset_size = 0;
    op.base_reg = get_register_index(reg.str);
    op.mod = 0;

    if (op.base_reg == RIP)
    {
        op.offset_size = 4;
        op.base_reg = RBP;
    }
    else if (op.base_reg == RSP)
    {
        op.is_sib = true;
        op.sib = 0b00100100;
    }
    else if (op.base_reg == RBP)
    {
        op.offset_size = 1;
        op.mod = 1;
    }

    operands.push_back(op);

    cursor += 3;

    return true;
}

uint64_t get_number(const Token& tok)
{
    if (tok.type != NUMERIC)
        throw runtime_error("get_number called on non-numeric token");

    if (tok.str.size() > 2)
    {
        if (tok.str[1] == 'x')
            return stoi(tok.str.substr(2), nullptr, 16);

        if (tok.str[1] == 'b')
            return stoi(tok.str.substr(2), nullptr, 2);
    }

    return stoi(tok.str);
}

bool Assembler::match_memory_with_offset(int size, vector<Operand>& operands)
{
    // [ base +- offset ]
    if (!match_seq({ OPEN_BRACKET, REGISTER, PLUS_MINUS, NUMERIC, CLOSE_BRACKET }))
        return false;

    Token reg = *(cursor + 1);
    Token sign = *(cursor + 2);
    Token offset = *(cursor + 3);

    if (get_register_size(reg.str) != 8)
        throw runtime_error("expected 64-bit register but found " + reg.str);

    Operand op;
    op.is_memory = true;
    op.is_sib = false;
    op.offset = (int32_t)get_number(offset) * ((sign.str == "+") ? 1 : -1);
    op.offset_size = get_signed_immediate_size(op.offset) > 1 ? 4 : 1;
    op.base_reg = get_register_index(reg.str);
    op.mod = 1 + (op.offset_size == 4);

    if (op.base_reg == RIP)
    {
        op.offset_size = 4;
        op.base_reg = RBP;
        op.mod = 0;
    }
    else if (op.base_reg == RSP)
    {
        op.is_sib = true;
        op.sib = 0b00100100;
    }

    operands.push_back(op);

    cursor += 5;

    return true;
}

bool Assembler::match_memory_sib_no_offset(int size, vector<Operand>& operands)
{
    // [ base + index * scale ]
    if (!match_seq({ OPEN_BRACKET, REGISTER, PLUS_MINUS, REGISTER, TIMES, NUMERIC, CLOSE_BRACKET }))
        return false;

    Token base = *(cursor + 1);
    Token index = *(cursor + 3);
    Token scale = *(cursor + 5);

    // if (match_by_order(tok, sib_no_offset, 7))
    // {
    //     token* base_tok = tok->next;
    //     token* index_tok = base_tok->next->next;
    //     token* scale_tok = index_tok->next->next;

    //     if (base_tok->next->index != 1)
    //         return ERROR_INVALID_SIB;

    //     if (base_tok->size != QWORD || index_tok->size != QWORD)
    //         return ERROR_REG_64;

    //     int scale = get_number(scale_tok->string);

    //     if (scale != 1 && scale != 2 && scale != 4 && scale != 8)
    //         return ERROR_INVALID_SCALE;

    //     reg base = (reg)base_tok->index;
    //     reg index = (reg)index_tok->index;

    //     if (base == RIP || index == RIP)
    //         return ERROR_INVALID_SIB;

    //     if (index == RSP)
    //         return ERROR_INVALID_SIB;

    //     op.is_memory = true;
    //     op.is_sib = true;
    //     op.offset = 0;
    //     op.offset_size = 0;
    //     op.base_reg = RSP;
    //     op.mod = 0;

    //     switch (scale)
    //     {
    //     case 1:
    //         scale = 0;
    //         break;
    //     case 2:
    //         scale = 1;
    //         break;
    //     case 4:
    //         scale = 2;
    //         break;
    //     case 8:
    //         scale = 3;
    //         break;
    //     }

    //     if (base == RBP)
    //     {
    //         op.mod = 1;
    //         op.offset_size = 1;
    //     }

    //     op.sib = (scale << 6) + ((index & 7) << 3) + (base & 7);

    //     return 0;
    // }
}

bool Assembler::match_memory(int size, vector<Operand>& operands)
{
    return match_memory_no_offset(size, operands) || match_memory_with_offset(size, operands);

    // token_type sib_no_offset[] =
    // {
    //     // [ base + index * scale ]
    //     OPEN_SQUARE_BRACKET,
    //     REGISTER,
    //     PLUS_MINUS_SIGN,
    //     REGISTER,
    //     TIMES_SIGN,
    //     NUMERIC,
    //     CLOSED_SQUARE_BRACKET
    // };

    // if (match_by_order(tok, sib_no_offset, 7))
    // {
    //     token* base_tok = tok->next;
    //     token* index_tok = base_tok->next->next;
    //     token* scale_tok = index_tok->next->next;

    //     if (base_tok->next->index != 1)
    //         return ERROR_INVALID_SIB;

    //     if (base_tok->size != QWORD || index_tok->size != QWORD)
    //         return ERROR_REG_64;

    //     int scale = get_number(scale_tok->string);

    //     if (scale != 1 && scale != 2 && scale != 4 && scale != 8)
    //         return ERROR_INVALID_SCALE;

    //     reg base = (reg)base_tok->index;
    //     reg index = (reg)index_tok->index;

    //     if (base == RIP || index == RIP)
    //         return ERROR_INVALID_SIB;

    //     if (index == RSP)
    //         return ERROR_INVALID_SIB;

    //     op.is_memory = true;
    //     op.is_sib = true;
    //     op.offset = 0;
    //     op.offset_size = 0;
    //     op.base_reg = RSP;
    //     op.mod = 0;

    //     switch (scale)
    //     {
    //     case 1:
    //         scale = 0;
    //         break;
    //     case 2:
    //         scale = 1;
    //         break;
    //     case 4:
    //         scale = 2;
    //         break;
    //     case 8:
    //         scale = 3;
    //         break;
    //     }

    //     if (base == RBP)
    //     {
    //         op.mod = 1;
    //         op.offset_size = 1;
    //     }

    //     op.sib = (scale << 6) + ((index & 7) << 3) + (base & 7);

    //     return 0;
    // }

    // token_type sib_with_offset[] =
    // {
    //     // [ base + index * scale +- offset ]
    //     OPEN_SQUARE_BRACKET,
    //     REGISTER,
    //     PLUS_MINUS_SIGN,
    //     REGISTER,
    //     TIMES_SIGN,
    //     NUMERIC,
    //     PLUS_MINUS_SIGN,
    //     NUMERIC,
    //     CLOSED_SQUARE_BRACKET
    // };

    // if (match_by_order(tok, sib_with_offset, 9))
    // {
    //     token* base_tok = tok->next;
    //     token* index_tok = base_tok->next->next;
    //     token* scale_tok = index_tok->next->next;
    //     token* sign_tok = scale_tok->next;
    //     token* offset_tok = sign_tok->next;

    //     if (base_tok->next->index != 1)
    //         return ERROR_INVALID_SIB;

    //     if (base_tok->size != QWORD || index_tok->size != QWORD)
    //         return ERROR_REG_64;

    //     int scale = get_number(scale_tok->string);

    //     if (scale != 1 && scale != 2 && scale != 4 && scale != 8)
    //         return ERROR_INVALID_SCALE;

    //     reg base = (reg)base_tok->index;
    //     reg index = (reg)index_tok->index;

    //     if (base == RIP || index == RIP)
    //         return ERROR_INVALID_SIB;

    //     if (index == RSP)
    //         return ERROR_INVALID_SIB;

    //     int32_t offset = (int32_t)get_number(offset_tok->string) * sign_tok->index;

    //     int offset_size = sizeof_signed_immediate(offset);

    //     if (offset_size == BYTE)
    //         offset_size = 1;
    //     else
    //         offset_size = 4;

    //     op.is_memory = true;
    //     op.is_sib = true;
    //     op.offset = offset;
    //     op.offset_size = offset_size;
    //     op.base_reg = RSP;
    //     op.mod = 1 + (offset_size == 4);

    //     switch (scale)
    //     {
    //     case 1:
    //         scale = 0;
    //         break;
    //     case 2:
    //         scale = 1;
    //         break;
    //     case 4:
    //         scale = 2;
    //         break;
    //     case 8:
    //         scale = 3;
    //         break;
    //     }

    //     op.sib = (scale << 6) + ((index & 7) << 3) + (base & 7);

    //     return 0;
    // }

    // token_type memory_address[] =
    // {
    //     // [ address ]
    //     OPEN_SQUARE_BRACKET,
    //     NUMERIC,
    //     CLOSED_SQUARE_BRACKET
    // };

    // if (match_by_order(tok, memory_address, 3))
    // {
    //     uint64_t offset = get_number(tok->next->string);

    //     int offset_size = sizeof_immediate(offset);

    //     if (offset_size > DWORD)
    //         return ERROR_TOO_BIG;

    //     op.is_memory = true;
    //     op.is_sib = true;
    //     op.offset = offset;
    //     op.offset_size = 4;
    //     op.base_reg = RSP;
    //     op.mod = 0;
    //     op.sib = 0b00100101;

    //     return 0;
    // }

    // token_type sib_no_base_no_offset[] =
    // {
    //     // [ index * scale ]
    //     OPEN_SQUARE_BRACKET,
    //     REGISTER,
    //     TIMES_SIGN,
    //     NUMERIC,
    //     CLOSED_SQUARE_BRACKET
    // };

    // if (match_by_order(tok, sib_no_base_no_offset, 5))
    // {
    //     token* index_tok = tok->next;
    //     token* scale_tok = index_tok->next->next;

    //     if (index_tok->size != QWORD)
    //         return ERROR_REG_64;

    //     int scale = get_number(scale_tok->string);

    //     if (scale != 1 && scale != 2 && scale != 4 && scale != 8)
    //         return ERROR_INVALID_SCALE;

    //     reg index = (reg)index_tok->index;

    //     if (index == RSP)
    //         return ERROR_INVALID_SIB;

    //     op.is_memory = true;
    //     op.is_sib = true;
    //     op.offset = 0;
    //     op.offset_size = 4;
    //     op.base_reg = RSP;
    //     op.mod = 0;

    //     switch (scale)
    //     {
    //     case 1:
    //         scale = 0;
    //         break;
    //     case 2:
    //         scale = 1;
    //         break;
    //     case 4:
    //         scale = 2;
    //         break;
    //     case 8:
    //         scale = 3;
    //         break;
    //     }

    //     op.sib = (scale << 6) + ((index & 7) << 3) + (RBP & 7);

    //     return 0;
    // }

    // token_type sib_no_base_with_offset[] =
    // {
    //     // [ index * scale +- offset ]
    //     OPEN_SQUARE_BRACKET,
    //     REGISTER,
    //     TIMES_SIGN,
    //     NUMERIC,
    //     PLUS_MINUS_SIGN,
    //     NUMERIC,
    //     CLOSED_SQUARE_BRACKET
    // };

    // if (match_by_order(tok, sib_no_base_with_offset, 7))
    // {
    //     token* index_tok = tok->next;
    //     token* scale_tok = index_tok->next->next;
    //     token* sign_tok = scale_tok->next;
    //     token* offset_tok = sign_tok->next;

    //     if (index_tok->size != QWORD)
    //         return ERROR_REG_64;

    //     int scale = get_number(scale_tok->string);

    //     if (scale != 1 && scale != 2 && scale != 4 && scale != 8)
    //         return ERROR_INVALID_SCALE;

    //     reg index = (reg)index_tok->index;

    //     if (index == RSP)
    //         return ERROR_INVALID_SIB;

    //     int32_t offset = (int32_t)get_number(offset_tok->string) * sign_tok->index;

    //     op.is_memory = true;
    //     op.is_sib = true;
    //     op.offset = offset;
    //     op.offset_size = 4;
    //     op.base_reg = RSP;
    //     op.mod = 0;

    //     switch (scale)
    //     {
    //     case 1:
    //         scale = 0;
    //         break;
    //     case 2:
    //         scale = 1;
    //         break;
    //     case 4:
    //         scale = 2;
    //         break;
    //     case 8:
    //         scale = 3;
    //         break;
    //     }

    //     op.sib = (scale << 6) + ((index & 7) << 3) + (RBP & 7);

    //     return 0;
    // }

    // return 1;
}

int get_prefix_size(const string& str)
{
    if (str == "byte")
        return 1;

    if (str == "word")
        return 2;

    if (str == "dword")
        return 4;

    if (str == "qword")
        return 8;

    if (str == "xmmword")
        return 16;

    return 0;
}

bool Assembler::match_prefix_memory(int size, vector<Operand>& operands)
{
    if (cursor->type != SIZE)
        return false;

    if (get_prefix_size(cursor->str) != size)
        return false;

    cursor++;

    return match_memory(size, operands);
}

bool Assembler::match_immediate(int size, vector<Operand>& operands)
{
    return false;
}