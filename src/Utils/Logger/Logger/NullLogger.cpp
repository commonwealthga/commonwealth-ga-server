#include "src/Utils/Logger/Logger.hpp"

std::string Logger::LogDir = "C:";

bool Logger::IsChannelEnabled(const char* /*channel*/) {
	// NullLogger never emits anything — always tell callers the channel is
	// off so they skip building expensive log args.
	return false;
}

void Logger::Log(const char* /*channel*/, const char* /*format*/, ...) {
}

void Logger::IndentChannel(const char* /*channel*/, int /*delta*/) {
}

void Logger::EnableChannel(const std::string& /*name*/) {
}

void Logger::EnableCrashChannel(const std::string& /*name*/) {
}

void Logger::DumpMemory(const char* /*channel*/, void* /*address*/, int /*size*/, int /*negativeSize*/) {
}

void Logger::DumpCrashBuffer(void* /*fileHandle*/) {
}

void Logger::EnsureLogDirExists() {
}

void Logger::ClearEnabledChannelFiles() {
}
