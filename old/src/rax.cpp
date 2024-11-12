#include "rax.h"

using namespace std;

Assembler::Assembler()
{
    add_section(".data", { true, true, false, true, 4 });
    add_section(".bss", { false, true, false, true, 4 });
    add_section(".comment", { true, false, false, false, 1 });
    add_section(".rodata", { true, true, false, false, 4 });
    add_section(".text", { true, true, true, false, 16 });

    set_current_section(".text");
}

void Assembler::assemble(string line)
{
    vector<Token> tokens = get_tokens(line);

    cursor = tokens.begin();
    end = tokens.end();

    // labeled_line = [label] line
    // line = empty | directive | instruction

    // instruction = [prefix] menmonic [operands]
    // operands = operand | operand, operands
    // operand = register | memory | immediate | symbol

    if (!parse_labeled_line())
        cout << line << "\n";
}

void Assembler::output()
{

}

bool Assembler::parse_labeled_line()
{
    parse_label();

    return parse_line();
}

bool Assembler::parse_label()
{
    if (cursor == end || (cursor + 1) == end)
        return false;

    if (cursor->type == REGULAR && (cursor + 1)->type == COLON)
    {
        define_symbol(cursor->str);
        cursor += 2;

        return true;
    }

    return false;
}

bool Assembler::parse_line()
{
    return parse_empty() || parse_directive() || parse_instruction();
}

bool Assembler::parse_empty()
{
    return cursor == end;
}

bool Assembler::parse_directive()
{
    return false;
}

bool Assembler::parse_instruction()
{
    Instruction inst;

    parse_prefix(inst);

    if (!parse_mnemonic(inst))
        return false;

    parse_operands(inst);

    return encode(inst);
}

bool Assembler::parse_prefix(Instruction& inst)
{
    return false;
}

bool Assembler::parse_mnemonic(Instruction& inst)
{
    return false;
}

bool Assembler::parse_operands(Instruction& inst)
{
    return false;
}

bool Assembler::parse_operand(Instruction& inst)
{
    return false;
}

bool Assembler::parse_register(Instruction& inst)
{
    return false;
}

bool Assembler::parse_memory(Instruction& inst)
{
    return false;
}

bool Assembler::parse_immediate(Instruction& inst)
{
    return false;
}

bool Assembler::encode(Instruction& inst)
{
    if (templates.find(inst.mnemonic) == templates.end())
    {
        printf("unkown instruction '%s'\n", inst.mnemonic.c_str());
        return false;
    }

    vector<Template> all = templates[inst.mnemonic];

    for (auto& temp : all)
    {
        if (match_template(inst, temp))
        {
            temp.encode(inst);
            return true;
        }
    }

    printf("unknown operands for '%s' instruction\n", inst.mnemonic.c_str());
    return false;
}

Symbol* Assembler::get_symbol(const string& name)
{
    for (auto& sym : symbols)
        if (sym->name == name)
            return sym;

    return nullptr;
}

Symbol* Assembler::add_symbol(const string& name)
{
    Symbol* sym = new Symbol;
    sym->name = name;
    symbols.push_back(sym);

    return sym;
}

void Assembler::define_symbol(const string& name)
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

void Assembler::export_symbol(const string& name)
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

void Assembler::import_symbol(const string& name)
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

Section* Assembler::get_section(const string& name)
{
    for (auto& sec : sections)
        if (sec->name == name)
            return sec;

    return nullptr;
}

Section* Assembler::add_section(const string& name, const SectionAttributes& attr)
{
    Section* sec = new Section();
    sec->name = name;
    sec->attr = attr;
    sections.push_back(sec);

    return sec;
}

void Assembler::set_current_section(const string& name)
{
    current_section = get_section(name);

    if (!current_section)
        current_section = add_section(name);
}