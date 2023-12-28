#include "dar.hpp"
#include <cpp/tools.hpp>
#include <utils/utils.hpp>

RW_NAMESPACE_START

constexpr size_t MAX_AIG_SIZE = 1e5;
constexpr size_t MAX_CUT_SIZE = 8;
constexpr size_t TABLE_SIZE = 8;
constexpr size_t MATCH_SIZE = 54;
constexpr int K_EMPTY = -1;

DarEngine::DarEngine(LUT &lut_) : lut(lut_) {
    pFanin0 = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    pFanin1 = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    pOuts = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    levels = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    nRef = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    del = (bool *)malloc(sizeof(bool) * MAX_AIG_SIZE);
    visTime = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    bestCuts = (Cut *)malloc(sizeof(Cut) * MAX_AIG_SIZE);
    bestSubgr = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    created = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    phase = (int *)malloc(sizeof(int) * MAX_AIG_SIZE);
    std::fill(pFanin0, pFanin0 + MAX_AIG_SIZE, -1);
    std::fill(pFanin1, pFanin1 + MAX_AIG_SIZE, -1);
    std::fill(pOuts, pOuts + MAX_AIG_SIZE, -1);
    std::fill(levels, levels + MAX_AIG_SIZE, 0);
    std::fill(nRef, nRef + MAX_AIG_SIZE, 0);
    std::fill(visTime, visTime + MAX_AIG_SIZE, -1);
    std::fill(created, created + MAX_AIG_SIZE, -1);
    std::fill(del, del + MAX_AIG_SIZE, 0);
    nObjs = 0;
    nPIs = 0;
    nPOs = 0;
    nLevel = 0;
    timeStamp = 0;
    expected = 0;
}

DarEngine::~DarEngine() {
    free(pFanin0);
    free(pFanin1);
    free(pOuts);
    free(levels);
    free(del);
    free(visTime);
    free(bestCuts);
    free(bestSubgr);
}

void DarEngine::rewrite(int &nObjs_, int nPIs_, int nPOs_, int *pFanin0_,
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
        // printf("%d ", (int)levelNodes[level].size());
        // printf("first of level %d %d\n", level, levelNodes[level][0]);
        enumerateCutOneLevel(cuts, levelNodes[level], level);
    }

    int totalCut = 0;

    for (int i = 0; i < nObjs * MAX_CUT_SIZE; i++) {
        if (cuts[i].used == true) {
            totalCut++;
        }
    }
    printf("    #Cuts enumerated : %d\n", totalCut);

    hashTable =
        (TableNode *)malloc(sizeof(TableNode) * (BIG_PRIME + nObjs * 2));

    printf("    Rewriting..., now memory usage = %.2lf MB, peak memory usage = "
           "%.2lf MB\n",
           utils::tools::MemReporter::getCurrent(),
           utils::tools::MemReporter::getPeak());

    for (int i = 0; i < BIG_PRIME + nObjs * 2; i++) {
        hashTable[i].next = K_EMPTY;
    }

    buildHashTable();

    // std::cout << "use zero " << useZero << std::endl;

    evaluateNode(cuts, useZero);

    totalCut = 0;
    for (int i = 0; i < nObjs; i++) {
        if (bestCuts[i].used == true) {
            totalCut++;
        }
    }

    buildReplaceHashTable();

    printf("    #Cuts best : %d\n", totalCut);

    int originObjs = nObjs;
    for (int i = nPIs + 1; i < originObjs; i++) {
        evalAndReplace(i, bestCuts[i]);
    }

    for (int i = nPIs + 1; i < nObjs; i++) {
        if (!del[i]) {
            if (!isRedundant(i)) {
                pFanin0[i] = getFanin(pFanin0[i]);
                pFanin1[i] = getFanin(pFanin1[i]);
                // std::cout << "(" << pFanin0[i] << ", " << pFanin1[i] << ")" << std::endl;
            } else {
                del[i] = true;
            }
        }
    }

    for (int i = 0; i < nPOs; i++) {
        pOuts[i] = getFanin(pOuts[i]);
    }


    free(cuts);
    free(hashTable);

    cuts = nullptr;
    hashTable = nullptr;

    reorder();
    printf("    After rewrite, nObjs = %d, level = %d\n", nObjs, nLevel);
    printf("    Rewriting..., now memory usage = %.2lf MB, peak memory usage = "
           "%.2lf MB\n",
           utils::tools::MemReporter::getCurrent(),
           utils::tools::MemReporter::getPeak());

    memcpy(pFanin0_, pFanin0, sizeof(int) * nObjs);
    memcpy(pFanin1_, pFanin1, sizeof(int) * nObjs);
    memcpy(pOuts_, pOuts, sizeof(int) * nPOs);
    nObjs_ = nObjs;
}

void DarEngine::reorder() {
    int newObjs = 1;
    for (int i = 1; i < nObjs; i++) {
        if (!del[i])
            newObjs++;
    }
    // std::cout << "new objs : " << newObjs << std::endl;
    std::vector<int> pos(nObjs, 0), f0(nObjs, 0), f1(nObjs, 0),
        counts(nObjs, 0);
    std::fill(levels, levels + nObjs, -1);
    levels[0] = -1;
    for (int i = 1; i <= nPIs; i++) {
        levels[i] = 0;
    }
    for (int i = nPIs + 1; i < nObjs; i++)
        if (!del[i]) {
            dfs(i);
        }
    nLevel = *std::max_element(levels, levels + nObjs);
    // std::cout << "nlevel " << nLevel << std::endl;
    for (int i = 1; i < nObjs; i++)
        if (!del[i]) {
            counts[levels[i]]++;
        }
    for (int i = 1; i <= nLevel; i++)
        counts[i] += counts[i - 1];
    for (int i = nObjs - 1; i > 0; i--)
        if (!del[i]) {
            pos[i] = counts[levels[i]];
            counts[levels[i]]--;
        }
    for (int i = nPIs + 1; i < nObjs; i++) {
        f0[i] = pFanin0[i];
        f1[i] = pFanin1[i];
    }
    for (int i = nPIs + 1; i < nObjs; i++)
        if (!del[i]) {
            pFanin0[pos[i]] = (pos[id(f0[i])] << 1 | isC(f0[i]));
            pFanin1[pos[i]] = (pos[id(f1[i])] << 1 | isC(f1[i]));
        }

    for (int i = 0; i < nPOs; i++) {
        pOuts[i] = (pos[id(pOuts[i])] << 1 | isC(pOuts[i]));
    }

    nObjs = newObjs;
}

void DarEngine::buildReplaceHashTable() {
    table.clear();
    table.reserve(2 * nObjs);
    for (int i = nPIs + 1; i < nObjs; i++) {
        auto in0 = pFanin0[i], in1 = pFanin1[i];
        if (in0 > in1) {
            std::swap(in0, in1);
        }
        table[std::make_pair(in0, in1)] = i;
        // if(i == 834){
        //     std::cout << i << " " << in0 << " " << in1 << std::endl;
        //     assert(false);
        // }
        // if (in0 == 560 && in1 == 1461) {
        //     std::cout << in0 << " " << in1 << " -> " << i << std::endl;
        // }
    }
}

void DarEngine::evalAndReplace(int idx, Cut &cut) {
    if (cut.used == false || cut.nLeaves < 3) {
        return;
    }
    if (cut.nLeaves == 3) {
        cut.leaves[cut.nLeaves++] = 0;
    }
    for (int i = 0; i < cut.nLeaves; i++) {
        if (del[cut.leaves[i]] == true)
            return;
    }
    timeStamp++;
    int saved = markMFFC(idx, cut);
    auto uPhase = lut.pPhase[cut.truthTable];
    auto Class = lut.pMap[cut.truthTable];
    auto pPerm = lut.pPerms4[lut.pPerms[cut.truthTable]];
    std::vector<int> match(54, -1), isCom(54, false);
    for (int i = 0; i < 4; i++) {
        match[i] = cut.leaves[pPerm[i]];
        isCom[i] = uPhase >> i & 1;
    }
    int rt = lut.pSubgr[Class][bestSubgr[idx]];
    std::vector<int> used(lut.nNodes[Class] + 4, 0);
    used[rt] = 1;
    for (int i = rt; i >= 4; i--) {
        if (used[i]) {
            used[lut.fanin0[Class][i - 4]] = 1;
            used[lut.fanin1[Class][i - 4]] = 1;
        }
    }
    for (int i = 0; i < lut.nNodes[Class]; i++) {
        if (used[i + 4]) {
            int num = i + 4;
            int in0 = lut.fanin0[Class][i], in1 = lut.fanin1[Class][i];
            if (match[in0] == -1 || match[in1] == -1) {
                continue;
            }
            auto it =
                match[in0] < match[in1]
                    ? table.find(std::make_pair(
                          match[in0] * 2 + (isCom[in0] ^ lut.isC0[Class][i]),
                          match[in1] * 2 + (isCom[in1] ^ lut.isC1[Class][i])))
                    : table.find(std::make_pair(
                          match[in1] * 2 + (isCom[in1] ^ lut.isC1[Class][i]),
                          match[in0] * 2 + (isCom[in0] ^ lut.isC0[Class][i])));
            auto nodeIdx = (it == table.end() ? -1 : it->second);
            if (nodeIdx != -1) {
                match[num] = nodeIdx;
            }
        }
    }
    int added = eval2(rt, Class, match);
    if (match[rt] == idx)
        return;

    if (saved - added > 0) {
        expected += saved - added;
        build(rt, Class, match, isCom);
        replace(match[rt], idx, phase[match[rt]] ^ phase[idx]);
    }
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
    // printf("outputs : ");
    for (int i = 0; i < nPOs; i++) {
        pOuts[i] = (pos[id(pOuts[i])] << 1 | isC(pOuts[i]));
        // printf("%d ", pOuts[i]);
    }
    // printf("\n");
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
        if (pFanin0[i] > pFanin1[i]) {
            std::swap(pFanin0[i], pFanin1[i]);
        }
        nRef[id(pFanin0[i])]++;
        nRef[id(pFanin1[i])]++;
        phase[i] = (phase[id(pFanin0[i])] ^ isC(pFanin0[i])) &
                   (phase[id(pFanin1[i])] ^ isC(pFanin1[i]));
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

// void countCut(RW::DarEngine::Cut *cuts) {
//     int total = 0;
//     for (int i = 0; i < MAX_CUT_SIZE; i++) {
//         if (cuts[i].used == true) {
//             printf("%d %d %d %d %d %u\n", cuts[i].nLeaves, cuts[i].leaves[0],
//                    cuts[i].leaves[1], cuts[i].leaves[2], cuts[i].leaves[3],
//                    cuts[i].truthTable);
//             total++;
//         }
//     }
//     printf("total %d\n", total);
// }

void DarEngine::enumerateCutOneLevel(Cut *cuts, std::vector<int> &levelNode,
                                     int level) {
    // printf("level %d size = %d\n", level, (int) levelNode.size());
    if (level == 0) {
        for (int node : levelNode) {
            for (int i = 0; i < MAX_CUT_SIZE; i++) {
                cuts[cutIdx(node, 0)].used = false;
            }
            auto &cut = cuts[cutIdx(node, 0)];
            cut.used = true;
            cut.sign = 1 << (node & 31);
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
            for (int i = 0; i < MAX_CUT_SIZE; i++) {
                cuts[cutIdx(node, 0)].used = false;
            }
            auto &cutNaive = cuts[cutIdx(node, 0)];
            cutNaive.used = true;
            cutNaive.sign = 1 << (node & 31);
            cutNaive.truthTable = 0xAAAA;
            cutNaive.nLeaves = 1;
            cutNaive.leaves[0] = node;
            cutNaive.val = getCutValue(cutNaive);
            int in0 = id(pFanin0[node]), in1 = id(pFanin1[node]);
            // printf("node %d in0 %d in1 %d\n", node, in0, in1);
            bool isC0 = isC(pFanin0[node]), isC1 = isC(pFanin1[node]);
            bool found2 = false;
            for (int i = 0; i < MAX_CUT_SIZE && !found2; i++) {
                auto cut0 = cuts[cutIdx(in0, i)];
                if (cut0.used == true) {
                    for (int j = 0; j < MAX_CUT_SIZE && !found2; j++) {
                        auto cut1 = cuts[cutIdx(in1, j)];
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
                            if (cut.nLeaves < 2) {
                                found2 = true;
                            }
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

void DarEngine::buildHashTable() {
    for (int i = 1; i < nObjs; i++) {
        if (pFanin0[i] == 0 && pFanin1[i] == 0)
            continue;
        int in0 = id(pFanin0[i]), in1 = id(pFanin1[i]);
        bool isC0 = isC(pFanin0[i]), isC1 = isC(pFanin1[i]);
        auto hashValue = hash(hash(in0, in1), hash(isC0, isC1)) % BIG_PRIME;
        hashTable[BIG_PRIME + i - 1].value = i;
        while (true) {
            if (hashValue >= BIG_PRIME + 2 * nObjs || hashValue < 0) {
                assert(false);
            }
            size_t res = hashTable[hashValue].next;
            hashTable[hashValue].next = (hashTable[hashValue].next == K_EMPTY
                                             ? BIG_PRIME + i - 1
                                             : hashTable[hashValue].next);
            if (res == K_EMPTY) {
                break;
            } else {
                hashValue = res;
            }
        }
    }
}

int DarEngine::lookUp(int in0, int in1, bool isC0, bool isC1) {
    if (in0 > in1) {
        std::swap(in0, in1);
        std::swap(isC0, isC1);
    }
    auto hashValue = hash(hash(in0, in1), hash(isC0, isC1)) % BIG_PRIME;
    for (auto cur = hashTable[hashValue].next; cur != K_EMPTY;
         cur = hashTable[cur].next) {
        int idx = hashTable[cur].value;
        if (id(pFanin0[idx]) == in0 && id(pFanin1[idx]) == in1 &&
            isC(pFanin0[idx]) == isC0 && isC(pFanin1[idx]) == isC1) {
            return idx;
        }
    }
    return K_EMPTY;
}

int DarEngine::decrease(int idx, Cut &cut, int *tableSize, int *tableIdx,
                        int *tableNum) {
    for (int i = 0; i < cut.nLeaves; i++) {
        if (idx == cut.leaves[i]) {
            return 1;
        }
    }
    for (int i = 0; i < tableSize[0]; i++) {
        if (tableIdx[i] == idx)
            return --tableNum[i];
    }
    if (tableSize[0] == TABLE_SIZE) {
        return 1;
    }
    tableIdx[tableSize[0]] = idx;
    tableNum[tableSize[0]] = nRef[idx] - 1;
    tableSize[0]++;
    return nRef[idx] - 1;
}

int DarEngine::calcMFFC(int idx, Cut &cut, int *tableSize, int *tableIdx,
                        int *tableNum, int root) {
    int res = 1;
    int in0 = id(pFanin0[idx]), in1 = id(pFanin1[idx]);
    if (decrease(in0, cut, tableSize, tableIdx, tableNum) == 0) {
        res += calcMFFC(in0, cut, tableSize, tableIdx, tableNum, root);
    }
    if (decrease(in1, cut, tableSize, tableIdx, tableNum) == 0) {
        res += calcMFFC(in1, cut, tableSize, tableIdx, tableNum, root);
    }
    return res;
}

bool DarEngine::isDeleted(int idx, int *tableSize, int *tableIdx,
                          int *tableNum) {
    for (int i = 0; i < tableSize[0]; i++) {
        if (tableIdx[i] == idx)
            return tableNum[i] == 0;
    }
    return false;
}

int DarEngine::eval(int matchIdx, int *match, int Class, int idx) {
    if (match[matchIdx] > -1)
        return 0;
    if (match[matchIdx] == idx)
        return 0;
    match[matchIdx] = idx;
    return 1 + eval(lut.fanin0[Class][matchIdx - 4], match, Class, idx) +
           eval(lut.fanin1[Class][matchIdx - 4], match, Class, idx);
}

int DarEngine::eval2(int idx, int Class, const std::vector<int> &match) {
    if (match[idx] != -1 && visTime[match[idx]] != timeStamp)
        return 0;
    if (created[idx] == timeStamp)
        return 0;
    created[idx] = timeStamp;
    return 1 + eval2(lut.fanin0[Class][idx - 4], Class, match) +
           eval2(lut.fanin1[Class][idx - 4], Class, match);
}

void DarEngine::evaluateNode(Cut *cuts, bool useZero) {
    for (int idx = 1; idx < nObjs; idx++) {
        bestCuts[idx].used = false;
        int best = 1, bestLevel = std::numeric_limits<int>::max();
        int bestCutIdx = -1;
        int bestOut = -1;
        for (int i = 0; i < MAX_CUT_SIZE; i++) {
            auto &cut = cuts[cutIdx(idx, i)];
            if (cut.used == false || cut.nLeaves < 3)
                continue;
            int nleaves = cut.nLeaves;
            if (nleaves == 3) {
                cut.leaves[cut.nLeaves++] = 0;
            }
            int tableSize = 0;
            int tableIdx[TABLE_SIZE], tableNum[TABLE_SIZE];
            int match[MATCH_SIZE], matchLevel[MATCH_SIZE];
            int saved = calcMFFC(idx, cut, &tableSize, tableIdx, tableNum, idx);
            int uPhase = lut.pPhase[cut.truthTable];
            int Class = lut.pMap[cut.truthTable];
            auto pPerm = lut.pPerms4[lut.pPerms[cut.truthTable]];
            for (int j = 0; j < MATCH_SIZE; j++) {
                match[j] = -1;
                matchLevel[j] = -1;
            }
            uint64_t isc = 0;
            for (int j = 0; j < 4; j++) {
                match[j] = cut.leaves[pPerm[j]];
                matchLevel[j] = levels[match[j]];
                if (uPhase >> j & 1) {
                    isc |= 1ull << j;
                }
            }
            // std::cout << "nNodes : " << lut.nNodes[Class] << std::endl;
            for (int j = 0; j < lut.nNodes[Class]; j++) {
                int num = j + 4;
                int in0 = lut.fanin0[Class][j];
                int in1 = lut.fanin1[Class][j];
                matchLevel[num] =
                    1 + std::max(matchLevel[in0], matchLevel[in1]);
                if (match[in0] == -1 || match[in1] == -1 || match[in0] == idx ||
                    match[in1] == idx) {
                    continue;
                }
                int nodeIdx = lookUp(match[in0], match[in1],
                                     (isc >> in0 & 1) ^ lut.isC0[Class][j],
                                     (isc >> in1 & 1) ^ lut.isC1[Class][j]);
                if (nodeIdx != -1 &&
                    !isDeleted(nodeIdx, &tableSize, tableIdx, tableNum)) {
                    match[num] = nodeIdx;
                    matchLevel[num] = levels[nodeIdx];
                }
            }
            for (int out = 0; out < lut.nSubgr[Class]; out++) {
                int rt = lut.pSubgr[Class][out];
                if (match[rt] == idx)
                    continue;
                int nodesAdded = eval(rt, match, Class, -out - 2);
                int rtLevel = matchLevel[rt];
                int benefit = saved - nodesAdded;
                if (benefit < 0)
                    continue;
                if (!useZero && (benefit == 0))
                    continue;
                if (benefit < best)
                    continue;
                if (benefit == best && rtLevel >= bestLevel)
                    continue;
                best = benefit;
                bestLevel = rtLevel;
                bestCutIdx = i;
                bestOut = out;
            }
            cut.nLeaves = nleaves;
        }
        if (bestCutIdx != -1) {
            bestCuts[idx] = cuts[cutIdx(idx, bestCutIdx)];
            bestCuts[idx].used = true;
            bestSubgr[idx] = bestOut;
        } else {
            bestCuts[idx].used = false;
        }
    }

    // for (int idx = 1; idx < nObjs; idx++) {
    //     if (bestCuts[idx].used == true) {
    //         printf("(%d, %d) ", idx, bestSubgr[idx]);
    //     }
    // }
    // printf("\n");
}

int DarEngine::markMFFC(int idx, Cut &cut) {
    for (int i = 0; i < cut.nLeaves; i++) {
        nRef[cut.leaves[i]]++;
    }
    int num = doDeref(idx);
    doRef(idx);
    for (int i = 0; i < cut.nLeaves; i++) {
        nRef[cut.leaves[i]]--;
    }
    return num;
}

int DarEngine::isRedundant(int idx) const {
    return idx > nPIs && (pFanin0[idx] == pFanin1[idx] || pFanin0[idx] == 1);
}

int DarEngine::doDeref(int idx) {
    visTime[idx] = timeStamp;
    int num = !isRedundant(idx);
    nRef[id(pFanin0[idx])]--;
    if (nRef[id(pFanin0[idx])] == 0) {
        num += doDeref(id(pFanin0[idx]));
    }
    nRef[id(pFanin1[idx])]--;
    if (nRef[id(pFanin1[idx])] == 0) {
        num += doDeref(id(pFanin1[idx]));
    }
    return num;
}

void DarEngine::doRef(int idx) {
    if (nRef[id(pFanin0[idx])] == 0) {
        doRef(id(pFanin0[idx]));
    }
    nRef[id(pFanin0[idx])]++;
    if (nRef[id(pFanin1[idx])] == 0) {
        doRef(id(pFanin1[idx]));
    }
    nRef[id(pFanin1[idx])]++;
}

void DarEngine::build(int cur, int Class, std::vector<int> &match,
                      const std::vector<int> &isc) {
    if (match[cur] != -1)
        return;
    int in0 = lut.fanin0[Class][cur - 4], in1 = lut.fanin1[Class][cur - 4];
    build(in0, Class, match, isc);
    build(in1, Class, match, isc);
    match[cur] = genAnd(match[in0] * 2 + (lut.isC0[Class][cur - 4] ^ isc[in0]),
                        match[in1] * 2 + (lut.isC1[Class][cur - 4] ^ isc[in1]));
}

void DarEngine::replace(int newn, int oldn, int isc) {
    nRef[newn]++;
    delet(oldn, 0);
    nRef[newn]--;
    if (nRef[newn] || isc) {
        connect(oldn, 1, 2 * newn + isc);
    } else {
        int in0 = pFanin0[newn], in1 = pFanin1[newn];
        disconnect(newn);
        connect(oldn, in0, in1);
        del[newn] = true;
    }
}

void DarEngine::delet(int node, int flag) {
    if (node <= nPIs)
        return;
    int in0 = id(pFanin0[node]), in1 = id(pFanin1[node]);
    disconnect(node);
    if (nRef[in0] == 0) {
        delet(in0);
    }
    if (nRef[in1] == 0) {
        delet(in1);
    }
    del[node] = flag;
}

void DarEngine::connect(int node, int in0, int in1) {
    if (in0 > in1) {
        std::swap(in0, in1);
    }
    pFanin0[node] = in0;
    pFanin1[node] = in1;
    nRef[id(in0)]++;
    nRef[id(in1)]++;
    levels[node] = std::max(levels[id(in0)], levels[id(in1)]) + 1;
    phase[node] = (phase[id(in0)] ^ isC(in0)) & (phase[id(in1)] ^ isC(in1));
    table[std::make_pair(in0, in1)] = node;
}

void DarEngine::disconnect(int node) {
    int in0 = pFanin0[node];
    int in1 = pFanin1[node];
    nRef[id(in0)]--;
    nRef[id(in1)]--;
    if (in0 > in1) {
        std::swap(in0, in1);
    }
    table.erase(std::make_pair(in0, in1));
    pFanin0[node] = -1;
    pFanin1[node] = -1;
}

int DarEngine::genAnd(int in0, int in1) {
    int node = nObjs++;
    nRef[node] = 0;
    del[node] = false;
    if (in0 > in1) {
        std::swap(in0, in1);
    }
    connect(node, in0, in1);
    return node;
}

int DarEngine::getFanin(int x) {
    if (!isRedundant(id(x))) {
        return x;
    }
    return isC(x) ^ getFanin(pFanin1[id(x)]);
}

int DarEngine::dfs(int x){
    if(levels[x] >= 0)
        return levels[x];
    return levels[x] = 1 + std::max(dfs(id(pFanin0[x])), dfs(id(pFanin1[x])));
}

int DarEngine::id(int x) const { return x >> 1; }

bool DarEngine::isC(int x) const { return x & 1; }

RW_NAMESPACE_END