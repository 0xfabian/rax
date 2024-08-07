#pragma once

#include <string>
#include <sstream>
#include <vector>

enum TokenType
{
    OTHER,

    REGISTER,
    SIMD_REGISTER,
    SIZE,
    NUMERIC,
    LABEL,
    LABEL_DEFINITION,

    OPEN_BRACKET,
    CLOSE_BRACKET,
    COMMA,

    PLUS_MINUS,
    TIMES
};

struct Token
{
    TokenType type;
    std::string str;
};

std::vector<Token> get_tokens(std::string line);

int get_register_size(const std::string& str);
int get_register_index(const std::string& str);
int get_simd_register_index(const std::string& str);
bool is_register(const std::string& str);
bool is_simd_register(const std::string& str);
