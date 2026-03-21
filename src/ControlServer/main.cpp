#include "src/ControlServer/Logger.hpp"
#include <cstdio>

int main(int argc, char* argv[]) {
    Logger::Log("main", "Control server starting...\n");
    return 0;
}
