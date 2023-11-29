#include "utils.hpp"

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

unsigned decode(char **pos){
    unsigned x = 0, i = 0;
    unsigned char ch;
    while((ch = *(*pos)++) & 0x80){
        x |= (ch & 0x7f) << (7 * i++);
    }
    return x | (ch << (7 * i));
}

void encode(char *buf, int &cur, unsigned x){
    unsigned char ch;
    while(x & ~0x7f){
        ch = (x & 0x7f) | 0x80;
        buf[cur++] = ch;
        x >>= 7;
    }
    ch = x;
    buf[cur++] = ch;
}

std::string getDesignName(const std::string& path){
    char *input = new char[path.length() + 1];
    for (int i = 0; i < path.length(); i++){
        input[i] = path[i];
    }
    input[path.length()] = '\0';
    char *token = std::strtok(input, "/");
    std::string prev;
    while(token != nullptr){
        prev = token;
        token = std::strtok(nullptr, "/");
    }
    for (int i = 0; i < prev.length(); i++){
        input[i] = prev[i];
    }
    input[prev.length()] = '\0';
    token = std::strtok(input, ".");
    prev.clear();
    auto l = strlen(token);
    for (int i = 0; i < l; i++){
        prev.push_back(token[i]);
    }
    free(input);
    return prev;
}

RW_NAMESPACE_END