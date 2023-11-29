#include "dar.hpp"

RW_NAMESPACE_START

DarEngine::DarEngine(LUT& lut_)
    : lut(lut_),
      nObjs(0),
      nPIs(0),
      nPOs(0),
      nLevel(0),
      pFanin0(nullptr),
      pFanin1(nullptr),
      isC0(nullptr),
      isC1(nullptr),
      pOuts(nullptr),
      levels(nullptr),
      levelCount(nullptr),
      isDar(false) {}

void DarEngine::resetDar() {
    nObjs = 0;
    nPIs = 0;
    nPOs = 0;
    nLevel = 0;
    isDar = false;
    if (pFanin0) {
        free(pFanin0);
        pFanin0 = nullptr;
    }
    if (pFanin1) {
        free(pFanin1);
        pFanin1 = nullptr;
    }
    if (pOuts){
        free(pOuts);
        pOuts = nullptr;
    }
    if (isC0) {
        free(isC0);
        isC0 = nullptr;
    }
    if (isC1) {
        free(isC1);
        isC1 = nullptr;
    }
    if(levels){
        free(levels);
        levels = nullptr;
    }
    if(levelCount){
        free(levelCount);
        levelCount = nullptr;
    }
}

DarEngine::~DarEngine() {
    resetDar();
}

void DarEngine::printDarLog() const{
    printf("Rewrite Engine INFO : nObjs = %d, nPIs = %d, nPOs = %d\n", nObjs, nPIs, nPOs);
    printf("                      nLevel = %d\n", nLevel);
    for (int level = 0; level <= nLevel; level++){
        printf("                    level %d has %d nodes\n", level, levelCount[level] - (level ? levelCount[level - 1] : 0));
    }
}

void DarEngine::setAig(int nObjs_, int nPIs_, int nPOs_, int *pFanin0_, int *pFanin1_, int *pOuts_){
    resetDar();
    nObjs = nObjs_;
    nPIs = nPIs_;
    nPOs = nPOs_;
    pFanin0 = (int *) malloc(sizeof(int) * nObjs);
    pFanin1 = (int *) malloc(sizeof(int) * nObjs);
    pOuts = (int *) malloc(sizeof(int) * nPOs);
    memcpy(pFanin0, pFanin0_, sizeof(int) * nObjs);
    memcpy(pFanin1, pFanin1_, sizeof(int) * nObjs);
    memcpy(pOuts, pOuts_, sizeof(int) * nPOs);
    isC0 = (int *) malloc(sizeof(int) * nObjs);
    isC1 = (int *) malloc(sizeof(int) * nObjs);
    convertFanArray(false);
    levels = (int *) malloc(sizeof(int) * nObjs);
    levelCount = (int *) malloc(sizeof(int) * nObjs);
    calcLevels();
}

void DarEngine::rewrite(bool useZero, bool useGPU) {
    if (useGPU) {
        // rewriteGPUVersion(useZero);
    } else {
        rewriteCPUVersion(useZero);
    }
}

void DarEngine::rewriteCPUVersion(bool useZero) {

}

void DarEngine::convertFanArray(bool direction) {
    if(!direction){
        if(isDar){
            printf("Error : AIG format is already changed to DAR format\n");
            return ;
        }
    }
    else{
        if(!isDar){
            printf("Error : AIG format is already done\n");
            return ;
        }
    }
    for (int i = 0; i < nObjs; i++) {
        if (!direction) {
            isC0[i] = pFanin0[i] & 1;
            pFanin0[i] >>= 1;
            isC1[i] = pFanin1[i] & 1;
            pFanin1[i] >>= 1;
        } else {
            pFanin0[i] = pFanin0[i] << 1 | isC0[i];
            pFanin1[i] = pFanin1[i] << 1 | isC1[i];
        }
    }
}

void DarEngine::calcLevels(){
    memset(levels, 0, sizeof(int) * nObjs);
    memset(levelCount, 0, sizeof(int) * nObjs);
    for (int i = nPIs + 1; i < nObjs; i++){
        levels[i] = 1 + std::max(levels[pFanin0[i]], levels[pFanin1[i]]);
    }
    nLevel = *std::max_element(levels, levels + nObjs);
    for (int i = 0; i < nObjs; i++){
        levelCount[levels[i]]++;
    }
    for (int level = 1; level <= nLevel; level++){
        levelCount[level] += levelCount[level - 1];
    }
}

void DarEngine::copyAig(int &nObjs_, int &nPIs_, int &nPOs_, int *pFanin0_, int *pFanin1_, int *pOuts_, bool useGPU){
    if(!useGPU){
        copyAigCPU(nObjs_, nPIs_, nPOs_, pFanin0_, pFanin1_, pOuts_);
    }
    else{
        //TODO
    }
}

void DarEngine::copyAigCPU(int &nObjs_, int &nPIs_, int &nPOs_, int *pFanin0_, int *pFanin1_, int *pOuts_){
    nObjs_ = nObjs;
    nPIs_ = nPIs;
    nPOs_ = nPOs;
    convertFanArray(true);
    memcpy(pFanin0_, pFanin0, sizeof(int) * nObjs);
    memcpy(pFanin1_, pFanin1, sizeof(int) * nObjs);
    memcpy(pOuts_, pOuts, sizeof(int) * nPOs);
}

RW_NAMESPACE_END