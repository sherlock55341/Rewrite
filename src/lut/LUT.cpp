#include "LUT.hpp"
#include "LUT.inc"

RW_NAMESPACE_START

void LUT::readLUT(){
    int cur = 0;

    auto word = [&](){
        return LIBRARY[cur++];
    };

    int len = 1 << 16;
    for (int i = 0; i < len; i++){
        pPhase[i] = word();
    }
    for (int i = 0; i < len; i++){
        pPerms[i] = word();
    }
    for (int i = 0; i < 2 * 3 * 4; i++){
        for (int j = 0; j < 4; j++){
            pPerms4[i][j] = word();
        }
    }
    for (int i = 0; i < len; i++){
        pMap[i] = word();
    }
    for (int i = 0; i < CLASS; i++){
        nNodes[i] = word();
    }
    for (int i = 0; i < CLASS; i++){
        nSubgr[i] = word();
    }
    for (int c = 0; c < CLASS; c++){
        for (int i = 0; i < nNodes[c]; i++){
            fanin0[c][i] = word();
            fanin1[c][i] = word();
            isC0[c][i] = word();
            isC1[c][i] = word();
        }
        for (int i = 0; i < nSubgr[c]; i++){
            pSubgr[c][i] = word();
        }
    }
}

RW_NAMESPACE_END