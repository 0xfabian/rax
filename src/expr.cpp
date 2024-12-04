#include "expr.h"

bool Constant::is_symbolic() const
{
    return !symbol.empty();
}

Constant Constant::operator+(const Constant& other) const
{
    if (other.is_symbolic() && is_symbolic())
        throw std::runtime_error("cannot add two symbols");

    return { other.is_symbolic() ? other.symbol : symbol, offset + other.offset };
}

Constant Constant::operator-(const Constant& other) const
{
    if (other.is_symbolic())
        throw std::runtime_error("cannot subtract symbol");

    return { symbol, offset - other.offset };
}

Constant Constant::operator-() const
{
    if (is_symbolic())
        throw std::runtime_error("cannot negate symbol");

    return { "", -offset };
}