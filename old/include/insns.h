#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct Symbol;

enum OperandType
{
    REGISTER,
    MEMORY,
    IMMEDIATE,
    SYMBOL,
};

enum RegisterClass
{
    GENERAL_PURPOSE,
    CONTROL,
    DEBUG,
};

struct Operand
{
    OperandType type;

    // every type uses this field
    int size;

    // register type, reg is also used with memory and no sib
    RegisterClass rclass;
    int reg;

    // memory type
    int32_t disp;
    bool address_override;
    bool is_relative;
    bool is_sib;
    int base;
    int scale;
    int index;

    // immediate type
    uint64_t imm;

    // symbol type
    Symbol* sym;
    uint64_t addend;
};

struct Instruction
{
    std::string mnemonic;
    std::vector<Operand> operands;
};

enum OperandDescriptor
{
    REG8,
    REG16,
    REG32,
    REG64,

    MEM8,
    MEM16,
    MEM32,
    MEM64,

    RM8,
    RM16,
    RM32,
    RM64,

    IMM8,
    IMM16,
    IMM32,
    IMM64,
};

struct Template
{
    std::vector<OperandDescriptor> desc;

    void encode(const Instruction& inst);
};

extern std::unordered_map<std::string, std::vector<Template>> templates;

bool match_template(const Instruction& inst, const Template& temp);
bool match_descriptor(const Operand& op, const OperandDescriptor& desc);