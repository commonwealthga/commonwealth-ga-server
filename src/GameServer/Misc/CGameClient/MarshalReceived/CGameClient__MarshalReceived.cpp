#include "src/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/GameServer/Constants/TcpFunctions.h"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/TgNetDrv/MarshalChannel/MarshalReceived/MarshalChannel__MarshalReceived.hpp"
#include "src/GameServer/Misc/CMarshal/GetArray/CMarshal__GetArray.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetFlag/CMarshal__GetFlag.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Database/Database.hpp"
#include "src/Shared/SkillRules.hpp"
#include "src/GameServer/Engine/KismetDump/KismetDump.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/Social/InspectRelay.hpp"

enum class QuestAction { Request, Abandon };

// Node-list walk: CMarshal::GetArray returns a struct whose head is at
// (arr + 4), next pointer at (row + 8). Matches the AsmDataCapture pattern.
template <class Fn>
static void WalkArrayNodes(uint32_t arrayPtr, Fn fn) {
	if (arrayPtr == 0) return;
	void* head = *(void**)((char*)arrayPtr + 4);
	for (void* row = head; row; row = *(void**)((char*)row + 8)) {
		fn(row);
	}
}

uint8_t __fastcall CGameClient__MarshalReceived::Call(void* GameClient, void* edx, void* InBunch) {

	LogCallBegin();

	short uVar1 = *(short*)((int)InBunch + 0x26);
	Logger::Log(GetLogChannel(), "Received packet type [0x%04X] (suppressed on server)\n", uVar1);

	if (InspectRelay::Handle(uVar1, InBunch, edx)) { LogCallEnd(); return 0; }   // X inspect

	if (uVar1 == GA_U::REQUEST_BILBOARD_CONTENT) {   // 0760: the client asks for a volume's text when the player enters it
		uint32_t nKey = 0;
		CMarshal__GetInt32t::CallOriginal(InBunch, edx, GA_T::BILBOARD_ID, &nKey);
		auto it = GClientConnectionsData.find((int32_t)(intptr_t)GCurrentMarshalConnection);
		if (it != GClientConnectionsData.end() && !it->second.SessionGuid.empty()) {
			nlohmann::json ev;
			ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
			ev["subtype"]      = "billboard_request";
			ev["instance_id"]  = IpcClient::GetInstanceId();
			ev["session_guid"] = it->second.SessionGuid;
			ev["key"]          = nKey;
			IpcClient::Send(ev.dump());
		}
		LogCallEnd();
		return 0;
	}

	if (uVar1 == GA_U::SEND_PLAYER_SKILLS) {
		// Client-sent skill-tree save. The client's SendRequest routes this
		// opcode (flag=0x3000 in the shipped per-opcode table) through the
		// game NetDriver's marshal channel — i.e. straight to our game
		// server — rather than the lobby TCP. TgMarshalChannel::
		// MarshalReceived fans it into this function. Parse the payload,
		// forward to the control server for DB persistence, and don't let
		// it fall through to the client-bound default (which would try to
		// update a client UI that doesn't exist on this process).
		std::string session_guid;
		int64_t nCharacterId = 0;
		int itemProfileId = -1;
		auto it = GClientConnectionsData.find((int32_t)(intptr_t)GCurrentMarshalConnection);
		if (it != GClientConnectionsData.end()) {
			session_guid = it->second.SessionGuid;
			if (it->second.pPlayerInfo) nCharacterId = it->second.pPlayerInfo->selected_character_id;
			if (it->second.Pawn) itemProfileId = ((ATgPawn_Character*)it->second.Pawn)->r_nItemProfileId;
		}
		Logger::Log("skills_debug",
			"[SEND_PLAYER_SKILLS inbound] pawn r_nItemProfileId=%d char=%lld\n",
			itemProfileId, (long long)nCharacterId);

		uint32_t arrayPtr = 0;
		int ok = CMarshal__GetArray::CallOriginal(InBunch, edx,
			GA_T::DATA_SET_CHARACTER_SKILLS, &arrayPtr);

		nlohmann::json rows = nlohmann::json::array();
		int counted = 0;
		if (ok && arrayPtr != 0) {
			WalkArrayNodes(arrayPtr, [&](void* row) {
				uint32_t groupId = 0, skillId = 0, points = 0;
				CMarshal__GetInt32t::CallOriginal(row, nullptr, GA_T::SKILL_GROUP_ID,   &groupId);
				CMarshal__GetInt32t::CallOriginal(row, nullptr, GA_T::SKILL_ID,         &skillId);
				CMarshal__GetInt32t::CallOriginal(row, nullptr, GA_T::POINTS_ALLOCATED, &points);
				if (groupId && skillId && points > 0) {
					nlohmann::json r;
					r["skill_group_id"] = (int)groupId;
					r["skill_id"]       = (int)skillId;
					r["points"]         = (int)points;
					rows.push_back(r);
					counted++;
				}
			});
		}

		Logger::Log(GetLogChannel(),
			"[SEND_PLAYER_SKILLS] parsed %d rows session_guid=%s char=%lld\n",
			counted, session_guid.c_str(), (long long)nCharacterId);

		// pawn_id: forwarded to the control server so the skill_save IPC
		// handler can push a SEND_INVENTORY refresh after persisting — the
		// client's per-device stat panels can then re-render against the
		// new skill build.
		int pawnId = 0;
		if (it != GClientConnectionsData.end() && it->second.Pawn) {
			pawnId = (int)it->second.Pawn->r_nPawnId;
		}

		if (!session_guid.empty()) {
			nlohmann::json ev;
			ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
			ev["subtype"]      = "skill_save";
			ev["instance_id"]  = IpcClient::GetInstanceId();
			ev["session_guid"] = session_guid;
			ev["character_id"] = nCharacterId;
			ev["pawn_id"]      = pawnId;
			// Only include item_profile_id if we actually read it from the
			// pawn (>=0). Sending -1 would clobber the control server's
			// cached value from the spawn IPC and break the echo PROFILE_ID.
			if (itemProfileId >= 0)
				ev["item_profile_id"] = itemProfileId;
			ev["skills"] = rows;
			IpcClient::Send(ev.dump());
		}

		// Respec: refresh this session's PlayerInfo.skills from the just-
		// received marshal, then re-run ReapplyCharacterSkillTree so the
		// new allocation takes effect (old deltas reversed + new applied)
		// without waiting for a respawn.
		//
		// We unconditionally honor an empty marshal as "player respec'd to
		// zero" — without this, a respec back to no allocation kept all the
		// previous build's modifiers/tickers/buff entries active until the
		// next spawn. The control server's skill_save IPC handler has its
		// own "empty payload = client requesting current state" guard for
		// DB writes, so an inadvertent empty here doesn't wipe persistence.
		// Apply only an allocation the game's rules allow (same
		// check the control server runs before saving); otherwise keep the
		// current build - the CS echoes the stored tree back to the UI.
		bool skillsValid = true;
		{
			std::vector<SkillRules::SkillPick> picks;
			for (const auto& r : rows)
				picks.push_back({r.value("skill_group_id", 0), r.value("skill_id", 0), r.value("points", 0)});
			std::string why;
			skillsValid = SkillRules::ValidateSkillAllocation(Database::GetConnection(), nCharacterId, picks, why);
			if (!skillsValid)
				Logger::Log(GetLogChannel(), "[SEND_PLAYER_SKILLS] REJECTED char=%lld: %s\n",
					(long long)nCharacterId, why.c_str());
		}
		if (skillsValid && it != GClientConnectionsData.end()
			&& it->second.pPlayerInfo && it->second.Pawn) {
			it->second.pPlayerInfo->skills.clear();
			for (const auto& r : rows) {
				SkillAllocation a;
				a.skill_group_id = r.value("skill_group_id", 0);
				a.skill_id       = r.value("skill_id",       0);
				a.points         = r.value("points",         0);
				if (a.skill_group_id && a.skill_id && a.points > 0)
					it->second.pPlayerInfo->skills.push_back(a);
			}
			Logger::Log(GetLogChannel(),
				"[SEND_PLAYER_SKILLS] respec → PlayerInfo.skills refreshed (%d rows), calling ReapplyCharacterSkillTree\n",
				(int)it->second.pPlayerInfo->skills.size());
			it->second.Pawn->ReapplyCharacterSkillTree();
		}

		LogCallEnd();
		return 0;
	}

	if (uVar1 == GA_U::QUEST_UPDATE_REQUEST) {
		uint32_t nQuestId = 0;

		CMarshal__GetInt32t::CallOriginal(InBunch, edx, GA_T::QUEST_ID, &nQuestId);

		unsigned char bAbandonFlag = 0;
		CMarshal__GetFlag::CallOriginal(InBunch, edx, GA_T::ABANDON_FLAG, &bAbandonFlag);

		QuestAction action = bAbandonFlag ? QuestAction::Abandon : QuestAction::Request;

		// A reward the player picked in the quest dialog (pick-one
		// reward lists). Field not confirmed yet - read every candidate, log
		// them, pass the first one set; absent fields stay 0.
		uint32_t nRewardItemId = 0, nLootTableItemId = 0, nItemId = 0;
		CMarshal__GetInt32t::CallOriginal(InBunch, edx, GA_T::REWARD_ITEM_ID, &nRewardItemId);
		CMarshal__GetInt32t::CallOriginal(InBunch, edx, GA_T::LOOT_TABLE_ITEM_ID, &nLootTableItemId);
		CMarshal__GetInt32t::CallOriginal(InBunch, edx, GA_T::ITEM_ID, &nItemId);
		const uint32_t nRewardPick = nRewardItemId ? nRewardItemId : (nLootTableItemId ? nLootTableItemId : nItemId);

		// GCurrentMarshalConnection is the UNetConnection set by MarshalChannel__MarshalReceived
		// before dispatching here — one per player, proper multi-player identity.
		std::string session_guid;
		ATgPawn_Character* InstigatorPawn = nullptr;
		int64_t nCharacterId = 0;
		auto it = GClientConnectionsData.find((int32_t)(intptr_t)GCurrentMarshalConnection);
		if (it != GClientConnectionsData.end()) {
			session_guid = it->second.SessionGuid;
			InstigatorPawn = it->second.Pawn;
			if (it->second.pPlayerInfo) nCharacterId = it->second.pPlayerInfo->selected_character_id;
		}

		Logger::Log(GetLogChannel(), "[QUEST_UPDATE_REQUEST] questId=%u abandon=%d reward_item=%u loot_table_item=%u item=%u session_guid=%s\n",
			nQuestId, (int)bAbandonFlag, nRewardItemId, nLootTableItemId, nItemId, session_guid.c_str());

		// Kismet (QuestUpdated links) fires on QUEST_KISMET from the
		// control server AFTER it accepted / refused / finished the request, so a
		// refused turn-in no longer plays the "Turned In" branch.
		if (!session_guid.empty()) {
			{
				nlohmann::json ev;
				ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
				ev["subtype"]      = "quest";
				ev["instance_id"]  = IpcClient::GetInstanceId();
				ev["session_guid"] = session_guid;
				ev["quest_id"]     = (int)nQuestId;
				ev["abandon"]      = (action == QuestAction::Abandon);
				ev["character_id"] = nCharacterId;
				ev["reward_item_id"] = (int)nRewardPick;
				// Lets the control server refresh the inventory after a turn-in
				// consumes components / grants reward loot.
				ev["pawn_id"]      = InstigatorPawn ? (int)InstigatorPawn->r_nPawnId : 0;
				IpcClient::Send(ev.dump());
			}
		}
	}

	LogCallEnd();

	return 0;
}

// Fire every op on one output link of a quest sequence event. Non-latent ops are
// force-activated so they run this tick. Latent ops (bLatentExecution =
// USequenceOp+0x008C bit 0x2) like SeqAct_Delay must NOT be force-activated: their
// outputs fire asynchronously without an instigator pawn in variable links, causing
// a null-deref crash downstream. SeqAct_Interp is latent but safe (it only starts a
// matinee); IsA is unreliable on this build, so match the class name.
static void ActivateSeqEventLink(USequenceEvent* Event, int outputLink, const char* channel) {
	if (outputLink < 0 || outputLink >= Event->OutputLinks.Num()) return;
	Event->ActivateOutputLink(outputLink);
	auto& olink = Event->OutputLinks.Data[outputLink];
	for (int k = 0; k < olink.Links.Num(); k++) {
		USequenceOp* Op = olink.Links.Data[k].LinkedOp;
		if (!Op) continue;
		bool bLatent = (*(uint32_t*)((char*)Op + 0x008C) & 0x2) != 0;
		bool bIsInterp = false;
		if (Op->Class) {
			const char* ocRaw = Op->Class->GetFullName();
			if (ocRaw) bIsInterp = std::string(ocRaw).find("SeqAct_Interp") != std::string::npos;
		}
		if (bLatent && !bIsInterp) {
			Logger::Log(channel, "Skipping latent op %s\n", Op->GetFullName());
			continue;
		}
		int inputIdx = olink.Links.Data[k].InputLinkIdx;
		Logger::Log(channel, "ForceActivateInput(%d) on %s\n", inputIdx, Op->GetFullName());
		Op->ForceActivateInput(inputIdx);
	}
}

void CGameClient__MarshalReceived::FireQuestKismet(int quest_id, int link, int requirement_id) {
	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (!WorldInfo || !WorldInfo->GetGameSequence()) return;

	// Re-dump Kismet once after sub-levels have loaded (first quest event fires well after BeginPlay)
	static bool s_bKismetRedumped = false;
	if (!s_bKismetRedumped) {
		s_bKismetRedumped = true;
		DumpKismetSequence();
	}

	TArray<USequenceObject*> Events;
	const bool bReq = requirement_id > 0;
	WorldInfo->GetGameSequence()->FindSeqObjectsByClass(
		ClassPreloader::GetClass(bReq ? "Class TgGame.TgSeqEvent_QuestRequirementUpdated"
		                              : "Class TgGame.TgSeqEvent_QuestUpdated"),
		1, &Events);

	int fired = 0;
	for (int i = 0; i < Events.Num(); i++) {
		if (bReq) {
			auto* Ev = (UTgSeqEvent_QuestRequirementUpdated*)Events.Data[i];
			if (Ev->QuestId != quest_id || Ev->QuestRequirementId != requirement_id) continue;
			ActivateSeqEventLink(Ev, 0, GetLogChannel());
		} else {
			auto* Ev = (UTgSeqEvent_QuestUpdated*)Events.Data[i];
			if (Ev->QuestId != quest_id) continue;
			ActivateSeqEventLink(Ev, link, GetLogChannel());
		}
		fired++;
	}
	if (fired)
		Logger::Log(GetLogChannel(), "[QuestKismet] quest=%d req=%d link=%d -> %d event(s)\n",
			quest_id, requirement_id, link, fired);
}
