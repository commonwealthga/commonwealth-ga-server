#pragma once
// Date windows for content that is only on part of the year.
// Config "calendar": { "<key>": {"start": "MM-DD", "end": "MM-DD", "force": "on"|"off"} }
// (UTC, end day inclusive, a window may wrap over New Year; keys starting
// with "_" are notes). Map pool entries with event_key = <key> are only
// picked while the window is open (see MatchmakingService), and items whose
// name starts with "Seasonal" only drop while any window is open.
#include <cstdio>
#include <ctime>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace EventCalendar {

struct Window {
    int start_mmdd = 0;   // e.g. 1017
    int end_mmdd   = 0;   // inclusive
    int force      = 0;   // 1 = always on, -1 = always off, 0 = by date
    int win_loot_table = 0; // pick-one loot table rolled once per mission win while open (0 = none)
};

inline std::mutex& Mu() { static std::mutex m; return m; }
inline std::map<std::string, Window>& Windows() { static std::map<std::string, Window> w; return w; }

// "MM-DD" -> MMDD, 0 when malformed.
inline int ParseMmdd(const std::string& s) {
    int m = 0, d = 0;
    if (sscanf(s.c_str(), "%d-%d", &m, &d) != 2 || m < 1 || m > 12 || d < 1 || d > 31) return 0;
    return m * 100 + d;
}

inline void Set(const std::map<std::string, Window>& w) {
    std::lock_guard<std::mutex> lk(Mu());
    Windows() = w;
}

inline int TodayMmdd(time_t now) {
    struct tm t{};
    gmtime_r(&now, &t);
    return (t.tm_mon + 1) * 100 + t.tm_mday;
}

inline bool WindowOpen(const Window& w, int mmdd) {
    if (w.force > 0) return true;
    if (w.force < 0 || !w.start_mmdd || !w.end_mmdd) return false;
    if (w.start_mmdd <= w.end_mmdd) return mmdd >= w.start_mmdd && mmdd <= w.end_mmdd;
    return mmdd >= w.start_mmdd || mmdd <= w.end_mmdd;  // wraps over New Year
}

inline bool IsActive(const std::string& key, time_t now = time(nullptr)) {
    std::lock_guard<std::mutex> lk(Mu());
    auto it = Windows().find(key);
    return it != Windows().end() && WindowOpen(it->second, TodayMmdd(now));
}

inline bool AnyActive(time_t now = time(nullptr)) {
    std::lock_guard<std::mutex> lk(Mu());
    const int mmdd = TodayMmdd(now);
    for (const auto& kv : Windows()) if (WindowOpen(kv.second, mmdd)) return true;
    return false;
}

inline std::vector<std::string> ActiveKeys(time_t now = time(nullptr)) {
    std::lock_guard<std::mutex> lk(Mu());
    const int mmdd = TodayMmdd(now);
    std::vector<std::string> out;
    for (const auto& kv : Windows()) if (WindowOpen(kv.second, mmdd)) out.push_back(kv.first);
    return out;
}

// Win-reward tables of every open window (config "win_loot_table").
inline std::vector<int> ActiveWinTables(time_t now = time(nullptr)) {
    std::lock_guard<std::mutex> lk(Mu());
    const int mmdd = TodayMmdd(now);
    std::vector<int> out;
    for (const auto& kv : Windows())
        if (kv.second.win_loot_table > 0 && WindowOpen(kv.second, mmdd)) out.push_back(kv.second.win_loot_table);
    return out;
}

}  // namespace EventCalendar
