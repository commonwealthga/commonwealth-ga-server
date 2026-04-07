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
};

