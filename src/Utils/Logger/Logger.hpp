#pragma once

#include "src/pch.hpp"

class Logger {
public:
	static std::vector<std::string> EnabledChannels;
	// Channels in this list are recorded to an in-memory ring buffer instead of
	// (or in addition to, if also in EnabledChannels) being written to a file.
	// The ring is flushed to the crash log only if CrashHandler fires — zero
	// disk I/O during normal operation. Use this for high-volume channels like
	// "hook_calltree" that you want *only* when something crashes.
	static std::vector<std::string> EnabledCrashChannels;
	static std::map<std::string, int> ChannelIndents;
	// Directory written to by per-channel file logging. Files are named
	// "<LogDir>\<Channel>.txt". Default "C:" preserves the historical
	// "C:\<Channel>.txt" location inside the Wine prefix's drive_c. Set via
	// Config::GetLogDir() at DLL init; do NOT include a trailing slash.
	static std::string LogDir;
	static void Log(const char* Channel, const char* Format, ...);
	static void DumpMemory(const char* Channel, void* Address, int Size, int NegativeSize = 0);
	// Called by CrashHandler to append the ring buffer to the crash dump file.
	static void DumpCrashBuffer(void* fileHandle);
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

