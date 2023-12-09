#include "dar.hpp"
#include <utils/utils.hpp>
#include <cpp/tools.hpp>

RW_NAMESPACE_START

constexpr size_t MAX_AIG_SIZE = 1e5;
constexpr size_t MAX_CUT_SIZE = 8;
constexpr int K_EMPTY = -1;

inline size_t hash(size_t x) { return x; }

inline size_t hash(size_t hash_1, size_t hash_2) {
    return hash_1 ^ (hash_2 * 0x9e3779b9 + (hash_1 << 6) + (hash_1 >> 2));
}

DarEngine::DarEngine(LUT &lut_) : lut(lut_) {
    pFanin0 = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    pFanin1 = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    pOuts = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    levels = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    nRef = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    del = (bool *)malloc(sizeof(bool) * MAX_AIG_SIZE);
    nObjs = 0;
    nPIs = 0;
    nPOs = 0;
    nLevel = 0;
}

DarEngine::~DarEngine() {
    free(pFanin0);
    free(pFanin1);
    free(pOuts);
    free(levels);
    free(del);
}

void DarEngine::rewrite(int nObjs_, int nPIs_, int nPOs_, int *pFanin0_,
                        int *pFanin1_, int *pOuts_, bool useZero) {
    nObjs = nObjs_;
    nPIs = nPIs_;
    nPOs = nPOs_;
    BIG_PRIME = getNearestPrime(nObjs * 2);
    memcpy(pFanin0, pFanin0_, sizeof(int) * nObjs);
    memcpy(pFanin1, pFanin1_, sizeof(int) * nObjs);
    memcpy(pOuts, pOuts_, sizeof(int) * nPOs);
    memset(del, 0, sizeof(bool) * nObjs);
    calcLevelsAndReOrder();
    printf("    Before rewrite #node = %d, #level = %d\n", nObjs - 1, nLevel);
    countRefs();

    Cut *cuts = (Cut *)malloc(sizeof(Cut) * MAX_CUT_SIZE * nObjs);

    for (int i = 0; i < nObjs * MAX_CUT_SIZE; i++) {
        cuts[i].used = false;
    }

    for (int level = 0; level <= nLevel; level++) {
        // printf("first of level %d %d\n", level, levelNodes[level][0]);
        solveCutOneLevel(cuts, levelNodes[level], level);
    }

    int totalCut = 0;

    for (int i = 0; i < nObjs * MAX_CUT_SIZE; i++) {
        if (cuts[i].used == true) {
            totalCut++;
        }
    }
    printf("    #Cuts enumerated : %d\n", totalCut);

    hashTable = (TableNode*) malloc(sizeof(TableNode) * (BIG_PRIME + nObjs * 2));

    printf("    Rewriting..., now memory usage = %.2lf MB, peak memory usage = %.2lf MB\n", utils::tools::MemReporter::getCurrent(), utils::tools::MemReporter::getPeak());

    for (int i = 0; i < BIG_PRIME + nObjs * 2; i++){
        hashTable[i].next = K_EMPTY;
    }

    buildHashTable();

    free(cuts);
    free(hashTable);
    cuts = nullptr;
    hashTable = nullptr;
}

void DarEngine::calcLevelsAndReOrder() {
    levels[0] = -1;
    for (int i = 1; i <= nPIs; i++) {
        levels[i] = 0;
    }
    for (int i = nPIs + 1; i < nObjs; i++) {
        levels[i] =
            1 + std::max(levels[id(pFanin0[i])], levels[id(pFanin1[i])]);
    }
    nLevel = *std::max_element(levels, levels + nObjs);
    std::vector<int> pos(nObjs, 0), counts(nObjs, 0), f0(nObjs, 0),
        f1(nObjs, 0);
    for (int i = 1; i < nObjs; i++) {
        counts[levels[i]]++;
    }
    for (int i = 1; i <= nLevel; i++) {
        counts[i] += counts[i - 1];
    }
    for (int i = nObjs - 1; i > 0; i--) {
        pos[i] = counts[levels[i]];
        counts[levels[i]]--;
    }
    for (int i = nPIs + 1; i < nObjs; i++) {
        f0[i] = pFanin0[i];
        f1[i] = pFanin1[i];
    }
    for (int i = nPIs + 1; i < nObjs; i++) {
        pFanin0[pos[i]] = (pos[id(f0[i])] << 1 | isC(f0[i]));
        pFanin1[pos[i]] = (pos[id(f1[i])] << 1 | isC(f1[i]));
    }
    for (int i = 0; i < nPOs; i++) {
        pOuts[i] = (pos[id(pOuts[i])] << 1 | isC(pOuts[i]));
    }
    levels[0] = -1;
    for (int i = 1; i <= nPIs; i++) {
        levels[i] = 0;
    }
    for (int i = nPIs + 1; i < nObjs; i++) {
        levels[i] =
            1 + std::max(levels[id(pFanin0[i])], levels[id(pFanin1[i])]);
    }
    nLevel = *std::max_element(levels, levels + nObjs);
    levelNodes.clear();
    levelNodes.resize(nLevel + 1, std::vector<int>());
    for (int i = 1; i < nObjs; i++) {
        levelNodes[levels[i]].push_back(i);
    }
}

void DarEngine::countRefs() const {
    for (int i = 1; i < nObjs; i++) {
        nRef[i] = 0;
    }
    for (int i = nPIs + 1; i < nObjs; i++) {
        nRef[id(pFanin0[i])]++;
        nRef[id(pFanin1[i])]++;
    }
    for (int i = 0; i < nPOs; i++) {
        nRef[id(pOuts[i])]++;
    }
    for (int i = 1; i < nObjs; i++) {
        if (nRef[i] == 0) {
            del[i] = true;
        }
    }
}

int DarEngine::cutIdx(int x, int i) const { return x * MAX_CUT_SIZE + i; }

int DarEngine::getCutValue(const Cut &cut) {
    if (cut.nLeaves < 2)
        return 1001;
    int value = 0, ones = 0;
    for (int i = 0; i < cut.nLeaves; i++) {
        value += nRef[cut.leaves[i]];
        ones += (nRef[cut.leaves[i]] == 1);
    }
    if (ones > 3) {
        value = 5 - ones;
    }
    return std::min(value, 1000);
}

int DarEngine::findCutPosition(Cut *cuts, int node) {
    for (int i = 0; i < MAX_CUT_SIZE; i++) {
        if (cuts[cutIdx(node, i)].used == false) {
            return i;
        }
    }
    int pos = -1;
    for (int i = 0; i < MAX_CUT_SIZE; i++) {
        if (cuts[cutIdx(node, i)].nLeaves > 2) {
            if (pos == -1 ||
                cuts[cutIdx(node, i)].val < cuts[cutIdx(node, pos)].val) {
                pos = i;
            }
        }
    }
    if (pos == -1) {
        for (int i = 0; i < MAX_CUT_SIZE; i++) {
            if (cuts[cutIdx(node, i)].nLeaves == 2) {
                if (pos == -1 ||
                    cuts[cutIdx(node, i)].val < cuts[cutIdx(node, pos)].val) {
                    pos = i;
                }
            }
        }
    }
    if (pos == -1) {
        for (int i = 0; i < MAX_CUT_SIZE; i++) {
            if (cuts[cutIdx(node, i)].nLeaves < 2) {
                if (pos == -1 ||
                    cuts[cutIdx(node, i)].val < cuts[cutIdx(node, pos)].val) {
                    pos = i;
                }
            }
        }
    }
    cuts[cutIdx(node, pos)].used = false;
    return pos;
}

bool DarEngine::mergeCut(const Cut &a, const Cut &b, Cut &c) {
    // // if(a.nLeaves > 1 || b.nLeaves > 1){
    // {
    //     printf("a cut : ");
    //     for (int ca = 0; ca < a.nLeaves; ca++){
    //         printf("%d ", a.leaves[ca]);
    //     }
    //     printf("\n");
    //     printf("b cut : ");
    //     for (int cb = 0; cb < b.nLeaves; cb++){
    //         printf("%d ", b.leaves[cb]);
    //     }
    //     printf("\n");
    // }

    int i = 0, j = 0, k = 0;
    while (i < a.nLeaves && j < b.nLeaves) {
        if (k == 4)
            return false;
        if (a.leaves[i] < b.leaves[j]) {
            c.leaves[k] = a.leaves[i];
            k++;
            i++;
        } else if (a.leaves[i] > b.leaves[j]) {
            c.leaves[k] = b.leaves[j];
            k++;
            j++;
        } else {
            c.leaves[k] = a.leaves[i];
            k++;
            i++;
            j++;
        }
    }
    while (i < a.nLeaves) {
        if (k == 4) {
            return false;
        }
        c.leaves[k] = a.leaves[i];
        k++;
        i++;
    }
    while (j < b.nLeaves) {
        if (k == 4) {
            return false;
        }
        c.leaves[k] = b.leaves[j];
        k++;
        j++;
    }
    c.nLeaves = k;
    c.sign = a.sign | b.sign;
    c.used = true;
    return true;
}

bool DarEngine::checkCutSubset(const Cut &a, const Cut &b) {
    for (int i = 0; i < a.nLeaves; i++) {
        bool inb = false;
        for (int j = 0; j < b.nLeaves; j++) {
            if (a.leaves[i] == b.leaves[j]) {
                inb = true;
                break;
            }
        }
        if (inb == false)
            return false;
    }
    return true;
}

bool DarEngine::checkCutRedundant(Cut *cuts, int pos) {
    for (int i = 0; i < MAX_CUT_SIZE; i++) {
        if (i == pos || cuts[i].used == false)
            continue;
        if (cuts[i].nLeaves > cuts[pos].nLeaves) {
            if ((cuts[i].sign & cuts[pos].sign) != cuts[pos].sign)
                continue;
            if (checkCutSubset(cuts[pos], cuts[i]))
                cuts[i].used = false;
        } else {
            if ((cuts[i].sign & cuts[pos].sign) != cuts[i].sign)
                continue;
            if (checkCutSubset(cuts[i], cuts[pos])) {
                cuts[pos].used = false;
                return true;
            }
        }
    }
    return false;
}

int DarEngine::cutPhase(const Cut &a, const Cut &b) {
    int phase = 0;
    for (int i = 0; i < b.nLeaves; i++) {
        for (int j = 0; j < a.nLeaves; j++) {
            if (a.leaves[j] == b.leaves[i])
                phase |= (1 << i);
        }
    }
    return phase;
}

unsigned DarEngine::truthTableMerge(const Cut &a, const Cut &b, const Cut &c,
                                    bool isC0, bool isC1) {
    unsigned aTruth = isC0 ? ~a.truthTable : a.truthTable;
    unsigned bTruth = isC1 ? ~b.truthTable : b.truthTable;
    aTruth = truthStretch(aTruth, a.nLeaves, cutPhase(a, c));
    bTruth = truthStretch(bTruth, b.nLeaves, cutPhase(b, c));
    return aTruth & bTruth;
}

bool DarEngine::minimizeCut(Cut &cut) {
    constexpr int masks[4][2] = {
        {0x5555, 0xAAAA}, {0x3333, 0xCCCC}, {0x0F0F, 0xF0F0}, {0x00FF, 0xFF00}};
    int phase = 0, truth = cut.truthTable & 0xFFFF, nLeaves = cut.nLeaves;
    for (int i = 0; i < cut.nLeaves; i++) {
        if ((truth & masks[i][0]) == ((truth & masks[i][1]) >> (1 << i))) {
            nLeaves--;
        } else {
            phase |= (1 << i);
        }
    }
    if (nLeaves == cut.nLeaves) {
        return false;
    }
    truth = truthShrink(truth, cut.nLeaves, phase);
    cut.truthTable = truth & 0xFFFF;
    cut.sign = 0;
    for (int i = 0, k = 0; i < cut.nLeaves; i++) {
        if (phase >> i & 1) {
            cut.leaves[k++] = cut.leaves[i];
            cut.sign |= (1 << (31 & cut.leaves[i]));
        }
    }
    cut.nLeaves = nLeaves;
    return true;
}

void countCut(RW::DarEngine::Cut *cuts) {
    int total = 0;
    for (int i = 0; i < MAX_CUT_SIZE; i++) {
        if (cuts[i].used == true) {
            printf("%d %d %d %d %d %u\n", cuts[i].nLeaves, cuts[i].leaves[0],
                   cuts[i].leaves[1], cuts[i].leaves[2], cuts[i].leaves[3],
                   cuts[i].truthTable);
            total++;
        }
    }
    printf("total %d\n", total);
}

void DarEngine::solveCutOneLevel(Cut *cuts, std::vector<int> &levelNode,
                                 int level) {
    // printf("level %d size = %d\n", level, (int) levelNode.size());
    if (level == 0) {
        for (int node : levelNode) {
            auto &cut = cuts[cutIdx(node, 0)];
            cut.used = true;
            cut.sign = 1u << (node & 31);
            cut.truthTable = 0xAAAA;
            cut.nLeaves = 1;
            cut.leaves[0] = node;
            cut.val = getCutValue(cut);
        }
    } else {
        // std::cout << "level " << level << " " << levelNode.size() <<
        // std::endl;
        // if (level == 2) {
        //     printf("node %d\n", levelNode.front());
        //     printf("in0 %d (%d) in1 %d (%d)\n",
        //     id(pFanin0[levelNode.front()]),
        //            levels[id(pFanin0[levelNode.front()])],
        //            id(pFanin1[levelNode.front()]),
        //            levels[id(pFanin1[levelNode.front()])]);
        // }
        for (int node : levelNode) {
            auto &cutNaive = cuts[cutIdx(node, 0)];
            cutNaive.used = true;
            cutNaive.sign = 1u << (node & 31);
            cutNaive.truthTable = 0xAAAA;
            cutNaive.nLeaves = 1;
            cutNaive.leaves[0] = node;
            cutNaive.val = getCutValue(cutNaive);
            int in0 = id(pFanin0[node]), in1 = id(pFanin1[node]);
            // printf("node %d in0 %d in1 %d\n", node, in0, in1);
            bool isC0 = isC(pFanin0[node]), isC1 = isC(pFanin1[node]);
            for (int i = 0; i < MAX_CUT_SIZE; i++) {
                auto &cut0 = cuts[cutIdx(in0, i)];
                if (cut0.used == true) {
                    for (int j = 0; j < MAX_CUT_SIZE; j++) {
                        auto &cut1 = cuts[cutIdx(in1, j)];
                        if (cut1.used == true) {
                            if (popCount(cut0.sign | cut1.sign) > 4)
                                continue;
                            int pos = findCutPosition(cuts, node);
                            auto &cut = cuts[cutIdx(node, pos)];
                            if (!mergeCut(cut0, cut1, cut))
                                continue;
                            if (checkCutRedundant(cuts + cutIdx(node, 0), pos))
                                continue;
                            cut.truthTable =
                                0xFFFF &
                                truthTableMerge(cut0, cut1, cut, isC0, isC1);
                            // if (node == 394) {
                            //     printf("cut for %d\n", node);
                            //     countCut(cuts + cutIdx(node, 0));
                            // }
                            if (minimizeCut(cut)) {
                                // if (node == 394) {
                                //     printf("cut for %d\n", node);
                                //     countCut(cuts + cutIdx(node, 0));
                                // }
                                checkCutRedundant(cuts + cutIdx(node, 0), pos);
                            }
                            cut.val = getCutValue(cut);
                        }
                    }
                }
            }
            // if (level <= 2 && node == levelNode.front()) {
            //     printf("%d %d %d\n", node, in0, in1);
            //     printf("cut for %d\n", node);
            //     countCut(cuts + cutIdx(node, 0));
            //     printf("cut for %d\n", in0);
            //     countCut(cuts + cutIdx(in0, 0));
            //     printf("cut for %d\n", in1);
            //     countCut(cuts + cutIdx(in1, 0));
            // }
        }
    }
}

void DarEngine::buildHashTable(){
    for (int i = 1; i <= nObjs; i++){
        if(pFanin0[i] == 0 && pFanin1[i] == 0)
            continue ;
        int in0 = id(pFanin0[i]), in1 = id(pFanin1[i]);
        bool isC0 = isC(pFanin0[i]), isC1 = isC(pFanin1[i]);
        auto hashValue = hash(hash(in0, in1), hash(isC0, isC1)) % BIG_PRIME;
        hashTable[BIG_PRIME + i - 1].value = i;
        while(true){
            if(hashValue >= BIG_PRIME + 2 * nObjs || hashValue < 0){
                assert(false);
            }
            size_t res = (hashTable[hashValue].next == K_EMPTY ? BIG_PRIME + i - 1 : K_EMPTY);
            hashTable[hashValue].next = res;
            if(res == K_EMPTY){
                break ;
            }
            else{
                hashValue = res;
            }
        }   
    }
}

int DarEngine::lookUp(int in0, int in1, bool isC0, bool isC1){
    if(in0 > in1){
        std::swap(in0, in1);
        std::swap(isC0, isC1);
    }
    auto hashValue = hash(hash(in0, in1), hash(isC0, isC1)) % BIG_PRIME;
    for (auto cur = hashTable[hashValue].next; cur != K_EMPTY; cur = hashTable[cur].next){
        int idx = hashTable[cur].value;
        if(id(pFanin0[idx]) == in0 && id(pFanin1[idx]) == in1 && isC(pFanin0[idx]) == isC0 && isC(pFanin1[idx]) == isC1){
            return idx;
        }
    }
    return K_EMPTY;
}

int DarEngine::id(int x) const { return x >> 1; }

bool DarEngine::isC(int x) const { return x & 1; }

RW_NAMESPACE_END