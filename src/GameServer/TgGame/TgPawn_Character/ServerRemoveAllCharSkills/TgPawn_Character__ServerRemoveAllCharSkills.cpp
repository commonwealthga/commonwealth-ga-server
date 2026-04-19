#include "src/GameServer/TgGame/TgPawn_Character/ServerRemoveAllCharSkills/TgPawn_Character__ServerRemoveAllCharSkills.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

// UTgPawn_Character::ServerRemoveAllCharSkills — reliable server RPC,
// original `_notimplemented` at 0x109c0de0. Fires from UC respec flows to
// refund every allocated point.
//
// Three things happen:
//   1. strip all active skill effect groups from the pawn (RemoveCharacterSkillTree)
//   2. tell the control server to wipe ga_character_skills for this character
//   3. clear the cached allocation on PlayerInfo so a later Reapply is a no-op
//
// Step 2 doubles as a broadcast: the control server can also push an updated
// SEND_PLAYER_SKILLS (0xA0) so the client UI reflects the zeroed state.
void __fastcall TgPawn_Character__ServerRemoveAllCharSkills::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();

	if (!Pawn) { LogCallEnd(); return; }

	// Clear the on-pawn effect list immediately — this is authoritative for
	// runtime gameplay even before the control-server round-trip completes.
	Pawn->RemoveCharacterSkillTree();

	auto it = GPawnSessions.find((ATgPawn*)Pawn);
	if (it == GPawnSessions.end()) {
		Logger::Log("skills", "[ServerRemoveAllCharSkills] pawn=%p has no session\n", Pawn);
		LogCallEnd();
		return;
	}

	PlayerInfo* info = PlayerRegistry::GetByGuidPtr(it->second);
	if (info) info->skills.clear();

	nlohmann::json ev;
	ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
	ev["subtype"]      = "skill_respec";
	ev["instance_id"]  = IpcClient::GetInstanceId();
	ev["session_guid"] = it->second;
	ev["character_id"] = info ? info->selected_character_id : 0;
	IpcClient::Send(ev.dump());

	Logger::Log("skills", "[ServerRemoveAllCharSkills] respec requested for guid=%s\n",
		it->second.c_str());

	LogCallEnd();
}
