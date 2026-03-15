#pragma once

#include "src/pch.hpp"

class Logger {
public:
	static std::vector<std::string> EnabledChannels;
	static std::map<std::string, int> ChannelIndents;
	static void Log(const char* Channel, const char* Format, ...);
	static void DumpMemory(const char* Channel, void* Address, int Size, int NegativeSize = 0);
	static inline const char* GetTime() {
		auto now = std::chrono::system_clock::now();
		std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

		std::tm tm;
#if defined(_WIN32) || defined(_WIN64)
		localtime_s(&tm, &now_time_t);
#else
		localtime_r(&now_time_t, &tm);
#endif

		static thread_local char buf[32];
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
		return buf;
	}
};

