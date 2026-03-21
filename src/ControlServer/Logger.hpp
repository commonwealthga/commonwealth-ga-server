#pragma once

#include <cstdio>
#include <cstdarg>
#include <chrono>
#include <ctime>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Portable, header-only Logger for the control server.
// Writes to stderr. Same API as the DLL Logger: Logger::Log(channel, fmt, ...)
// No pch.hpp, no windows.h.
//
// EnabledChannels: if empty, all channels are logged.
//                  if non-empty, only listed channels are logged.
// ---------------------------------------------------------------------------

class Logger {
public:
    static std::vector<std::string> EnabledChannels;

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

    static void Log(const char* Channel, const char* Format, ...) {
        // If EnabledChannels is non-empty, check if this channel is listed.
        if (!EnabledChannels.empty()) {
            bool found = false;
            for (const auto& ch : EnabledChannels) {
                if (ch == Channel) {
                    found = true;
                    break;
                }
            }
            if (!found) return;
        }

        char buf[4096];
        va_list args;
        va_start(args, Format);
        vsnprintf(buf, sizeof(buf), Format, args);
        va_end(args);

        fprintf(stderr, "[%s] [%s] %s", GetTime(), Channel, buf);
    }
};

// Static member definitions — included once per translation unit via this header.
// Use inline to avoid ODR violations across multiple TUs.
inline std::vector<std::string> Logger::EnabledChannels;
