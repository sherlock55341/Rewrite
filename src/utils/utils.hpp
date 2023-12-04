#pragma once

#include <global.hpp>

RW_NAMESPACE_START

size_t getFileSize(const std::string &path);

unsigned decode(char **pos);

void encode(char *buf, int &cur, unsigned x);

std::string getDesignName(const std::string &path);

#define errorCheck(expression)                                                 \
    if ((expression) == false) {                                               \
        std::cout << "Expression failed at file " << __FILE__ << " line "      \
                  << __LINE__ << std::endl;                                    \
    }

int popCount(unsigned x);

unsigned truthSwap(unsigned truth, int bias);

unsigned truthStretch(unsigned truth, int n, int phase);

unsigned truthShrink(unsigned truth, int n, int phase);

RW_NAMESPACE_END