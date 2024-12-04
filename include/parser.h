#pragma once

#include "tokenizer.h"
#include "expr.h"
#include "instruction.h"

bool parse_label(TokenStream& ts, std::string& label);

// bool parse_directive(TokenStream& ts, Directive& dir);

bool parse_instruction(TokenStream& ts, Instruction& inst);
bool parse_operand(TokenStream& ts, Operand& op);

bool parse_register(TokenStream& ts, Operand& op);
bool parse_memory(TokenStream& ts, Operand& op);
bool parse_immediate(TokenStream& ts, Operand& op);

bool parse_constant(TokenStream& ts, Constant& c);