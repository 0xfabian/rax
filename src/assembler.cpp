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
                sections.back().add(cursor->str);
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

        for (size_t i = 0; i < sec.bytes.size(); i++)
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
    if (end < types.size() + cursor)
        return false;

    for (size_t i = 0; i < types.size(); i++)
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

    size_t i = 1;

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
    // doesn't handle labels, so no `d` encoded instructions
    // doesn't handle __

    Pattern p = parse_pattern(pattern);
    vector<Operand> operands;

    if (cursor->str != p.mnemonic)
        return false;

    cursor++;

    for (auto& op : p.operands)
    {
        if (cursor == end)
            return false;

        if (operands.size() > 0)
        {
            if (cursor->type != COMMA)
                throw runtime_error("expected comma after " + (cursor - 1)->str);

            cursor++;
        }

        if (!match_operand(op, operands))
            return false;
    }

    if (cursor != end)
        throw runtime_error("junk after " + (cursor - 1)->str);

    generate_bytes(p, operands);

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

    if (op == "rel8" || op == "rel32")
        return match_relative(op_size, operands);

    if (op == "m")
        return match_memory(operands);

    if (op == "m128")
        return match_prefix_memory(16, operands);

    if (op == "xmm")
        return match_simd_register(operands);

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

bool Assembler::match_simd_register(vector<Operand>& operands)
{
    if (cursor->type != SIMD_REGISTER)
        return false;

    Operand op;
    op.is_register = true;
    op.size = 16;
    op.base_reg = get_simd_register_index(cursor->str);

    operands.push_back(op);

    cursor++;

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

bool Assembler::match_memory_address(vector<Operand>& operands)
{
    // [ address ]
    if (!match_seq({ OPEN_BRACKET, NUMERIC, CLOSE_BRACKET }))
        return false;

    uint64_t offset = get_number(*(cursor + 1));
    int offset_size = get_immediate_size(offset);

    if (offset_size > 4)
        throw runtime_error("only 32-bit addresses are allowed");

    Operand op;
    op.is_memory = true;
    op.is_sib = true;
    op.offset = offset;
    op.offset_size = 4;
    op.base_reg = RSP;
    op.mod = 0;
    op.sib = 0b00100101;

    operands.push_back(op);

    cursor += 3;

    return true;
}

bool Assembler::match_memory_no_offset(vector<Operand>& operands)
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

bool Assembler::match_memory_with_offset(vector<Operand>& operands)
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

bool Assembler::match_memory_sib_no_offset(vector<Operand>& operands)
{
    // [ base + index * scale ]
    if (!match_seq({ OPEN_BRACKET, REGISTER, PLUS_MINUS, REGISTER, TIMES, NUMERIC, CLOSE_BRACKET }))
        return false;

    Token base = *(cursor + 1);
    Token index = *(cursor + 3);
    Token scale = *(cursor + 5);

    if ((cursor + 2)->str != "+")
        throw runtime_error("invalid sign in SIB expression");

    if (get_register_size(base.str) != 8 || get_register_size(index.str) != 8)
        throw runtime_error("register must be 64-bit in SIB expression");

    int sc = get_number(scale);

    if (sc != 1 && sc != 2 && sc != 4 && sc != 8)
        throw runtime_error("invalid scale in SIB expression");

    Register base_reg = (Register)get_register_index(base.str);
    Register index_reg = (Register)get_register_index(index.str);

    if (base_reg == RIP || index_reg == RIP || index_reg == RSP)
        throw runtime_error("invalid SIB expression");

    Operand op;
    op.is_memory = true;
    op.is_sib = true;
    op.offset = 0;
    op.offset_size = 0;
    op.base_reg = RSP;
    op.mod = 0;

    switch (sc)
    {
    case 1:     sc = 0;     break;
    case 2:     sc = 1;     break;
    case 4:     sc = 2;     break;
    case 8:     sc = 3;     break;
    }

    if (base_reg == RBP)
    {
        op.mod = 1;
        op.offset_size = 1;
    }

    op.sib = (sc << 6) + ((index_reg & 7) << 3) + (base_reg & 7);

    operands.push_back(op);

    cursor += 7;

    return true;
}

bool Assembler::match_memory_sib_with_offset(vector<Operand>& operands)
{
    // [ base + index * scale +- offset ]
    if (!match_seq({ OPEN_BRACKET, REGISTER, PLUS_MINUS, REGISTER, TIMES, NUMERIC, PLUS_MINUS, NUMERIC, CLOSE_BRACKET }))
        return false;

    Token base = *(cursor + 1);
    Token index = *(cursor + 3);
    Token scale = *(cursor + 5);
    Token sign = *(cursor + 6);
    Token offset = *(cursor + 7);

    if ((cursor + 2)->str != "+")
        throw runtime_error("invalid sign in SIB expression");

    if (get_register_size(base.str) != 8 || get_register_size(index.str) != 8)
        throw runtime_error("register must be 64-bit in SIB expression");

    int sc = get_number(scale);

    if (sc != 1 && sc != 2 && sc != 4 && sc != 8)
        throw runtime_error("invalid scale in SIB expression");

    Register base_reg = (Register)get_register_index(base.str);
    Register index_reg = (Register)get_register_index(index.str);

    if (base_reg == RIP || index_reg == RIP || index_reg == RSP)
        throw runtime_error("invalid SIB expression");

    Operand op;
    op.is_memory = true;
    op.is_sib = true;
    op.offset = (int32_t)get_number(offset) * ((sign.str == "+") ? 1 : -1);
    op.offset_size = get_signed_immediate_size(op.offset) > 1 ? 4 : 1;
    op.base_reg = RSP;
    op.mod = 1 + (op.offset_size == 4);

    switch (sc)
    {
    case 1:     sc = 0;     break;
    case 2:     sc = 1;     break;
    case 4:     sc = 2;     break;
    case 8:     sc = 3;     break;
    }

    op.sib = (sc << 6) + ((index_reg & 7) << 3) + (base_reg & 7);

    operands.push_back(op);

    cursor += 9;

    return true;
}

bool Assembler::match_memory_sib_no_base_no_offset(vector<Operand>& operands)
{
    // [ index * scale ]
    if (!match_seq({ OPEN_BRACKET, REGISTER, TIMES, NUMERIC, CLOSE_BRACKET }))
        return false;

    Token index = *(cursor + 1);
    Token scale = *(cursor + 3);

    if (get_register_size(index.str) != 8)
        throw runtime_error("register must be 64-bit in SIB expression");

    int sc = get_number(scale);

    if (sc != 1 && sc != 2 && sc != 4 && sc != 8)
        throw runtime_error("invalid scale in SIB expression");

    Register index_reg = (Register)get_register_index(index.str);

    if (index_reg == RSP)
        throw runtime_error("invalid SIB expression");

    Operand op;
    op.is_memory = true;
    op.is_sib = true;
    op.offset = 0;
    op.offset_size = 4;
    op.base_reg = RSP;
    op.mod = 0;

    switch (sc)
    {
    case 1:     sc = 0;     break;
    case 2:     sc = 1;     break;
    case 4:     sc = 2;     break;
    case 8:     sc = 3;     break;
    }

    op.sib = (sc << 6) + ((sc & 7) << 3) + (RBP & 7);

    operands.push_back(op);

    cursor += 5;

    return true;
}

bool Assembler::match_memory_sib_no_base_with_offset(vector<Operand>& operands)
{
    // [ index * scale +- offset ]
    if (!match_seq({ OPEN_BRACKET, REGISTER, TIMES, NUMERIC, PLUS_MINUS, NUMERIC, CLOSE_BRACKET }))
        return false;

    Token index = *(cursor + 1);
    Token scale = *(cursor + 3);
    Token sign = *(cursor + 4);
    Token offset = *(cursor + 5);

    if (get_register_size(index.str) != 8)
        throw runtime_error("register must be 64-bit in SIB expression");

    int sc = get_number(scale);

    if (sc != 1 && sc != 2 && sc != 4 && sc != 8)
        throw runtime_error("invalid scale in SIB expression");

    Register index_reg = (Register)get_register_index(index.str);

    if (index_reg == RSP)
        throw runtime_error("invalid SIB expression");

    Operand op;
    op.is_memory = true;
    op.is_sib = true;
    op.offset = (int32_t)get_number(offset) * ((sign.str == "+") ? 1 : -1);
    op.offset_size = 4;
    op.base_reg = RSP;
    op.mod = 0;

    switch (sc)
    {
    case 1:     sc = 0;     break;
    case 2:     sc = 1;     break;
    case 4:     sc = 2;     break;
    case 8:     sc = 3;     break;
    }

    op.sib = (sc << 6) + ((index_reg & 7) << 3) + (RBP & 7);

    operands.push_back(op);

    cursor += 7;

    return true;
}

bool Assembler::match_memory(vector<Operand>& operands)
{
    return
        match_memory_address(operands) ||
        match_memory_no_offset(operands) ||
        match_memory_with_offset(operands) ||
        match_memory_sib_no_offset(operands) ||
        match_memory_sib_with_offset(operands) ||
        match_memory_sib_no_base_no_offset(operands) ||
        match_memory_sib_no_base_with_offset(operands);
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

    return match_memory(operands);
}

bool Assembler::match_immediate(int size, vector<Operand>& operands)
{
    int sign = 1;
    int next = 0;

    if (cursor->type != NUMERIC)
    {
        if (!match_seq({ PLUS_MINUS, NUMERIC }))
            return false;

        if (cursor->str == "-")
            sign = -1;

        next = 1;
    }

    uint64_t value = (int64_t)get_number(*(cursor + next)) * sign;

    int signed_size = get_signed_immediate_size((int64_t)value);
    int unsigned_size = get_immediate_size(value);

    int compare_size = (signed_size > unsigned_size) ? unsigned_size : signed_size;

    if (compare_size > size)
        return false;

    Operand op;
    op.is_immediate = true;
    op.size = size;
    op.imm = value;

    operands.push_back(op);

    cursor += 1 + next;

    return true;
}

bool Assembler::match_relative(int size, vector<Operand>& operands)
{
    if (cursor->type != NUMERIC)
        return false;

    int64_t value = (uint64_t)get_number(*cursor);

    // value -= bytes + instruction_size;

    int signed_size = 4;

    if (size > signed_size)
        return false;

    Operand op;
    op.is_immediate = true;
    op.size = size;
    op.imm = value;

    operands.push_back(op);

    cursor++;

    return true;
}

void Assembler::generate_bytes(const Pattern& pattern, const vector<Operand>& operands)
{
    if (sections.empty())
        throw runtime_error("instruction must be inside a section");

    auto op = operands.begin();

    for (auto& byte : pattern.bytes)
    {
        if (pattern.encoding != "zo" && op == operands.end())
            throw runtime_error("not enough operands");

        if (byte[0] == '/')
        {
            uint8_t low;
            uint8_t mid;
            uint8_t mod;

            Operand* mem = nullptr;

            if (byte[1] == 'r')
            {
                Operand dest = *op;
                Operand src = *(op + 1);

                if (src.is_register && dest.is_register)
                    mod = 3;
                else
                {
                    if (src.is_memory)
                    {
                        mem = &src;
                        mod = src.mod;
                    }
                    else
                    {
                        mem = &dest;
                        mod = dest.mod;
                    }
                }

                low = dest.base_reg & 7;
                mid = src.base_reg & 7;

                op += 2;
            }
            else    // byte[1] = 0 ... 7 
            {
                Operand dest = *op;

                if (dest.is_register)
                    mod = 3;
                else
                {
                    mem = &dest;
                    mod = dest.mod;
                }

                low = dest.base_reg & 7;
                mid = byte[1] - '0';

                op++;
            }

            uint8_t modrm;

            if (pattern.encoding[0] == 'r' && pattern.encoding[1] == 'm')
                modrm = (mod << 6) + (low << 3) + mid;
            else
                modrm = (mod << 6) + (mid << 3) + low;

            sections.back().add(modrm);

            if (mem)
            {
                if (mem->sib)
                    sections.back().add(mem->sib);

                sections.back().add(mem->offset, mem->offset_size);
            }
        }
        else if (byte[0] == 'i')
        {
            sections.back().add(op->imm, op->size);

            op++;
        }
        else if (byte[2] == '+')
        {
            uint8_t val = stoi(byte.substr(0, 2), nullptr, 16);

            if (byte[3] == 'r')
                val += op->base_reg & 7;
            else if (byte[3] == 'c')
                val += op->imm;

            op++;

            sections.back().add(val);
        }
        else
            sections.back().add(byte);
    }

    if (op < operands.end())
    {
        printf("dif: %ld\n", operands.end() - op);
        throw runtime_error("there are more operands (" + to_string(operands.size()) + ") then expected");
    }
}