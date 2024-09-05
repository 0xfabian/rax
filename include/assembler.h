#pragma once

#include <iostream>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <tokenizer.h>
#include <patterns.h>
#include <elf.h>

enum Register
{
    AL, CL, DL, BL, AH, CH, DH, BH,
    AX, CX, DX, BX, SP, BP, SI, DI,
    EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
    RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
    RIP
};

enum SIMDRegister
{
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
    XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15
};

struct Relocation
{

};

struct SectionAttributes
{
    bool progbits;
    bool alloc;
    bool exec;
    bool write;
    uint64_t align;
};

struct Section
{
    std::string name;
    SectionAttributes attr;
    std::vector<uint8_t> bytes;
    std::vector<Relocation> rels;

    void add(std::string byte) { bytes.push_back(stoi(byte, nullptr, 16)); }
    void add(uint8_t byte) { bytes.push_back(byte); }

    void add(uint64_t value, int size)
    {
        for (int i = 0; i < size; i++)
        {
            add((uint8_t)value);
            value >>= 8;
        }
    }
};

struct Symbol
{
    std::string name;
    Section* section;
    size_t offset;
    bool is_defined = false;
    bool is_exported = false;
    bool is_imported = false;
};

struct Pattern
{
    std::string mnemonic;
    std::vector<std::string> operands;

    std::string encoding;
    std::vector<std::string> bytes;
};

struct Operand
{
    bool is_immediate = false;  // use imm and size   
    bool is_register = false;   // use base_reg and size   
    bool is_memory = false;     // use base_reg, size and offset
    bool is_sib = false;        // use base_reg, index_reg, scale, size and offset

    int size = 0;
    uint64_t imm = 0;
    int base_reg = 0;
    int mod = 0;
    uint8_t sib = 0;
    uint32_t offset = 0;
    int offset_size = 0;
};

struct StringTable
{
    std::vector<std::string> strings;

    int index_of(const std::string& str)
    {
        int ret = 0;

        for (auto& _str : strings)
        {
            if (_str == str)
                return ret;

            ret += _str.size() + 1;
        }

        strings.push_back(str);

        return ret;
    }

    std::vector<uint8_t> bytes()
    {
        std::vector<uint8_t> ret;

        for (auto& str : strings)
        {
            for (auto ch : str)
                ret.push_back(ch);

            ret.push_back(0);
        }

        return ret;
    }
};

typedef std::vector<Token>::const_iterator Cursor;

struct Assembler
{
    std::string filename;
    std::vector<Section*> sections;
    Section* current_section;

    std::vector<Symbol*> symbols;
    bool first_pass = true;

    Cursor cursor;
    Cursor end;

    Assembler(const std::string& _filename);

    int assemble(const std::vector<std::string>& lines);
    void parse_tokens(const std::vector<Token>& tokens);
    void remove_empty_sections();
    void dump();
    void output();

    void add_section(const std::string& name, const SectionAttributes& attr = { true, true, false, false, 1 });
    void add_symbol(const std::string& name);
    void export_symbol(const std::string& name);
    void import_symbol(const std::string& name);
    void add_label(const std::string& name);

    bool match_seq(const std::vector<TokenType>& types);
    bool match_condition(const std::string& suffix, std::vector<Operand>& operands);
    bool match_mnemonic(const std::string& mnemonic, std::vector<Operand>& operands);
    bool match_pattern(const std::string& pattern);
    bool match_operand(const std::string& op, std::vector<Operand>& operands);

    bool match_register(int size, std::vector<Operand>& operands);
    bool match_simd_register(std::vector<Operand>& operands);

    bool match_memory_address(std::vector<Operand>& operands);
    bool match_memory_no_offset(std::vector<Operand>& operands);
    bool match_memory_with_offset(std::vector<Operand>& operands);
    bool match_memory_sib_no_offset(std::vector<Operand>& operands);
    bool match_memory_sib_with_offset(std::vector<Operand>& operands);
    bool match_memory_sib_no_base_no_offset(std::vector<Operand>& operands);
    bool match_memory_sib_no_base_with_offset(std::vector<Operand>& operands);
    bool match_memory(std::vector<Operand>& operands);
    bool match_prefix_memory(int size, std::vector<Operand>& operands);

    bool match_immediate(int size, std::vector<Operand>& operands);
    bool match_relative(int size, std::vector<Operand>& operands);

    void generate_bytes(const Pattern& pattern, const std::vector<Operand>& operands);
};