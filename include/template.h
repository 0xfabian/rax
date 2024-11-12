#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "instruction.h"

struct Template
{
    std::vector<OperandType> types;
    std::vector<uint8_t> code;
};

extern std::unordered_map<std::string, std::vector<Template>> rax_instructions;