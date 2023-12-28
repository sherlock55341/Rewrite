#pragma once

#include <global.hpp>

RW_NAMESPACE_START

size_t getFileSize(const std::string &path);

unsigned decode(char **pos);

void encode(char *buf, int &cur, unsigned x);

inline unsigned invert(unsigned lit){
    return lit < 2 ? 1 - lit : lit;
}

std::string getDesignName(const std::string &path);

#define errorCheck(expression)                                                 \
    if ((expression) == false) {                                               \
        std::cout << "Expression failed at file " << __FILE__ << " line "      \
                  << __LINE__ << std::endl;                                    \
    }

int popCount(unsigned x);

int truthSwap(int truth, int bias);

int truthStretch(int truth, int n, int phase);

int truthShrink(int truth, int n, int phase);

bool isPrime(int x);

int getNearestPrime(int x);

RW_NAMESPACE_END