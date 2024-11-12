#include "output.h"

using namespace std;

Symbol* Output::get_symbol(const string& name)
{
    for (auto& sym : symbols)
        if (sym->name == name)
            return sym;

    return nullptr;
}

Symbol* Output::add_symbol(const string& name)
{
    Symbol* sym = new Symbol;
    sym->name = name;
    symbols.push_back(sym);

    return sym;
}

void Output::define_symbol(const string& name)
{
    Symbol* sym = get_symbol(name);

    if (sym)
    {
        if (sym->is_defined)
            throw runtime_error("the symbol '" + name + "' is already defined");

        if (sym->is_imported)
            throw runtime_error("can't define imported symbol");
    }
    else
        sym = add_symbol(name);

    sym->is_defined = true;
    sym->section = current_section;
    sym->offset = current_section->bytes.size();
}

void Output::export_symbol(const string& name)
{
    Symbol* sym = get_symbol(name);

    if (sym)
    {
        if (sym->is_imported)
            throw runtime_error("can't export imported symbol");
    }
    else
        sym = add_symbol(name);

    sym->is_exported = true;
}

void Output::import_symbol(const string& name)
{
    Symbol* sym = get_symbol(name);

    if (sym)
    {
        if (sym->is_defined)
            throw runtime_error("can't import an already defined symbol");

        if (sym->is_exported)
            throw runtime_error("can't import exported symbol");
    }
    else
        sym = add_symbol(name);

    sym->is_imported = true;
}

Section* Output::get_section(const string& name)
{
    for (auto& sec : sections)
        if (sec->name == name)
            return sec;

    return nullptr;
}

Section* Output::add_section(const string& name, const SectionAttributes& attr)
{
    Section* sec = new Section();
    sec->name = name;
    sec->attr = attr;
    sections.push_back(sec);

    return sec;
}

void Output::set_current_section(const string& name)
{
    current_section = get_section(name);

    if (!current_section)
        current_section = add_section(name);
}

void Output::add(uint8_t byte)
{
    current_section->bytes.push_back(byte);
}

void Output::add(const std::vector<uint8_t>& bytes)
{
    current_section->bytes.insert(current_section->bytes.end(), bytes.begin(), bytes.end());
}

void Output::add_imm(uint64_t value, int size)
{
    for (int i = 0; i < size; i++)
    {
        add(value & 0xff);
        value >>= 8;
    }
}