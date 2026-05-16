#pragma once

#include "src/pch.hpp"

class Config {
public:
	static std::string GetIpChar();
	static int GetPort();
	static std::wstring GetMapUrl();
	static std::string GetMapNameChar();
	static std::wstring GetMapParams();
	static std::string GetMapParamsChar();
	static int GetDifficultyValueId();
	static uint16_t GetIpcPort();
	static std::string GetIpcHost();
	static int64_t GetInstanceId();
	static std::string GetGamePath();
	static bool GetFixPackageGuids();
	static std::string GetCrashDir();
	static std::string GetLogDir();
	static std::vector<std::string> GetEnabledChannels();
	static std::vector<std::string> GetEnabledCrashChannels();
	// When true, truncate every file under <LogDir>/<channel>.txt for each
	// channel in EnabledChannels at DLL load. Convenience for repeated tests
	// where stale entries from prior runs hide the new ones we care about.
	// Switch: -clearlogs=1. Default: false.
	static bool GetClearLogs();
	// When true, World::BeginPlay walks the global object array and writes a
	// human-readable dump of every map-placed actor we care about (player
	// starts, bot starts, factories, mission objectives, beacon factories)
	// to the "mapdump" log channel. One-shot data-mining aid; fire the
	// server manually without the control server to drive it.
	// Switch: -dumpmapdata=1. Default: false.
	static bool GetDumpMapData();
};

