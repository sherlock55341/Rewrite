#pragma once

#include <global.hpp>

RW_NAMESPACE_START

constexpr int CLASS = 222;

class LUT{
public:
    int pPhase[1 << 16];
    int pPerms[1 << 16];
    int pPerms4[2 * 3 * 4][4];
    int pMap[1 << 16];
    int nNodes[CLASS];
    int nSubgr[CLASS];
    int pSubgr[CLASS][20];
    int fanin0[CLASS][60];
    int fanin1[CLASS][60];
    int isC0[CLASS][60];
    int isC1[CLASS][60];

    void readLUT();
};

RW_NAMESPACE_END