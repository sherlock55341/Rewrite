#pragma once

#include <global.hpp>
#include <aig/aig.hpp>

RW_NAMESPACE_START

class Cmd{
public:
    Cmd(AIGManager& aigMan_);
    
    ~Cmd();

    bool parse(const std::string &line);
private:
    AIGManager& aigMan;

    unsigned lineCounter;

    void printHint();
};

RW_NAMESPACE_END