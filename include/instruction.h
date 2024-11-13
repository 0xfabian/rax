#pragma once

#include <cstdint>
#include <vector>
#include <string>

struct Symbol;

typedef uint64_t OperandType;

struct Operand
{
    OperandType type;

    int reg;

    int32_t disp;
    bool address_override;
    bool is_relative;
    bool is_sib;
    int base;
    int scale;
    int index;

    uint64_t imm;
    Symbol* sym;
};

struct Instruction
{
    uint8_t prefixes[5];
    std::string menmonic;
    std::vector<Operand> operands;
};