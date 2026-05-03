#pragma once

#include <string>

class CrashHandler {
public:
    // crashDir is the directory where crash dumps are written (created on demand).
    // Pass via Config::GetCrashDir(); empty string falls back to a built-in default.
    static void Init(const std::string& crashDir);
};
