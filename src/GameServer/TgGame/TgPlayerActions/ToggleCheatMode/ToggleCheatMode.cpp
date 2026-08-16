#include "src/GameServer/TgGame/TgPlayerActions/ToggleCheatMode/ToggleCheatMode.hpp"

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TgPlayerActions::ToggleCheatCmd {

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

bool IsPlayerController(AController* c) {
    if (!c || !c->Class) return false;
    const char* full = c->Class->GetFullName();
    return full && std::strstr(full, "PlayerController");
}

void Audit(const std::string& guid,
           const std::string& outcome, const std::string& details) {
    IpcClient::SendChatCommandAudit(guid, "-changeteam", outcome, details);
}

}   // namespace

void Execute(const std::string& session_guid, int cheat_mode) {
    ATgPawn_Character* pawn = FindPawnBySessionGuid(session_guid);
    if (!pawn) {
        Logger::Log("chat-command",
            "[ChatCmd][ToggleCheatMode] guid=%s: no pawn found\n", session_guid.c_str());
        Audit(session_guid, "ignored", "no player pawn");
        return;
    }
    if (!IsPlayerController(pawn->Controller)) {
        Logger::Log("chat-command",
            "[ChatCmd][ToggleCheatMode] guid=%s: session controller isn't a PlayerController\n",
            session_guid.c_str());
        Audit(session_guid, "ignored", "session controller is not PlayerController");
        return;
    }
    ATgPlayerController* PlayerController=reinterpret_cast<ATgPlayerController*> (pawn->Controller);
    switch (cheat_mode) {
        case 1: PlayerController->Zeus ();   break;
        case 2: PlayerController->Icarus (); break;
        case 3: PlayerController->Hades ();  break;
        case 4: PlayerController->Apollo (); break;
        case 5: PlayerController->Athena (); break;
        default:
            Logger::Log("chat-command",
                "[ChatCmd][ToggleCheatMode] guid=%s: invalid parameter '%d'\n",
                session_guid.c_str(),cheat_mode);
            Audit(session_guid, "ignored", "invalid parameter");
            return;
    }

    Logger::Log("chat-command",
        "[ChatCmd][ToggleCheatMode] guid=%s: applied (mode=%d)\n",
        session_guid.c_str(), cheat_mode);
    Audit(session_guid, "activated","cheat "+std::to_string (cheat_mode));
}

} // namespace TgPlayerActions::ToggleCheatCmd
