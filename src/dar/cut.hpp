#pragma once

#include <global.hpp>

RW_NAMESPACE_START

class Cut{
public:
    int sign, leaves[4];
    unsigned truthTable : 16;
    unsigned val : 11;
    unsigned nLeaves : 4;
    bool used;
};

RW_NAMESPACE_END