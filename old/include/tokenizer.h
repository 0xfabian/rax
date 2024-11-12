#pragma once

#include <string>
#include <sstream>
#include <vector>

enum TokenType
{
    REGULAR,
    NUMERIC,
    COLON,
    OPEN_BRACKET,
    CLOSE_BRACKET,
    COMMA,
    PLUS,
    MINUS,
    TIMES
};

struct Token
{
    TokenType type;
    std::string str;
};

std::vector<Token> get_tokens(std::string line);