#include "src/GameServer/TgGame/TgPawn/CheckKillQuestCredit/TgPawn__CheckKillQuestCredit.hpp"

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <string>

// Reimplementation of TgPawn::CheckKillQuestCredit (0x109c0a80). The binary's
// copy is a STUB (Ghidra: TgPawn__CheckKillQuestCredit_notimplemented, empty
// body) — CallOriginal here would do nothing, which is why killing a quest
// target never advanced anything.
//
// Call site is TgPawn.uc:2940, from the death path:
//     if(killerTaskForce != none) { GiveKillXp(tf); SpawnLoot(tf); CheckKillQuestCredit(tf); }
// Note the argument is the killer's TASK FORCE, not the killer — retail credits
// kills team-wide (same default as TgSeqAct_QuestIncrementReqCount's
// bTeamwideCredit). So every connected player on that task force gets credit.
//
// `self` is the pawn that just died. SpawnBotById stamps the bot id onto
// ATgPawn::r_nProfileId, which is the key asm_data_set_quest_requirements
// matches on via target_bot_id (requirement_type_value_id 1428 = "Kill").
//
// Quest state lives on the control server (ga_character_quests +
// ga_character_quest_progress) and the progress packet goes out on that
// player's own TCP session, so the DLL's job ends at reporting the kill.
void __fastcall TgPawn__CheckKillQuestCredit::Call(ATgPawn* Pawn, void* edx, ATgRepInfo_TaskForce* tf) {
	if (Pawn == nullptr || tf == nullptr) return;

	// Player deaths must never award kill credit — only bot pawns carry a
	// meaningful r_nProfileId (bot id); on a player it's the loadout profile.
	ATgRepInfo_Player* DeadPri = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	if (DeadPri == nullptr || DeadPri->bBot == 0) return;

	const int botId = Pawn->r_nProfileId;
	if (botId <= 0) return;

	int credited = 0;
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.bClosed) continue;
		ATgPawn_Character* PlayerPawn = kv.second.Pawn;
		if (PlayerPawn == nullptr) continue;

		ATgRepInfo_Player* Pri = (ATgRepInfo_Player*)PlayerPawn->PlayerReplicationInfo;
		if (Pri == nullptr || Pri->r_TaskForce != tf) continue;

		// pPlayerInfo is the live PlayerRegistry entry; the inline copy can be
		// stale for a character selected after the connection was recorded.
		const int64_t characterId = kv.second.pPlayerInfo
			? kv.second.pPlayerInfo->selected_character_id
			: kv.second.PlayerInfo.selected_character_id;
		if (characterId == 0 || kv.second.SessionGuid.empty()) continue;

		nlohmann::json ev;
		ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]      = "quest_kill_credit";
		ev["instance_id"]  = IpcClient::GetInstanceId();
		ev["session_guid"] = kv.second.SessionGuid;
		ev["character_id"] = characterId;
		ev["bot_id"]       = botId;
		// Lets the control server push an inventory refresh after a loot grant.
		ev["pawn_id"]      = (int)PlayerPawn->r_nPawnId;
		IpcClient::Send(ev.dump());
		credited++;
	}

	Logger::Log("quest",
		"CheckKillQuestCredit: bot=%d died to taskforce=%p — credited %d player(s)\n",
		botId, tf, credited);
}
