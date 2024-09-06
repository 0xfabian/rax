#include <assembler.h>

using namespace std;

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

Assembler::Assembler(const string& _filename)
{
    filename = _filename;

    add_section(".data", { true, true, false, true, 4 });
    add_section(".bss", { false, true, false, true, 4 });
    add_section(".comment", { true, false, false, false, 1 });
    add_section(".rodata", { true, true, false, false, 4 });
    add_section(".text", { true, true, true, false, 16 });
}

int Assembler::assemble(const vector<string>& lines)
{
    size_t errors = 0;

    for (size_t i = 0; i < lines.size(); i++)
    {
        vector<Token> tokens = get_tokens(lines[i]);

        try
        {
            parse_tokens(tokens);
        }
        catch (const std::exception& e)
        {
            errors++;
            cerr << "\e[91merror:\e[0m " << i + 1 << ": " << e.what() << '\n';
        }
    }

    for (auto& sym : symbols)
    {
        if (!sym->is_defined && !sym->is_imported)
        {
            errors++;
            cerr << "\e[91merror:\e[0m " << "symbol '" + sym->name + "' is undefined\n";
        }
    }

    if (!errors)
    {
        remove_empty_sections();

        for (auto& sec : sections)
        {
            for (auto& rel : sec->rels)
            {
                if (!rel.sym->is_imported) // so is a defined symbol
                {
                    // need to swap the symbol definition to the section plus its offset

                    rel.addend += rel.sym->offset;
                    rel.sec = rel.sym->section;
                    rel.sym = nullptr;
                }
            }
        }

        // for (auto& sec : sections)
        // {
        //     if (sec->rels.empty())
        //         continue;

        //     cout << ".rela" << sec->name << "\n";

        //     for (auto& rel : sec->rels)
        //         cout << "offset = " << rel.offset << "    sym = " << ((rel.sym) ? rel.sym->name : rel.sec->name) << "   type = " << rel.type << "   addend = " << rel.addend << "\n";

        //     cout << "\n";
        // }

        // dump();
        output();
    }

    return errors;
}

void Assembler::parse_tokens(const vector<Token>& tokens)
{
    if (tokens.empty())
        return;

    auto begin = tokens.begin();
    end = tokens.end();

    cursor = begin;

    if (cursor->type == LABEL_DEFINITION)
    {
        add_label(cursor->str.substr(0, cursor->str.length() - 1));

        cursor++;
        begin = cursor;

        if (cursor == end)
            return;
    }

    if (cursor->str == "section")
    {
        cursor++;

        if (cursor == end)
            throw runtime_error("expected section name after section");

        if (cursor->type != LABEL)
            throw runtime_error("invalid section name '" + cursor->str + "'");

        if (cursor + 1 != end)
            throw runtime_error("junk after section " + cursor->str);

        add_section(cursor->str);
    }
    else if (cursor->str == "export" || cursor->str == "global")
    {
        cursor++;

        if (cursor == end)
            throw runtime_error("expected symbol name after export");

        if (cursor->type != LABEL)
            throw runtime_error("invalid symbol name '" + cursor->str + "'");

        if (cursor + 1 != end)
            throw runtime_error("junk after symbol " + cursor->str);

        export_symbol(cursor->str);
    }
    else if (cursor->str == "import" || cursor->str == "extern")
    {
        cursor++;

        if (cursor == end)
            throw runtime_error("expected symbol name after import");

        if (cursor->type != LABEL)
            throw runtime_error("invalid symbol name '" + cursor->str + "'");

        if (cursor + 1 != end)
            throw runtime_error("junk after symbol " + cursor->str);

        import_symbol(cursor->str);
    }
    else if (cursor->str == "db")
    {
        cursor++;

        if (cursor == end)
            throw runtime_error("expected constant after db");

        while (cursor != end)
        {
            if (cursor->type == COMMA)
            {
                cursor++;
                continue;
            }

            try
            {
                current_section->add(cursor->str);
                cursor++;
            }
            catch (...)
            {
                throw runtime_error("unable to parse constant '" + cursor->str + "'");
            }
        }
    }
    else if (cursor->str == "resb")
    {
        cursor++;

        if (cursor == end)
            throw runtime_error("expected constant after resb");

        if (cursor->type != NUMERIC)
            throw runtime_error("unable to parse constant '" + cursor->str + "'");

        uint64_t count = get_number(*cursor);

        for (uint64_t i = 0; i < count; i++)
            current_section->add(0);
    }
    else
    {
        for (auto& pattern : patterns)
        {
            cursor = begin;

            if (match_pattern(pattern))
                return;
        }

        throw runtime_error("unable to match instruction");
    }
}

void Assembler::remove_empty_sections()
{
    sections.erase(remove_if(sections.begin(), sections.end(), [](Section* sec) { return sec->bytes.size() == 0; }), sections.end());
}

void hexdump(vector<uint8_t> bytes)
{
    size_t i = 0;

    while (true)
    {
        if (i % 16 == 0)
        {
            if (i >= bytes.size())
                break;

            printf("%08lx:  ", i);
        }

        if (i % 16 == 8)
            printf(" ");

        if (i < bytes.size())
            printf("%02x ", bytes[i]);
        else
            printf("   ");

        if (i % 16 == 15)
        {
            printf(" |");

            for (int j = 0; j < 16; j++)
            {
                size_t index = i - (i % 16) + j;

                if (index >= bytes.size())
                    break;

                if (isprint(bytes[index]))
                    printf("%c", bytes[index]);
                else
                    printf(".");
            }

            printf("|\n");
        }

        i++;
    }
}

void Assembler::dump()
{
    for (auto& sec : sections)
    {
        cout << sec->name << "\n";
        hexdump(sec->bytes);
        cout << "\n";
    }
}

string get_output_name(const string& filename)
{
    size_t dot = filename.rfind(".");

    if (dot != string::npos)
        return filename.substr(0, dot) + ".o";

    return filename + ".o";
}

void zero_fill(ofstream& out, size_t offset)
{
    streampos pos = out.tellp();

    if (offset > (size_t)pos)
    {
        size_t padding = offset - pos;
        vector<char> zeros(padding, 0);
        out.write(zeros.data(), zeros.size());
    }
}

void Assembler::output()
{
    size_t shstrtab_index = 1 + sections.size();
    size_t symtab_index = shstrtab_index + 1;
    size_t strtab_index = symtab_index + 1;

    vector<Elf64_Shdr> shdrs;
    vector<Elf64_Sym> syms;
    vector<vector<Elf64_Rela>> all_rels;

    StringTable shstrtab;
    StringTable strtab;

    Elf64_Shdr null_shdr;
    memset(&null_shdr, 0, sizeof(Elf64_Shdr));

    null_shdr.sh_name = shstrtab.index_of("");

    shdrs.push_back(null_shdr);

    for (auto& sec : sections)
    {
        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.index_of(sec->name);

        shdr.sh_type = sec->attr.progbits ? SHT_PROGBITS : SHT_NOBITS;
        shdr.sh_flags = 0;

        if (sec->attr.alloc)
            shdr.sh_flags |= SHF_ALLOC;

        if (sec->attr.exec)
            shdr.sh_flags |= SHF_EXECINSTR;

        if (sec->attr.write)
            shdr.sh_flags |= SHF_WRITE;

        shdr.sh_addr = 0;
        shdr.sh_offset = 0;
        shdr.sh_size = sec->bytes.size();
        shdr.sh_link = 0;
        shdr.sh_info = 0;
        shdr.sh_addralign = sec->attr.align;
        shdr.sh_entsize = 0;

        shdrs.push_back(shdr);
    }

    Elf64_Shdr shstrtab_shdr;
    shstrtab_shdr.sh_name = shstrtab.index_of(".shstrtab");
    shstrtab_shdr.sh_type = SHT_STRTAB;
    shstrtab_shdr.sh_flags = 0;
    shstrtab_shdr.sh_addr = 0;
    shstrtab_shdr.sh_offset = 0;
    shstrtab_shdr.sh_size = 0;          // fill in later
    shstrtab_shdr.sh_link = 0;
    shstrtab_shdr.sh_info = 0;
    shstrtab_shdr.sh_addralign = 1;
    shstrtab_shdr.sh_entsize = 0;

    shdrs.push_back(shstrtab_shdr);

    Elf64_Shdr symtab_shdr;
    symtab_shdr.sh_name = shstrtab.index_of(".symtab");
    symtab_shdr.sh_type = SHT_SYMTAB;
    symtab_shdr.sh_flags = 0;
    symtab_shdr.sh_addr = 0;
    symtab_shdr.sh_offset = 0;
    symtab_shdr.sh_size = 0;                  // fill in later
    symtab_shdr.sh_link = strtab_index;
    symtab_shdr.sh_info = 0;                  // fill in later
    symtab_shdr.sh_addralign = 8;
    symtab_shdr.sh_entsize = sizeof(Elf64_Sym);

    shdrs.push_back(symtab_shdr);

    Elf64_Shdr strtab_shdr;
    strtab_shdr.sh_name = shstrtab.index_of(".strtab");
    strtab_shdr.sh_type = SHT_STRTAB;
    strtab_shdr.sh_flags = 0;
    strtab_shdr.sh_addr = 0;
    strtab_shdr.sh_offset = 0;
    strtab_shdr.sh_size = 0;    // fill in later
    strtab_shdr.sh_link = 0;
    strtab_shdr.sh_info = 0;
    strtab_shdr.sh_addralign = 1;
    strtab_shdr.sh_entsize = 0;

    shdrs.push_back(strtab_shdr);

    // cout << ".shstrtab\n";
    // hexdump(shstrtab.bytes());
    // cout << "\n";

    Elf64_Sym null_sym;
    null_sym.st_name = strtab.index_of("");
    null_sym.st_info = (STB_LOCAL << 4) | STT_NOTYPE;
    null_sym.st_other = STV_DEFAULT;
    null_sym.st_shndx = SHN_UNDEF;
    null_sym.st_value = 0;
    null_sym.st_size = 0;

    syms.push_back(null_sym);

    Elf64_Sym file_sym;
    file_sym.st_name = strtab.index_of(filename);
    file_sym.st_info = (STB_LOCAL << 4) | STT_FILE;
    file_sym.st_other = STV_DEFAULT;
    file_sym.st_shndx = SHN_ABS;
    file_sym.st_value = 0;
    file_sym.st_size = 0;

    syms.push_back(file_sym);

    for (size_t i = 0; i < sections.size(); i++)
    {
        Elf64_Sym sym;
        sym.st_name = strtab.index_of("");
        sym.st_info = (STB_LOCAL << 4) | STT_SECTION;
        sym.st_other = STV_DEFAULT;
        sym.st_shndx = 1 + i;               // 1 from the first null section
        sym.st_value = 0;
        sym.st_size = 0;

        syms.push_back(sym);
    }

    for (auto& _sym : symbols)
    {
        Elf64_Sym sym;
        sym.st_name = strtab.index_of(_sym->name);
        sym.st_info = STT_NOTYPE;

        if (_sym->is_exported || _sym->is_imported)
            sym.st_info |= STB_GLOBAL << 4;
        else
            sym.st_info |= STB_LOCAL << 4;

        sym.st_other = STV_DEFAULT;

        if (_sym->is_imported)
            sym.st_shndx = STN_UNDEF;
        else
        {
            size_t i;

            for (i = 0; i < sections.size(); i++)
                if (_sym->section == sections[i])
                    break;

            sym.st_shndx = 1 + i; // 1 from first null section
        }

        sym.st_value = _sym->is_defined ? _sym->offset : 0;
        sym.st_size = 0;

        syms.push_back(sym);
    }

    // cout << ".strtab\n";
    // hexdump(strtab.bytes());
    // cout << "\n";

    sort(syms.begin(), syms.end(), [](const Elf64_Sym& a, const Elf64_Sym& b) { return (a.st_info >> 4) < (b.st_info >> 4); });

    size_t global_index;

    for (global_index = 0; global_index < syms.size(); global_index++)
        if ((syms[global_index].st_info >> 4) == STB_GLOBAL)
            break;

    shdrs[symtab_index].sh_info = global_index;
    shdrs[symtab_index].sh_size = syms.size() * sizeof(Elf64_Sym);

    for (size_t i = 0; i < sections.size(); i++)
    {
        if (sections[i]->rels.empty())
            continue;

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.index_of(".rela" + sections[i]->name);

        shdr.sh_type = SHT_RELA;
        shdr.sh_flags = 0;

        shdr.sh_addr = 0;
        shdr.sh_offset = 0;
        shdr.sh_size = sections[i]->rels.size() * sizeof(Elf64_Rela);
        shdr.sh_link = symtab_index;
        shdr.sh_info = 1 + i;   // 1 from first null section
        shdr.sh_addralign = 8;
        shdr.sh_entsize = sizeof(Elf64_Rela);

        shdrs.push_back(shdr);

        vector<Elf64_Rela> rels;

        for (auto& rel : sections[i]->rels)
        {
            Elf64_Rela rela;

            rela.r_offset = rel.offset;
            rela.r_addend = rel.addend;

            if (rel.type == 1)
                rela.r_info = R_X86_64_PC32;
            else
                rela.r_info = R_X86_64_32S;

            size_t index;

            if (rel.sym == nullptr)
            {
                size_t si;

                for (si = 0; si < sections.size(); si++)
                    if (sections[si] == rel.sec)
                        break;

                si += 1; // from first entry;

                for (index = 0; index < syms.size(); index++)
                    if ((syms[index].st_info & 0x0f) == STT_SECTION && syms[index].st_shndx == si)
                        break;
            }
            else
            {
                for (index = 0; index < syms.size(); index++)
                    if (syms[index].st_name == (unsigned int)strtab.index_of(rel.sym->name))
                        break;
            }

            rela.r_info |= index << 32;

            rels.push_back(rela);
        }

        all_rels.push_back(rels);
    }

    shdrs[shstrtab_index].sh_size = shstrtab.bytes().size();
    shdrs[strtab_index].sh_size = strtab.bytes().size();

    Elf64_Ehdr ehdr;
    memset(&ehdr, 0, sizeof(Elf64_Ehdr));

    ehdr.e_ident[EI_MAG0] = 0x7F;
    ehdr.e_ident[EI_MAG1] = 'E';
    ehdr.e_ident[EI_MAG2] = 'L';
    ehdr.e_ident[EI_MAG3] = 'F';
    ehdr.e_ident[EI_CLASS] = 2;           // 64-bit
    ehdr.e_ident[EI_DATA] = 1;            // little endian
    ehdr.e_ident[EI_VERSION] = 1;         // always 1
    ehdr.e_ident[EI_OSABI] = 0;           // system-v
    ehdr.e_ident[EI_ABIVERSION] = 0;
    ehdr.e_type = ET_REL;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = 1;                   // always 1
    ehdr.e_entry = 0;                     // no entry
    ehdr.e_phoff = 0;                     // no program headers
    ehdr.e_shoff = sizeof(Elf64_Ehdr);
    ehdr.e_flags = 0;
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_phentsize = 0;
    ehdr.e_phnum = 0;
    ehdr.e_shentsize = sizeof(Elf64_Shdr);
    ehdr.e_shnum = shdrs.size();
    ehdr.e_shstrndx = shstrtab_index;

    size_t offset = ehdr.e_ehsize + ehdr.e_shnum * ehdr.e_shentsize;

    for (auto& shdr : shdrs)
    {
        if (shdr.sh_type == SHT_NOBITS)
            continue;

        size_t align = shdr.sh_addralign;

        if (align == 0)
            align = 1;

        offset = (offset + align - 1) & ~(align - 1);

        shdr.sh_offset = offset;

        offset += shdr.sh_size;
    }

    ofstream out(get_output_name(filename), ios::binary);

    out.write((const char*)&ehdr, sizeof(ehdr));

    for (auto& shdr : shdrs)
        out.write((const char*)&shdr, sizeof(shdr));

    for (size_t i = 1; i < shdrs.size(); i++)
    {
        if (shdrs[i].sh_type == SHT_NOBITS)
            continue;

        zero_fill(out, shdrs[i].sh_offset);

        if ((i - 1) < sections.size())  // real sections
            out.write((const char*)sections[i - 1]->bytes.data(), shdrs[i].sh_size);
        else
        {
            int index = (i - 1) - sections.size();

            if (index == 0)
                out.write((const char*)shstrtab.bytes().data(), shdrs[i].sh_size);
            else if (index == 1)
            {
                for (auto& _sym : syms)
                    out.write((const char*)&_sym, sizeof(_sym));
            }
            else if (index == 2)
                out.write((const char*)strtab.bytes().data(), shdrs[i].sh_size);
            else
                out.write((const char*)all_rels[index - 3].data(), shdrs[i].sh_size);
        }
    }

    out.close();
}

void Assembler::add_section(const string& name, const SectionAttributes& attr)
{
    for (size_t i = 0; i < sections.size(); i++)
        if (sections[i]->name == name)
        {
            current_section = sections[i];
            return;
        }

    Section* sec = new Section();
    sec->name = name;
    sec->attr = attr;

    current_section = sec;
    sections.push_back(sec);
}

Symbol* Assembler::add_symbol(const string& name)
{
    for (auto& sym : symbols)
        if (sym->name == name)
            return sym;

    Symbol* sym = new Symbol();
    sym->name = name;

    symbols.push_back(sym);

    return sym;
}

void Assembler::export_symbol(const std::string& name)
{
    for (auto& sym : symbols)
        if (sym->name == name)
        {
            if (sym->is_imported)
                throw runtime_error("can't export imported symbol");

            sym->is_exported = true;
        }

    Symbol* sym = new Symbol();
    sym->name = name;
    sym->is_exported = true;

    symbols.push_back(sym);
}

void Assembler::import_symbol(const std::string& name)
{
    for (auto& sym : symbols)
        if (sym->name == name)
        {
            if (sym->is_defined)
                throw runtime_error("can't import an already defined symbol");

            if (sym->is_exported)
                throw runtime_error("can't import exported symbol");

            sym->is_exported = true;
        }

    Symbol* sym = new Symbol();
    sym->name = name;
    sym->is_imported = true;

    symbols.push_back(sym);
}

void Assembler::add_label(const string& name)
{
    for (auto& sym : symbols)
        if (sym->name == name)
        {
            if (sym->is_defined)
                throw runtime_error("the symbol '" + name + "' is already defined");

            if (sym->is_imported)
                throw runtime_error("can't define imported symbol");

            sym->is_defined = true;
            sym->section = current_section;
            sym->offset = current_section->bytes.size();

            return;
        }

    Symbol* sym = new Symbol();
    sym->name = name;
    sym->is_defined = true;
    sym->section = current_section;
    sym->offset = current_section->bytes.size();

    symbols.push_back(sym);
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

bool Assembler::match_condition(const string& suffix, vector<Operand>& operands)
{
    if (conditions.find(suffix) == conditions.end())
        return false;

    Operand op;
    op.imm = conditions[suffix];
    operands.push_back(op);

    cursor++;
    return true;
}

bool Assembler::match_mnemonic(const string& mnemonic, vector<Operand>& operands)
{
    if (mnemonic.length() > 2 && mnemonic.substr(mnemonic.length() - 2) == "__")
    {
        string prefix = mnemonic.substr(0, mnemonic.length() - 2);

        if (cursor->str.length() < prefix.length())
            return false;

        if (cursor->str.find(prefix) != 0)
            return false;

        string suffix = cursor->str.substr(prefix.length());

        return match_condition(suffix, operands);
    }

    if (cursor->str == mnemonic)
    {
        cursor++;
        return true;
    }

    return false;
}

bool Assembler::match_pattern(const string& pattern)
{
    Pattern p = parse_pattern(pattern);
    vector<Operand> operands;

    if (!match_mnemonic(p.mnemonic, operands))
        return false;

    bool first_operand = true;

    for (auto& op : p.operands)
    {
        if (cursor == end)
            return false;

        if (!first_operand)
        {
            if (cursor->type != COMMA)
                throw runtime_error("expected comma after " + (cursor - 1)->str);

            cursor++;
        }
        else
            first_operand = false;

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

bool Assembler::match_memory_address(vector<Operand>& operands)
{
    uint64_t offset = 0;
    int offset_size = 4;
    Symbol* sym = nullptr;

    // [ address ]
    if (!match_seq({ OPEN_BRACKET, LABEL, CLOSE_BRACKET }))
    {
        if (!match_seq({ OPEN_BRACKET, NUMERIC, CLOSE_BRACKET }))
            return false;

        offset = get_number(*(cursor + 1));
        offset_size = get_immediate_size(offset);

        if (offset_size > 4)
            throw runtime_error("only 32-bit addresses are allowed");
    }
    else
        sym = add_symbol((cursor + 1)->str);

    Operand op;
    op.is_memory = true;
    op.is_sib = true;
    op.offset = offset;
    op.offset_size = 4;
    op.base_reg = RSP;
    op.mod = 0;
    op.sib = 0b00100101;

    if (sym)
    {
        op.is_symbol = true;
        op.sym = sym;
    }

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
    if (size != 4)
        return false;

    if (cursor->type != LABEL)
        return false;

    Symbol* sym = add_symbol(cursor->str);

    Operand op;
    op.is_immediate = true;
    op.size = 4;
    op.imm = 0;

    op.is_symbol = true;
    op.sym = sym;

    operands.push_back(op);

    cursor++;

    return true;
}

void Assembler::generate_bytes(const Pattern& pattern, const vector<Operand>& operands)
{
    // size_t sz = current_section->bytes.size();

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

            current_section->add(modrm);

            if (mem)
            {
                if (mem->sib)
                    current_section->add(mem->sib);

                if (mem->is_symbol)
                {
                    Relocation rel;
                    rel.offset = current_section->bytes.size();
                    rel.sym = mem->sym;
                    rel.type = 2;
                    rel.addend = 0;

                    current_section->rels.push_back(rel);
                }

                current_section->add(mem->offset, mem->offset_size);
            }
        }
        else if (byte[0] == 'i')
        {
            if (op->is_symbol)
            {
                Relocation rel;
                rel.offset = current_section->bytes.size();
                rel.sym = op->sym;
                rel.type = 1;
                rel.addend = -4;

                current_section->rels.push_back(rel);
            }

            current_section->add(op->imm, op->size);

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

            current_section->add(val);
        }
        else
            current_section->add(byte);
    }

    if (op < operands.end())
    {
        printf("dif: %ld\n", operands.end() - op);
        throw runtime_error("there are more operands (" + to_string(operands.size()) + ") then expected");
    }
}