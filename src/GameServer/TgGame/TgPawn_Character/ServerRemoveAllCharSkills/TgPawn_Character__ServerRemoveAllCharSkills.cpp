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
// Sequence:
//   1. clear the cached allocation on PlayerInfo (Reapply will read this)
//   2. RemoveCharacterSkillTree clears s_SkillBasedEffectGroups (event-driven
//      skill list)
//   3. ReapplyCharacterSkillTree runs the REVERSAL pass against the empty
//      allocation: subtracts every g_appliedSkillDeltas entry from the
//      pawn's s_Properties, removes every g_appliedSkillBuffs entry from
//      m_EffectBuffInfo (with bRemove=1 so ApplyBuff finds the same slot
//      and applies the inverse delta), and ProcessEffect(bRemove=1)'s every
//      m_bSkillEffect-tagged ticker clone in s_AppliedEffectGroups. Without
//      this step the pawn keeps every old skill modifier active even after
//      the trainer "refunded all points" — observed in playtest.
//   4. tell the control server to wipe ga_character_skills + push fresh
//      SEND_PLAYER_SKILLS + SEND_INVENTORY back to the client so the UI
//      refreshes its skill UI and per-device stat displays.
void __fastcall TgPawn_Character__ServerRemoveAllCharSkills::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();

	if (!Pawn) { LogCallEnd(); return; }

	auto it = GPawnSessions.find((ATgPawn*)Pawn);
	if (it == GPawnSessions.end()) {
		Logger::Log("skills", "[ServerRemoveAllCharSkills] pawn=%p has no session\n", Pawn);
		LogCallEnd();
		return;
	}

	PlayerInfo* info = PlayerRegistry::GetByGuidPtr(it->second);
	if (info) info->skills.clear();

	// Clear s_SkillBasedEffectGroups (event-driven skill list) FIRST so the
	// reversal pass below doesn't see any stale templates.
	Pawn->RemoveCharacterSkillTree();

	// Reapply with the now-empty info->skills runs only the REVERSAL pass —
	// no new applies — which is exactly the "refund everything" semantic.
	// Without this call the previous build's deltas/buff entries/ticker
	// clones stay active until the next spawn.
	Pawn->ReapplyCharacterSkillTree();

	nlohmann::json ev;
	ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
	ev["subtype"]      = "skill_respec";
	ev["instance_id"]  = IpcClient::GetInstanceId();
	ev["session_guid"] = it->second;
	ev["character_id"] = info ? info->selected_character_id : 0;
	// pawn_id lets the control server's skill_respec handler also push a
	// SEND_INVENTORY refresh so the client's per-device stat displays
	// re-render against the new skill build.
	ev["pawn_id"]      = (int)Pawn->r_nPawnId;
	IpcClient::Send(ev.dump());

	Logger::Log("skills", "[ServerRemoveAllCharSkills] respec requested for guid=%s pawnId=%d\n",
		it->second.c_str(), (int)Pawn->r_nPawnId);

	LogCallEnd();
}
