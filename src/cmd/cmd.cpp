#include "cmd.hpp"

RW_NAMESPACE_START

Cmd::Cmd(AIGManager &aigMan_) : aigMan(aigMan_), lineCounter(0) {printHint();}

Cmd::~Cmd() {}

void Cmd::printHint() { printf("rewrite %02u > ", ++lineCounter); }

bool Cmd::parse(const std::string &line) {
    std::stringstream ss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    if (tokens.size() > 0) {
        if (tokens[0] == "read") {
            if (tokens.size() != 2) {
                printf("Error : read format wrong\n");
            } else {
                aigMan.readAIGFromFile(tokens[1]);
            }
        } else if (tokens[0] == "print_stats") {
            if (tokens.size() != 1) {
                printf("Error : print_stats format wrong\n");
            } else {
                aigMan.printAIGLog();
            }
        } else if (tokens[0] == "write_aig") {
            if (tokens.size() != 2) {
                printf("Error : write format wrong\n");
            } else {
                aigMan.saveAIG2File(tokens[1]);
            }
        } else if (tokens[0] == "quit") {
            if (tokens.size() != 1) {
                printf("Error : quit format wrong\n");
            } else {
                return false;
            }
        } else if (tokens[0] == "rewrite") {
            if (tokens.size() == 1) {
                aigMan.rewrite();
            } else {
                printf("Error : rewrite format wrong\n");
            }
        } else {
            printf("Error : command cannot understand\n");
        }
    }
    printHint();
    return true;
}

RW_NAMESPACE_END