#pragma once

#include <iostream>
#include <vector>
#include <string>

#include "tokenizer.h"
#include "insns.h"

struct Symbol;
struct Section;
struct Relocation;

struct Symbol
{
    std::string name;
    Section* section;
    size_t offset;

    bool is_defined = false;
    bool is_exported = false;
    bool is_imported = false;
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

    void add(uint8_t byte)
    {
        bytes.push_back(byte);
    }

    void add(uint64_t value, int size)
    {
        for (int i = 0; i < size; i++)
        {
            add((uint8_t)value);
            value >>= 8;
        }
    }
};

struct Relocation
{
    Symbol* sym;
    Section* sec;
    uint64_t offset;
    int64_t addend;
    int type;
};

typedef std::vector<Token>::const_iterator Cursor;

struct Assembler
{
    std::vector<Symbol*> symbols;
    std::vector<Section*> sections;
    Section* current_section;

    Cursor cursor;
    Cursor end;

    Assembler();

    void assemble(std::string line);
    void output();

    bool parse_labeled_line();
    bool parse_label();
    bool parse_line();
    bool parse_empty();
    bool parse_directive();
    bool parse_instruction();

    bool parse_prefix(Instruction& inst);
    bool parse_mnemonic(Instruction& inst);
    bool parse_operands(Instruction& inst);
    bool parse_operand(Instruction& inst);
    bool parse_register(Instruction& inst);
    bool parse_memory(Instruction& inst);
    bool parse_immediate(Instruction& inst);

    bool encode(Instruction& inst);

    Symbol* get_symbol(const std::string& name);
    Symbol* add_symbol(const std::string& name);
    void define_symbol(const std::string& name);
    void export_symbol(const std::string& name);
    void import_symbol(const std::string& name);

    Section* get_section(const std::string& name);
    Section* add_section(const std::string& name, const SectionAttributes& attr = { true, true, false, false, 1 });
    void set_current_section(const std::string& name);
};