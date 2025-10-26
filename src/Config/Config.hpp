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
};

