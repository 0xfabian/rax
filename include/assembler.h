#pragma once

#include <tokenizer.h>
#include <patterns.h>

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

struct Section
{
    std::string name;
    std::vector<uint8_t> bytes;
};

struct Symbol
{
    std::string name;
    bool is_imported;
    bool is_defined;
    size_t address;
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

typedef std::vector<Token>::const_iterator Cursor;

struct Assembler
{
    std::vector<Section> sections;
    std::vector<Symbol> symbols;

    Cursor cursor;
    Cursor end;

    void parse_tokens(const std::vector<Token>& tokens);
    void dump();

    bool match_seq(const std::vector<TokenType>& types);
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
};