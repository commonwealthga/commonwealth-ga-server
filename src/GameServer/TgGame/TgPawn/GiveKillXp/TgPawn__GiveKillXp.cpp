#include "src/GameServer/TgGame/TgPawn/GiveKillXp/TgPawn__GiveKillXp.hpp"
#include "src/GameServer/TgGame/TgPawn/KillCredit/KillCredit.hpp"
#include "src/GameServer/TgGame/TgPawn/LevelUpFeedback/LevelUpFeedback.hpp"
#include "src/GameServer/Combat/SendCombatMessage/SendCombatMessage.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Config/Config.hpp"
#include "src/IpcClient/IpcClient.hpp"

// Kill XP. Called on the dying pawn from the TgPawn.uc death path
//     if(killerTaskForce != none) { GiveKillXp(tf); SpawnLoot(tf); CheckKillQuestCredit(tf); }
// The native (0x109c0a60) does nothing useful on its own (retail XP lived on
// Hi-Rez's backend), so after it runs every connected player on the killer's task
// force gets the dead bot's xp_value / currency_value from asm_data_set_bots -
// team-wide, same rule as CheckKillQuestCredit. Loot is upstream's quest_kill_credit
// -> credit_loot_table. Player deaths never award anything.
void __fastcall TgPawn__GiveKillXp::Call(ATgPawn* Pawn, void* edx, ATgRepInfo_TaskForce* tf) {
	LogCallBegin();
	CallOriginal(Pawn, edx, tf);

	ATgRepInfo_Player* DeadPri = Pawn ? (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo : nullptr;
	const int botId = Pawn ? Pawn->r_nProfileId : 0;  // SpawnBotById stamps asm_data_set_bots.bot_id
	if (tf == nullptr || DeadPri == nullptr || DeadPri->bBot == 0 || botId <= 0) {
		LogCallEnd();
		return;
	}
	if (KillCredit::NoKillReward(Pawn)) {
		Logger::Log("loot", "[KillXp] bot=%d from a no_kill_reward factory - no xp\n", botId);
		LogCallEnd();
		return;
	}

	for (auto& kv : GClientConnectionsData) {
		if (kv.second.bClosed) continue;
		ATgPawn_Character* PlayerPawn = kv.second.Pawn;
		if (PlayerPawn == nullptr) continue;
		ATgRepInfo_Player* Pri = (ATgRepInfo_Player*)PlayerPawn->PlayerReplicationInfo;
		if (Pri == nullptr || Pri->r_TaskForce != tf) continue;
		if (!KillCredit::Eligible(Pawn, PlayerPawn)) continue;  // open zone: killer only

		const auto* info = kv.second.pPlayerInfo ? kv.second.pPlayerInfo : &kv.second.PlayerInfo;
		// Standalone mode: update the local DB and forward the event.
		if (Config::GetNodeMode())
			IpcClient::SendNodeClaim("kill", info->user_id, info->selected_character_id, "bot_id", botId);
		const auto r = Database::AwardKillXp(info->user_id, info->selected_character_id, botId);
		Logger::Log("loot", "[KillXp] bot=%d char=%lld -> %s xp+%d cr+%d (xp=%d cr=%d)\n",
			botId, (long long)info->selected_character_id, r.ok ? "OK" : "none",
			r.xp, r.credits, r.wallet.xp, r.wallet.credits);
		if (!r.ok) continue;
		// Replicated -> HUD XP bar / level and Agent Info credits update live.
		PlayerPawn->r_nXp       = r.wallet.xp;
		PlayerPawn->r_nCurrency = r.wallet.credits;
		// Kill float + Combat Log line, the client's own 0x9F rows: `+N XP(0)` / `+N CR`
		// rising from the dead bot (handler 0x10919020 picks the pawn by the victim id).
		const auto floatRow = [&](uint16_t type, int amount) {
			if (amount > 0)
				SendCombatMessage::CallRaw(PlayerPawn, type, (int16_t)Pri->PlayerID, 0, (int16_t)DeadPri->PlayerID,
					(int16_t)(amount > 32767 ? 32767 : amount), 0);
		};
		floatRow(0xCDB2, r.paid_xp);
		floatRow(0xD744, r.paid_credits);
		// Level-up mid-map: the PRI level is only set at spawn, so an agent who reached
		// level N inside a mission was still level N-1 for the client's equip-screen
		// level gate ("needs level 5" on a level-5 item) until the next map.
		const int level = Database::GetCharacterLevel(info->selected_character_id);
		if (level != Pri->r_nLevel) {
			Logger::Log("loot", "[KillXp] char=%lld level %d -> %d\n",
				(long long)info->selected_character_id, Pri->r_nLevel, level);
			const bool up = level > Pri->r_nLevel && Pri->r_nLevel > 0;
			Pri->r_nLevel = level;
			Pri->bNetDirty = 1;
			if (up) LevelUpFeedback::Announce(PlayerPawn, level);
		}
	}
	LogCallEnd();
}
