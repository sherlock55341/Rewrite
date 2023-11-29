#pragma once

#include <global.hpp>
#include <lut/LUT.hpp>

RW_NAMESPACE_START

class DarEngine{
public:
    DarEngine(LUT& lut_);

    ~DarEngine();

    void setAig(int nObjs_, int nPIs_, int nPOs_, int *pFanin0_, int *pFanin1_, int *pOuts_);

    void rewrite(bool useZero, bool useGPU);

    void copyAig(int &nObjs_, int &nPIs_, int &nPOs_, int *pFanin0_, int *pFanin1_, int *pOuts_, bool useGPU);

    void printDarLog() const;
private:    
    LUT &lut;

    void resetDar();
    
    void rewriteCPUVersion(bool useZero);

    void rewriteGPUVersion(bool useZero);

    // 0 for aig to dar, 1 for dar to aig
    void convertFanArray(bool direction); 

    void calcLevels();

    void copyAigCPU(int &nObjs_, int &nPIs_, int &nPOs_, int *pFanin0_, int *pFanin1_, int *pOuts_);

    int nObjs, nPIs, nPOs, nLevel;

    int *pFanin0 = nullptr, *pFanin1 = nullptr, *isC0 = nullptr, *isC1 = nullptr, *pOuts = nullptr;
    int *levels = nullptr, *levelCount = nullptr;

    int *d_pFanin0 = nullptr, *d_pFanin1 = nullptr;

    bool isDar = false;
};

RW_NAMESPACE_END