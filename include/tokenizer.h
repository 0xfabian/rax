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

    static const Token eos;

    TokenStream(const std::string& line);

    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    void advance(size_t n = 1);

    const Token& operator[](size_t i) const;
    explicit operator bool() const;
};
