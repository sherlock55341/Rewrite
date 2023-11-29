#pragma once

#include <global.hpp>
#include <dar/dar.hpp>

RW_NAMESPACE_START

class AIGManager {
  public:
    int readAIGFromFile(const std::string &path);

    int saveAIG2File(const std::string &path);

    AIGManager();

    ~AIGManager();

    void printAIGLog() const;

    void printRWLog() const;

    void rewrite();

    std::string designName_;
    std::string designPath_;
    std::string comments_;
    int nObjs;
    int nPIs;
    int nPOs;
    int nLatches;
    int nNodes;
    int nLevels;
    int *pFanin0 = nullptr, *pFanin1 = nullptr;
    int *pNumFanouts = nullptr;
    int *pOuts = nullptr;

    int *d_pFanin0 = nullptr, *d_pFanin1 = nullptr;
    int *d_pNumFanouts = nullptr;
    int *d_pOuts = nullptr;

    bool aigCreated = false;

    bool usingDar = false;

    bool aigOnDevice = false;

    bool aigNewest = true;

    LUT lut;

    DarEngine dar;

  private:
  
    void resetManager();

    void mallocDevice();

    void freeDevice();

    void copy2Device() const;

    void copyFromDevice() const;

    int readAIGFromMemory(char *fc, size_t length);

    void copyFromDarCPU();
};

RW_NAMESPACE_END