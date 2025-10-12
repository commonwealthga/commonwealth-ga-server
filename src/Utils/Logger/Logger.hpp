#pragma once

#include "src/pch.hpp"

class Logger {
public:
	static void Log(const char* Channel, const char* Format, ...);
};

