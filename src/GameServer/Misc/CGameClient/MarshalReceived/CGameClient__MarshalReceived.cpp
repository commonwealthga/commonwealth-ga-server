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
#include "src/GameServer/Engine/KismetDump/KismetDump.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"

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

		if (!session_guid.empty()) {
			nlohmann::json ev;
			ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
			ev["subtype"]      = "skill_save";
			ev["instance_id"]  = IpcClient::GetInstanceId();
			ev["session_guid"] = session_guid;
			ev["character_id"] = nCharacterId;
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
		// without waiting for a respawn. Only do this for a real save —
		// counted>0. An empty payload is a request, we don't want to wipe.
		if (counted > 0 && it != GClientConnectionsData.end()
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

		Logger::Log(GetLogChannel(), "[QUEST_UPDATE_REQUEST] questId=%u abandon=%d session_guid=%s\n",
			nQuestId, (int)bAbandonFlag, session_guid.c_str());

		if (!session_guid.empty()) {
			TArray<USequenceObject*> Events;
			TArray<int> Indices;
			AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);

			WorldInfo->GetGameSequence()->FindSeqObjectsByClass(
				ClassPreloader::GetClass("Class TgGame.TgSeqEvent_QuestUpdated"),
				1,
				&Events
			);

			for (int i = 0; i < Events.Num(); i++) {
				UTgSeqEvent_QuestUpdated* Event = (UTgSeqEvent_QuestUpdated*)Events.Data[i];
				if (Event->QuestId == nQuestId) {

					for (int j = 0; j < Event->OutputLinks.Num(); j++) {
						for (int k = 0; k < Event->OutputLinks.Data[j].Links.Num(); k++) {
							USequenceOp* Op = Event->OutputLinks.Data[j].Links.Data[k].LinkedOp;
							Logger::Log(GetLogChannel(), "OutputLink [%d] Link [%d] Sequence Op: %s\n", j, k, Op->GetFullName());
						}
					}

					// USeqAct_Interp::bClientSideOnly is at +0x017C bit 0x400.
					// The sequence tick silently skips client-side-only ops on a dedicated server.
					// Clear the flag on all ops connected to the output link we're about to fire.
					auto ClearClientSideOnly = [](UTgSeqEvent_QuestUpdated* Ev, int LinkIdx) {
						if (LinkIdx >= Ev->OutputLinks.Num()) return;
						auto& link = Ev->OutputLinks.Data[LinkIdx];
						for (int k = 0; k < link.Links.Num(); k++) {
							USequenceOp* Op = link.Links.Data[k].LinkedOp;
							if (Op) {
								uint32_t& flags = *(uint32_t*)((char*)Op + 0x017C);
								if (flags & 0x400u) {
									flags &= ~0x400u;
									Logger::Log(GetLogChannel(), "Cleared bClientSideOnly on %s\n", Op->GetFullName());
								}
							}
						}
					};

					// Re-dump Kismet once after sub-levels have loaded (first quest event fires well after BeginPlay)
					static bool s_bKismetRedumped = false;
					if (!s_bKismetRedumped) {
						s_bKismetRedumped = true;
						DumpKismetSequence();
					}

					auto status = Database::GetQuestStatus(nCharacterId, nQuestId);
					int outputLink = -1;
					if (status == "active" && action != QuestAction::Abandon) {
						// ClearClientSideOnly(Event, 2);
						Event->ActivateOutputLink(2);
						outputLink = 2;
						Logger::Log(GetLogChannel(), "Activating output link 2\n");
					} else if (action != QuestAction::Abandon) {
						// ClearClientSideOnly(Event, 0);
						Event->ActivateOutputLink(0);
						outputLink = 0;
						Logger::Log(GetLogChannel(), "Activating output link 0\n");
					}

					// ForceActivateInput on non-latent ops to ensure immediate execution.
					// Latent ops (bLatentExecution = USequenceOp+0x008C bit 0x2) like SeqAct_Delay
					// must NOT be force-activated: their outputs fire asynchronously without an
					// instigator pawn in variable links, causing a null-deref crash downstream.
					if (outputLink >= 0 && outputLink < Event->OutputLinks.Num()) {
						auto& olink = Event->OutputLinks.Data[outputLink];
						for (int k = 0; k < olink.Links.Num(); k++) {
							USequenceOp* Op = olink.Links.Data[k].LinkedOp;
							if (!Op) continue;
							bool bLatent = (*(uint32_t*)((char*)Op + 0x008C) & 0x2) != 0;
							// SeqAct_Interp is latent (derives from USeqAct_Latent) but safe to
							// force-activate — it just starts a matinee and doesn't fire async
							// outputs that need an instigator pawn in variable links.
							bool bIsInterp = Op->IsA(ClassPreloader::GetClass("Class Engine.SeqAct_Interp"));
							if (bLatent && !bIsInterp) {
								Logger::Log(GetLogChannel(), "Skipping latent op %s\n", Op->GetFullName());
								continue;
							}
							int inputIdx = olink.Links.Data[k].InputLinkIdx;
							Logger::Log(GetLogChannel(), "ForceActivateInput(%d) on %s\n", inputIdx, Op->GetFullName());
							Op->ForceActivateInput(inputIdx);
						}
					}
				}
			}

			{
				nlohmann::json ev;
				ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
				ev["subtype"]      = "quest";
				ev["instance_id"]  = IpcClient::GetInstanceId();
				ev["session_guid"] = session_guid;
				ev["quest_id"]     = (int)nQuestId;
				ev["abandon"]      = (action == QuestAction::Abandon);
				ev["character_id"] = nCharacterId;
				IpcClient::Send(ev.dump());
			}
		}
	}

	LogCallEnd();

	return 0;
}
