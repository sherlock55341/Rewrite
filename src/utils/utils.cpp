#include "utils.hpp"
#include <limits>

RW_NAMESPACE_START

size_t getFileSize(const std::string &path) {
    FILE *fp = fopen(path.c_str(), "r");
    size_t res = 0;
    if (fp == nullptr) {
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    res = ftell(fp);
    fclose(fp);
    return res;
}

unsigned decode(char **pos) {
    unsigned x = 0, i = 0;
    unsigned char ch;
    while ((ch = *(*pos)++) & 0x80) {
        x |= (ch & 0x7f) << (7 * i++);
    }
    return x | (ch << (7 * i));
}

void encode(char *buf, int &cur, unsigned x) {
    unsigned char ch;
    while (x & ~0x7f) {
        ch = (x & 0x7f) | 0x80;
        buf[cur++] = ch;
        x >>= 7;
    }
    ch = x;
    buf[cur++] = ch;
}

std::string getDesignName(const std::string &path) {
    char *input = new char[path.length() + 1];
    for (int i = 0; i < path.length(); i++) {
        input[i] = path[i];
    }
    input[path.length()] = '\0';
    char *token = std::strtok(input, "/");
    std::string prev;
    while (token != nullptr) {
        prev = token;
        token = std::strtok(nullptr, "/");
    }
    for (int i = 0; i < prev.length(); i++) {
        input[i] = prev[i];
    }
    input[prev.length()] = '\0';
    token = std::strtok(input, ".");
    prev.clear();
    auto l = strlen(token);
    for (int i = 0; i < l; i++) {
        prev.push_back(token[i]);
    }
    free(input);
    return prev;
}

int popCount(unsigned x) {
    constexpr unsigned MASK_1 = 0x55555555;
    constexpr unsigned MASK_2 = 0x33333333;
    constexpr unsigned MASK_4 = 0x0F0F0F0F;
    constexpr unsigned MASK_8 = 0x00FF00FF;
    constexpr unsigned MASK_16 = 0x0000FFFF;
    x = (x & MASK_1) + ((x >> 1) & MASK_1);
    x = (x & MASK_2) + ((x >> 2) & MASK_2);
    x = (x & MASK_4) + ((x >> 4) & MASK_4);
    x = (x & MASK_8) + ((x >> 8) & MASK_8);
    return (x & MASK_16) + (x >> 16);
}

unsigned truthSwap(unsigned truth, int bias) {
    if(bias == 0)
        return (truth & 0x99999999) | ((truth & 0x22222222) << 1) |
                 ((truth & 0x44444444) >> 1);
    if(bias == 1)
        return (truth & 0xC3C3C3C3) | ((truth & 0x0C0C0C0C) << 2) |
                 ((truth & 0x30303030) >> 2);
    if(bias == 2)
        return (truth & 0xF00FF00F) | ((truth & 0x00F000F0) << 4) |
                 ((truth & 0x0F000F00) >> 4);
    return 0;
}

unsigned truthStretch(unsigned truth, int n, int phase) {
    n--;
    for (int i = 3; i >= 0; i--) {
        if (phase >> i & 1) {
            for (int k = n; k < i; k++) {
                truth = truthSwap(truth, k);
            }
            n--;
        }
    }
    return truth;
}

unsigned truthShrink(unsigned truth, int n, int phase){
    int var = 0;
    for (int i = 0; i < 4; i++){
        if(phase >> i & 1){
            for (int k = i - 1; k >= var; k--){
                truth = truthSwap(truth, k);
            }
            var++;
        }
    }
    return truth;
}

bool isPrime(int x){
    for (int i = 2; i * i <= x; i++){
        if(x % i == 0){
            return false;
        }
    }
    return true;
}

int getNearestPrime(int x){
    x++;
    while(!isPrime(x)){
        x++;
    }
    return x;
}

RW_NAMESPACE_END