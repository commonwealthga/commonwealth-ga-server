#pragma once
// Player bug reports and player reports from chat (-bug, -report).
// The client's own /bug launches a local AutoReporter.exe that posts to a dead
// Hi-Rez endpoint, so reports are filed through chat instead. Rows land in
// ga_bug_reports; listing and status changes are left to external tools.
#include <cstdint>
#include <string>

namespace BugReports {

constexpr size_t kMaxTextBytes     = 500;
constexpr int    kMaxPerHour       = 5;
constexpr size_t kContextLines     = 50;
constexpr size_t kContextLineBytes = 300;

void EnsureSchema();

struct Result {
    int64_t     id = 0;      // > 0 on success
    std::string reply;       // System-channel line for the sender
};

// kind = "bug" or "player". target is the reported player's name ("" for bugs).
Result Submit(const std::string& kind, const std::string& session_guid,
              const std::string& agent_name, const std::string& target,
              const std::string& text);

} // namespace BugReports
