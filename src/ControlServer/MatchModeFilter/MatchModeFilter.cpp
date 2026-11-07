#include "src/ControlServer/MatchModeFilter/MatchModeFilter.hpp"

#include <mutex>
#include <unordered_map>

#include "src/ControlServer/MatchmakingService/Domain.hpp"
#include "src/ControlServer/TeamService/TeamService.hpp"
#include "src/ControlServer/Logger.hpp"

namespace {
std::mutex g_mutex;
std::unordered_map<std::string, uint32_t> g_masks;   // session guid -> excluded-mode bits of its last MATCH_JOIN
}

namespace {
std::unordered_map<std::string, bool> g_late;   // session guid -> ticked "Volunteer me for late backfills"
}

void MatchModeFilter::OnClientJoinFlags(const std::string& session_guid, uint32_t match_filters) {
    const bool vol = (match_filters & kLateBackfillBit) != 0;
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_late.find(session_guid);
    if (it == g_late.end() || it->second != vol)
        Logger::Log("matchmaking", "[Matchmaking] 0740 late backfills: %s volunteer=%d (filters 0x%X)\n",
                    session_guid.c_str(), vol ? 1 : 0, match_filters);
    g_late[session_guid] = vol;
}

bool MatchModeFilter::LateBackfillVolunteer(const std::string& session_guid) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_late.find(session_guid);
    return it == g_late.end() ? true : it->second;
}

void MatchModeFilter::OnJoin(const std::string& session_guid, uint32_t match_filters) {
    const uint32_t mask = match_filters & kModeMask;
    std::vector<std::string> guids = TeamService::GetTeamMemberGuids(session_guid);
    if (guids.empty() || !TeamService::IsLeader(session_guid)) guids = { session_guid };
    std::lock_guard<std::mutex> lock(g_mutex);
    for (const auto& g : guids) {
        if (mask) g_masks[g] = mask; else g_masks.erase(g);
    }
}

uint32_t MatchModeFilter::ModeBit(const std::string& game_mode) {
    const auto dot = game_mode.rfind('.');
    const std::string cls = dot == std::string::npos ? game_mode : game_mode.substr(dot + 1);
    if (cls == "TgGame_Ticket")        return 0x02;
    if (cls == "TgGame_Mission")       return 0x04;
    if (cls == "TgGame_DualCTF")       return 0x08;
    if (cls == "TgGame_Escort")        return 0x10;
    if (cls == "TgGame_PointRotation") return 0x20;
    return 0;
}

void MatchModeFilter::Narrow(std::vector<const MapModeEntry*>& base, const std::vector<std::string>* guids,
                             uint32_t queue_id) {
    if (!guids || guids->empty() || base.empty()) return;
    std::vector<uint32_t> masks;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        for (const auto& g : *guids) {
            auto it = g_masks.find(g);
            masks.push_back(it == g_masks.end() ? 0u : it->second);
        }
    }
    uint32_t any = 0;
    for (uint32_t m : masks) any |= m;
    if (!any) return;

    std::vector<size_t> allow(base.size(), 0);
    size_t best = 0;
    for (size_t i = 0; i < base.size(); ++i) {
        const uint32_t bit = ModeBit(base[i]->game_mode);
        for (uint32_t m : masks) if (!(m & bit)) ++allow[i];
        if (allow[i] > best) best = allow[i];
    }
    if (best == 0) return;
    std::vector<const MapModeEntry*> kept;
    for (size_t i = 0; i < base.size(); ++i) if (allow[i] == best) kept.push_back(base[i]);
    Logger::Log("queue-pop",
        "[MatchModeFilter] queue=%u players=%zu excluded=0x%02x kept=%zu/%zu (%s)\n",
        queue_id, masks.size(), any, kept.size(), base.size(),
        best == masks.size() ? "all players' game types" : "most players' game types");
    base.swap(kept);
}
