#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

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
};

struct Output
{
    std::vector<Symbol*> symbols;
    std::vector<Section*> sections;
    Section* current_section;

    Symbol* get_symbol(const std::string& name);
    Symbol* add_symbol(const std::string& name);
    void define_symbol(const std::string& name);
    void export_symbol(const std::string& name);
    void import_symbol(const std::string& name);

    Section* get_section(const std::string& name);
    Section* add_section(const std::string& name, const SectionAttributes& attr = { true, true, false, false, 1 });
    void set_current_section(const std::string& name);

    void add(uint8_t byte);
    void add(const std::vector<uint8_t>& bytes);
    void add_imm(uint64_t value, int size);
};