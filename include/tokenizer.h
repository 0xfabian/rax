#pragma once

#include <string>
#include <sstream>
#include <vector>

enum TokenType
{
    EOS,
    REGULAR,
    NUMERIC,
    COLON,
    OPEN_BRACKET,
    CLOSE_BRACKET,
    COMMA,
    PLUS,
    MINUS,
    TIMES,
};

struct Token
{
    TokenType type;
    std::string str;
};

struct TokenStream
{
    std::vector<Token> tokens;
    size_t pos = 0;

    TokenStream(const std::string& line);

    const Token& operator[](size_t i) const;

    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    void advance(size_t n = 1);

    explicit operator bool() const { return pos < tokens.size(); }
};
