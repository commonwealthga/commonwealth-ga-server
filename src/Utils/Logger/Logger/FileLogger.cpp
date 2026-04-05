#include "src/Utils/Logger/Logger.hpp"

std::map<std::string, int> Logger::ChannelIndents;
std::vector<std::string> Logger::EnabledChannels;
static CRITICAL_SECTION g_log_cs;
static bool g_log_cs_init = false;

void Logger::Log(const char* Channel, const char* Format, ...) {
	if (!g_log_cs_init) {
		InitializeCriticalSection(&g_log_cs);
		g_log_cs_init = true;
	}

	EnterCriticalSection(&g_log_cs);

	if (std::find(EnabledChannels.begin(), EnabledChannels.end(), Channel) == EnabledChannels.end()) {
		LeaveCriticalSection(&g_log_cs);
		return;
	}

	char filename[128];
	sprintf(filename, "C:\\%s.txt", Channel);

    FILE* fp = fopen(filename, "a");
    if (fp == NULL) {
		LeaveCriticalSection(&g_log_cs);
        return;
    }

	int indent = ChannelIndents[Channel];
	for (int i = 0; i < indent; i++) {
		fprintf(fp, "│  ");
	}

    va_list args;
    va_start(args, Format);

    vfprintf(fp, Format, args);      // Write formatted content

    va_end(args);
    fclose(fp);

	LeaveCriticalSection(&g_log_cs);
}

void Logger::DumpMemory(const char* Channel, void* Address, int Size, int NegativeSize) {
	if (std::find(EnabledChannels.begin(), EnabledChannels.end(), Channel) == EnabledChannels.end()) {
		return;
	}

	char* base = (char*)Address;

	Log(Channel, "[%p] Memory Dump:\n", Address);

	if (NegativeSize > 0) {
		for (int i = -NegativeSize * 4; i < 0; i += 4) {
			Log(Channel, "  0x%04X:   %08X\n", i, *(int*)(base + i));
		}
	}

	for (int i = 0; i < Size; i += 4) {
		Log(Channel, "  0x%04X:   %08X\n", i, *(int*)(base + i));
	}
}

