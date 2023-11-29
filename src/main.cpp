#include <cmd/cmd.hpp>
#include "global.hpp"

int main(int argc, char** argv) {
    RW::AIGManager aigMan;
    std::string line;
    RW::Cmd cmdController(aigMan);

    while (getline(std::cin, line)) {
        if(!cmdController.parse(line)){
            break;
        }
    }
}