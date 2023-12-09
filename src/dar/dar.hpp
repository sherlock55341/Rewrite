#pragma once

#include <global.hpp>
#include <lut/LUT.hpp>

RW_NAMESPACE_START

class DarEngine {
  public:
    DarEngine(LUT &lut_);

    ~DarEngine();

    void rewrite(int nObjs_, int nPIs_, int nPOs_, int *pFanin0_, int *pFanin1_,
                 int *pOuts_, bool useZero);

    class Cut {
      public:
        int leaves[4];
        unsigned sign;
        unsigned truthTable : 16;
        unsigned val : 11;
        unsigned nLeaves : 4;
        bool used;
    };

    struct TableNode {
        int next;
        int value;
    };

  private:
    LUT &lut;

    int nObjs;
    int nPIs;
    int nPOs;
    int nLevel;
    int BIG_PRIME;

    int *pFanin0 = nullptr;
    int *pFanin1 = nullptr;
    int *pOuts = nullptr;
    int *levels = nullptr;
    int *nRef = nullptr;
    bool *del = nullptr;

    TableNode* hashTable = nullptr;

    std::vector<std::vector<int>> levelNodes;

    int id(int x) const;

    bool isC(int x) const;

    int cutIdx(int x, int i) const;

    void calcLevelsAndReOrder();

    void countRefs() const;

    void solveCutOneLevel(Cut *cuts, std::vector<int> &levelNode, int level);

    int getCutValue(const Cut &cut);

    int findCutPosition(Cut *cuts, int node);

    bool mergeCut(const Cut &a, const Cut &b, Cut &c);

    bool checkCutRedundant(Cut *cuts, int pos);
    // check whether a is subset of b
    bool checkCutSubset(const Cut &a, const Cut &b);

    int cutPhase(const Cut &a, const Cut &b);

    unsigned truthTableMerge(const Cut &a, const Cut &b, const Cut &c,
                             bool isC0, bool isC1);

    bool minimizeCut(Cut &cut);

    void buildHashTable();

    int lookUp(int in0, int in1, bool isC0, bool isC1);
};

RW_NAMESPACE_END