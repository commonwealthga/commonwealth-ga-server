#include "src/Utils/Logger/Logger.hpp"

std::map<std::string, int> Logger::ChannelIndents;
std::vector<std::string> Logger::EnabledChannels;

bool Logger::IsChannelEnabled(const char* Channel) {
	// NullLogger never emits anything — always tell callers the channel is
	// off so they skip building expensive log args.
	return false;
}

void Logger::Log(const char* Channel, const char* Format, ...) {
}

void Logger::DumpMemory(const char* Channel, void* Address, int Size, int NegativeSize) {
}

