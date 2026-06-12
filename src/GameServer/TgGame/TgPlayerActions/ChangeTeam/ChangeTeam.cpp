#include "src/GameServer/TgGame/TgPlayerActions/ChangeTeam/ChangeTeam.hpp"

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/GameServer/TgGame/TgPawn/SetTaskForceNumber/TgPawn__SetTaskForceNumber.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TgPlayerActions::ChangeTeamCmd {

namespace {

// Walk GClientConnectionsData looking for a matching SessionGuid.
// Returns the connected pawn or nullptr.
ATgPawn_Character* FindPawnBySessionGuid(const std::string& guid) {
    for (auto& kv : GClientConnectionsData) {
        if (kv.second.SessionGuid == guid) {
            return kv.second.Pawn;
        }
    }
    return nullptr;
}

const char* TargetName(Target t) {
    switch (t) {
        case Target::Toggle:    return "toggle";
        case Target::Attackers: return "attackers";
        case Target::Defenders: return "defenders";
    }
    return "?";
}

void Audit(const std::string& guid,
           const std::string& outcome, const std::string& details) {
    IpcClient::SendChatCommandAudit(guid, "-changeteam", outcome, details);
}

// Determine the current team number (1 = Attackers, 2 = Defenders).
// Reads the pawn's PlayerReplicationInfo->r_TaskForce and compares against
// the global team rep-info pointers in GTeamsData.
//
// Returns 0 if it cannot determine the team (PRI null, r_TaskForce null,
// GTeamsData unpopulated).
int ResolveCurrentTeamNumber(ATgPawn_Character* Pawn) {
    if (!Pawn || !Pawn->PlayerReplicationInfo) return 0;
    // Same cast pattern as TgPawn__SetTaskForceNumber.cpp:6 — the engine
    // PlayerReplicationInfo* on a TgPawn is always an ATgRepInfo_Player.
    ATgRepInfo_Player* repinfo = reinterpret_cast<ATgRepInfo_Player*>(Pawn->PlayerReplicationInfo);
    auto* tf = repinfo->r_TaskForce;
    if (!tf) return 0;
    if (tf == GTeamsData.Attackers) return 1;
    if (tf == GTeamsData.Defenders) return 2;
    return 0;
}

} // namespace

void Execute(const std::string& session_guid, Target target) {
    ATgPawn_Character* Pawn = FindPawnBySessionGuid(session_guid);
    if (!Pawn) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /changeteam: no pawn for guid=%s; dropping\n",
            session_guid.c_str());
        Audit(session_guid, "ignored", "no player pawn");
        return;
    }

    int current_team = ResolveCurrentTeamNumber(Pawn);
    if (current_team == 0) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /changeteam guid=%s: cannot resolve current team; dropping\n",
            session_guid.c_str());
        Audit(session_guid, "ignored", "cannot resolve current team");
        return;
    }

    int new_team = 0;
    switch (target) {
        case Target::Toggle:    new_team = (current_team == 1) ? 2 : 1; break;
        case Target::Attackers: new_team = 1; break;
        case Target::Defenders: new_team = 2; break;
    }

    if (new_team == current_team) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /changeteam guid=%s target=%s: already on team %d; no-op\n",
            session_guid.c_str(), TargetName(target), current_team);
        Audit(session_guid, "no-op",
            std::string("already on team ") + std::to_string(current_team)
            + " target=" + TargetName(target));
        return;
    }

    Logger::Log("chat-command",
        "[ChatCmd][DLL] /changeteam guid=%s target=%s: flipping %d -> %d\n",
        session_guid.c_str(), TargetName(target), current_team, new_team);

    // Bank the old-team stint + emit TEAM_CHANGE before the flip.
    MatchStats::OnTeamChanged((ATgPawn*)Pawn, new_team);

    // Flip the team. SetTaskForceNumber writes both r_TaskForce and Team on the
    // PRI, calls repinfo->SetTeam(taskforce), and fires Pawn->NotifyTeamChanged.
    // 1 = Attackers, anything else = Defenders (per
    // TgPawn__SetTaskForceNumber.cpp:5-18).
    TgPawn__SetTaskForceNumber::Call(Pawn, nullptr, new_team);

    // Administrative respawn — must not count as a death (design 2026-06-12).
    MatchStats::SuppressNextDeath(((ATgPawn*)Pawn)->r_nPawnId);

    // Suicide -> normal death state -> respawn timer -> respawn pulls the
    // freshly-flipped team from the PRI.
    Pawn->eventSuicide();

    Logger::Log("chat-command",
        "[ChatCmd][DLL] /changeteam guid=%s: SetTaskForceNumber + eventSuicide dispatched\n",
        session_guid.c_str());
    Audit(session_guid, "activated",
        "changed team " + std::to_string(current_team) + "->" + std::to_string(new_team));
}

} // namespace TgPlayerActions::ChangeTeamCmd
