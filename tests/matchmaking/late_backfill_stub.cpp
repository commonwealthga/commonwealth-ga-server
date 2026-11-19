// Test stub for the late-backfill flag (the real MatchModeFilter.cpp pulls in TeamService/Logger).
#include "src/ControlServer/MatchModeFilter/MatchModeFilter.hpp"
#include <unordered_map>
namespace { std::unordered_map<std::string, bool> g_late; }
void MatchModeFilter::OnClientJoinFlags(const std::string& g, uint32_t f) { g_late[g] = (f & kLateBackfillBit) != 0; }
bool MatchModeFilter::LateBackfillVolunteer(const std::string& g) { auto it = g_late.find(g); return it == g_late.end() || it->second; }
