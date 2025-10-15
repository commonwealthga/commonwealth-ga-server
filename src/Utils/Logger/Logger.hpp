#pragma once

#include "src/pch.hpp"

class Logger {
public:
	static void Log(const char* Channel, const char* Format, ...);
	static void DumpMemory(char* Channel, void* Address, int Size, int NegativeSize = 0);
	
};

