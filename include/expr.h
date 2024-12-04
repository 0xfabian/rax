#pragma once

#include <string>
#include <stdexcept>

struct Constant
{
    std::string symbol;
    int64_t offset = 0;

    bool is_symbolic() const;

    Constant operator+(const Constant& other) const;
    Constant operator-(const Constant& other) const;
    Constant operator-() const;
};