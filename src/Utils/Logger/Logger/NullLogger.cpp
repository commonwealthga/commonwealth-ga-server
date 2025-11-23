#include "src/Utils/Logger/Logger.hpp"

std::map<std::string, int> Logger::ChannelIndents;
std::vector<std::string> Logger::EnabledChannels;

void Logger::Log(const char* Channel, const char* Format, ...) {
}

void Logger::DumpMemory(char* Channel, void* Address, int Size, int NegativeSize) {
}

