#pragma once

#include <global.hpp>
#include <lut/LUT.hpp>

RW_NAMESPACE_START

class AIGManager {
  public:
    int readAIGFromFile(const std::string &path);

    int saveAIG2File(const std::string &path);

    AIGManager();

    ~AIGManager();

    void printAIGLog() const;

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
    int *pOuts = nullptr;
    bool aigCreated = false;

    LUT lut;

  private:
  
    void resetManager();

    int readAIGFromMemory(char *fc, size_t length);
};

RW_NAMESPACE_END