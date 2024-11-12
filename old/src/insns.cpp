#include "insns.h"

std::unordered_map<std::string, std::vector<Template>> templates =
{
    {
        "adc",
        {
            {{RM8, IMM8}, {0x10, 0x20, 4_LIT, 20, 20 ,30, MOD_RM}},
            {{RM16, IMM16}, {}},
        }
    },
};

bool match_template(const Instruction& inst, const Template& temp)
{
    if (inst.operands.size() != temp.desc.size())
        return false;

    for (size_t i = 0; i < inst.operands.size(); i++)
        if (!match_descriptor(inst.operands[i], temp.desc[i]))
            return false;

    return true;
}

bool match_descriptor(const Operand& op, const OperandDescriptor& desc)
{
    switch (desc)
    {
    case REG8: return op.type == REGISTER && op.rclass == GENERAL_PURPOSE && op.size == 1;
    case REG16: return op.type == REGISTER && op.rclass == GENERAL_PURPOSE && op.size == 2;
    case REG32: return op.type == REGISTER && op.rclass == GENERAL_PURPOSE && op.size == 4;
    case REG64: return op.type == REGISTER && op.rclass == GENERAL_PURPOSE && op.size == 8;

    case MEM8: return op.type == MEMORY && op.size == 1;
    case MEM16: return op.type == MEMORY && op.size == 2;
    case MEM32: return op.type == MEMORY && op.size == 4;
    case MEM64: return op.type == MEMORY && op.size == 8;

    case RM8: return op.type == MEMORY && op.size == 1;
    case RM16: return op.type == MEMORY && op.size == 2;
    case RM32: return op.type == MEMORY && op.size == 4;
    case RM64: return op.type == MEMORY && op.size == 8;

        IMM8,
            IMM16,
            IMM32,
            IMM64,
    default: return false;
    }
}

void Template::encode(const Instruction& inst)
{

}