#include "aig.hpp"

RW_NAMESPACE_START

void AIGManager::resetManager() {
    designName_ = "";
    designPath_ = "";
    comments_ = "";

    nObjs = 0;
    nPIs = 0;
    nPOs = 0;
    nLatches = 0;
    nNodes = 0;
    nLevels = 0;

    if (pFanin0) {
        free(pFanin0);
    }
    pFanin0 = nullptr;
    if (pFanin1) {
        free(pFanin1);
    }
    pFanin1 = nullptr;
    if (pOuts) {
        free(pOuts);
    }
    pOuts = nullptr;

    aigCreated = false;
}
RW_NAMESPACE_END