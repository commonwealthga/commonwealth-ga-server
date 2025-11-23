#pragma once

#include "src/pch.hpp"

class Logger {
public:
	static std::vector<std::string> EnabledChannels;
	static std::map<std::string, int> ChannelIndents;
	static void Log(const char* Channel, const char* Format, ...);
	static void DumpMemory(char* Channel, void* Address, int Size, int NegativeSize = 0);
	static inline const char* GetTime() {
		auto now = std::chrono::system_clock::now();
		std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

		std::tm tm;
#if defined(_WIN32) || defined(_WIN64)
		localtime_s(&tm, &now_time_t);  // Windows
#else
		localtime_r(&now_time_t, &tm);  // POSIX
#endif

		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

		return oss.str().c_str();
	}
};

