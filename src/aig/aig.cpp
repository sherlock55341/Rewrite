#include "aig.hpp"
#include <dar/dar.hpp>
#include <utils/utils.hpp>
#include <cpp/tools.hpp>

RW_NAMESPACE_START

AIGManager::AIGManager() {
    lut.readLUT();
    resetManager();
}

AIGManager::~AIGManager() { resetManager(); }

void AIGManager::printAIGLog() const {
    if (!aigCreated) {
        printf("Error : Empty network\n");
        return;
    }
    printf("%20s : I/O = %4d/%4d, latch = %3d and = %4d level = %4d\n",
           designName_.c_str(), nPIs, nPOs, nLatches, nNodes, nLevels);
}

int AIGManager::readAIGFromFile(const std::string &path) {
    auto fileSize = getFileSize(path);
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp || !fileSize) {
        printf("Error : Read aig file %s failed, empty or not exists.\n",
               path.c_str());
        return -1;
    }
    char *fc = nullptr;
    fc = (char *)malloc(sizeof(char) * fileSize);
    if (fc == nullptr) {
        printf("Error : Malloc %lld chars error in reading aig\n",
               (unsigned long long)fileSize);
        return -1;
    }
    size_t length = fread(fc, fileSize, 1, fp);
    fclose(fp);
    int ret = readAIGFromMemory(fc, fileSize);
    free(fc);
    if (ret == 0) {
        designPath_ = path;
        designName_ = getDesignName(path);
    }
    printf("    Read aig file finished, now memory usage %.2lf MB, peak memory usage %.2lf MB\n", utils::tools::MemReporter::getCurrent(), utils::tools::MemReporter::getPeak());
    return ret;
}

int AIGManager::readAIGFromMemory(char *fc, size_t length) {
    int nTotal_ = 0, nInputs_ = 0, nLatches_ = 0, nOutputs_ = 0, nAnds_ = 0;
    resetManager();
    if (strncmp(fc, "aig", 3) != 0 || fc[3] != ' ') {
        printf("Error : The file is not in aig format\n");
        return -1;
    }
    auto cur = fc;

    auto filterSpace = [&]() {
        while (*cur != ' ')
            cur++;
        cur++;
    };

    filterSpace();
    nTotal_ = atoi(cur);
    filterSpace();
    nInputs_ = atoi(cur);
    filterSpace();
    nLatches_ = atoi(cur);
    filterSpace();
    nOutputs_ = atoi(cur);
    filterSpace();
    nAnds_ = atoi(cur);

    if (nLatches_ != 0) {
        printf("Error : Program does not support latches\n");
        return -1;
    }

    if (nTotal_ != nInputs_ + nAnds_) {
        printf("Error : Total node number (%d) is not equal to input node "
               "number (%d) "
               "and and node number (%d)\n",
               nTotal_, nInputs_, nAnds_);
        return -1;
    }

    nObjs = nTotal_ + 1;
    nPIs = nInputs_;
    nPOs = nOutputs_;
    nNodes = nAnds_;

    pFanin0 = (int *)malloc(sizeof(int) * nObjs);
    pFanin1 = (int *)malloc(sizeof(int) * nObjs);
    pOuts = (int *)malloc(sizeof(int) * nPOs);

    memset(pFanin0, -1, sizeof(int) * nObjs);
    memset(pFanin1, -1, sizeof(int) * nObjs);

    while (*cur != ' ' && *cur != '\n') {
        cur++;
    }
    cur++;

    auto drivers = cur;

    for (int i = 0; i < nOutputs_;) {
        if (*cur == '\n') {
            i++;
        }
        cur++;
    }

    std::vector<int> levels(nObjs, -1);

    for (int i = 0; i <= nInputs_; i++) {
        levels[i] = 0;
    }

    for (size_t i = 1 + nInputs_; i < nObjs; i++) {
        size_t ulit = i << 1;
        auto ulit1 = ulit - decode(&cur);
        auto ulit0 = ulit1 - decode(&cur);

        pFanin0[i] = invert(ulit0);
        pFanin1[i] = invert(ulit1);

        levels[i] = 1 + std::max(levels[ulit0 >> 1], levels[ulit1 >> 1]);
    }

    nLevels = *std::max_element(levels.begin(), levels.end());

    auto symbols = cur;

    cur = drivers;

    // printf("outputs : ");

    for (int i = 0; i < nOutputs_; i++) {
        pOuts[i] = invert(atoi(cur));
        // printf("%d ", pOuts[i]);
        while (*cur != '\n') {
            cur++;
        }
        cur++;
    }
    // printf("\n");

    aigCreated = true;
    return 0;
}

int AIGManager::saveAIG2File(const std::string &path) {
    if (!aigCreated) {
        printf("Error : Empty network\n");
        return -1;
    }
    auto fp = fopen(path.c_str(), "wb");
    fprintf(fp, "aig %d %d %d %d %d\n", nObjs - 1, nPIs, nLatches, nPOs,
            nNodes);
    for (int i = 0; i < nPOs; i++) {
        auto lit = pOuts[i];
        fprintf(fp, "%d\n", lit);
    }
    char *buf = new char[nNodes * 30];
    int cur = 0;
    for (int i = nPIs + 1; i < nObjs; i++) {
        auto lit0 = pFanin0[i];
        auto lit1 = pFanin1[i];
        encode(buf, cur, 2 * i - lit1);
        encode(buf, cur, lit1 - lit0);
    }
    fwrite(buf, sizeof(char), cur * sizeof(char), fp);
    fprintf(fp, "c\n");
    fprintf(fp, "%s\n", comments_.c_str());
    fclose(fp);
    delete[] buf;
    printf("    Save aig file finished, now memory usage %.2lf MB, peak memory usage %.2lf MB\n", utils::tools::MemReporter::getCurrent(), utils::tools::MemReporter::getPeak());
    return 0;
}

void AIGManager::rewrite() {
    if (!aigCreated) {
        printf("Error : rewrite cannot be done before AIG is created\n");
        return;
    }
    DarEngine dar(lut);
    dar.rewrite(nObjs, nPIs, nPOs, pFanin0, pFanin1, pOuts, false);

    std::vector<int> levels(nObjs, -1);
    for (int i = 1; i <= nPIs; i++){
        levels[i] = 0;
    }
    for (int i = nPIs + 1; i < nObjs; i++){
        levels[i] = 1 + std::max(levels[pFanin0[i] >> 1], levels[pFanin1[i] >> 1]);
    }
    nLevels = *std::max_element(levels.begin(), levels.end());
    nNodes = nObjs - nPIs - 1;
}

RW_NAMESPACE_END