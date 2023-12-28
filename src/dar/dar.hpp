#pragma once

#include <global.hpp>
#include <lut/LUT.hpp>
#include <cassert>

RW_NAMESPACE_START

inline size_t hash(size_t x) { return x; }

inline size_t hash(size_t hash_1, size_t hash_2) {
    return hash_1 ^ (hash_2 * 0x9e3779b9 + (hash_1 << 6) + (hash_1 >> 2));
}

template <typename T>
inline void hash_combine(std::size_t &seed, const T &val) {
    seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
// auxiliary generic functions to create a hash value using a seed
template <typename T> inline void hash_val(std::size_t &seed, const T &val) {
    hash_combine(seed, val);
}
template <typename T, typename... Types>
inline void hash_val(std::size_t &seed, const T &val, const Types &... args) {
    hash_combine(seed, val);
    hash_val(seed, args...);
}

template <typename... Types>
inline std::size_t hash_val(const Types &... args) {
    std::size_t seed = 0;
    hash_val(seed, args...);
    return seed;
}

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &p) const {
        assert(p.first < p.second);
        return hash_val(p.first, p.second);
    }
};

class DarEngine {
  public:
    DarEngine(LUT &lut_);

    ~DarEngine();

    void rewrite(int &nObjs_, int nPIs_, int nPOs_, int *pFanin0_, int *pFanin1_,
                 int *pOuts_, bool useZero);

    class Cut {
      public:
        int leaves[4], sign;
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
    int timeStamp;
    int expected;

    int *pFanin0 = nullptr;
    int *pFanin1 = nullptr;
    int *pOuts = nullptr;
    int *levels = nullptr;
    int *nRef = nullptr;
    int *created = nullptr;
    int *phase = nullptr;
    int *visTime = nullptr;
    int *bestSubgr = nullptr;
    bool *del = nullptr;

    Cut *bestCuts = nullptr;
    

    TableNode* hashTable = nullptr;

    std::vector<std::vector<int>> levelNodes;

    std::unordered_map<std::pair<int, int>, int, pair_hash> table;

    int id(int x) const;

    bool isC(int x) const;

    int cutIdx(int x, int i) const;

    void calcLevelsAndReOrder();

    void countRefs() const;

    void enumerateCutOneLevel(Cut *cuts, std::vector<int> &levelNode, int level);

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

    int decrease(int idx, Cut& cut, int *tableSize, int *tableIdx, int *tableNum);

    int calcMFFC(int idx, Cut& cut, int *tableSize, int *tableIdx, int *tableNum, int root);

    void evaluateNode(Cut* cuts, bool useZero);

    bool isDeleted(int idx, int *tableSize, int *tableIdx, int *tableNum);

    int eval(int matchIdx, int *match, int Class, int idx);

    int eval2(int idx, int Class, const std::vector<int>& match);

    void buildReplaceHashTable();

    void evalAndReplace(int idx, Cut& cut);

    int markMFFC(int idx, Cut& cut);

    void build(int cur, int Class, std::vector<int>& match, const std::vector<int>& isc);

    void replace(int newn, int oldn, int isc);

    void delet(int node, int flag = 1);

    void disconnect(int node);

    int genAnd(int in0, int in1);

    void connect(int node, int in0, int in1);

    void doRef(int idx);

    int doDeref(int idx);

    int isRedundant(int idx) const;

    int getFanin(int x);

    void reorder();

    int dfs(int x);
};


RW_NAMESPACE_END