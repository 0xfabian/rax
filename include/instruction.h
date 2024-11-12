#pragma once

#include <cstdint>
#include <vector>
#include <string>

struct Symbol;

struct OperandFlags
{
    uint64_t data;
};

struct Operand
{
    OperandFlags flags;

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
    uint64_t addend;
};

struct Instruction
{
    uint8_t prefixes[5];
    std::string menmonic;
    std::vector<Operand> operands;
};