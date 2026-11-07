#include "src/GameServer/TgGame/TgPlayerController/ServerRequestBeaconNetworkHop/TgPlayerController__ServerRequestBeaconNetworkHop.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <map>

namespace {
	// TgPlayerController vtable and its one subclass both hold the stub at +0x4ac.
	void** const kVtableSlots[] = {
		reinterpret_cast<void**>(0x11695d18 + 0x4ac),
		reinterpret_cast<void**>(0x11878eb8 + 0x4ac),
	};
	void* const kStrippedStub = reinterpret_cast<void*>(0x10963360);

	// A hop must follow a Use on a Beacon Net Node volume (the zone map opens
	// from the node); without this a modified client could hop from anywhere.
	constexpr uint64_t kBeaconUseWindowMs = 120000;
	std::map<ATgPlayerController*, uint64_t> g_lastBeaconUseMs;
}

void TgPlayerController__ServerRequestBeaconNetworkHop::Install() {
	for (void** slot : kVtableSlots) {
		if (*slot != kStrippedStub) {
			Logger::Log("quest", "BeaconNetworkHop install SKIPPED at %p: slot holds %p, expected stub %p\n",
				(void*)slot, *slot, kStrippedStub);
			continue;
		}
		DWORD oldProtect = 0;
		VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
		*slot = reinterpret_cast<void*>(&Call);
		VirtualProtect(slot, sizeof(void*), oldProtect, &oldProtect);
		Logger::Log("quest", "BeaconNetworkHop vtable slot %p patched\n", (void*)slot);
	}
}

void TgPlayerController__ServerRequestBeaconNetworkHop::NoteBeaconUse(ATgPlayerController* Controller) {
	if (Controller) g_lastBeaconUseMs[Controller] = GetTickCount64();
}

// The original native moved the agent to the chosen node of the
// zone's TgStartpointPortalNetwork. Rebuilt here: the node must belong to a
// portal network, its AvailabilityQuestGroupId (e.g. 84 = quest 163 active or
// complete) must be met from ga_character_quests, and the agent must have used
// a beacon node within the last 2 minutes.
void __fastcall TgPlayerController__ServerRequestBeaconNetworkHop::Call(ATgPlayerController* Controller, void* edx, ATgStartPoint* pStartPoint) {
	ATgPawn_Character* Pawn = Controller ? (ATgPawn_Character*)Controller->Pawn : nullptr;
	if (!Pawn || Pawn->Health <= 0 || !pStartPoint || !pStartPoint->TgPortalNetwork) {
		Logger::Log("quest", "[BeaconHop] refused: pawn=%p sp=%p net=%p\n", (void*)Pawn, (void*)pStartPoint,
			pStartPoint ? (void*)pStartPoint->TgPortalNetwork : nullptr);
		return;
	}

	int64_t character_id = 0;
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.Pawn != Pawn || kv.second.bClosed) continue;
		if (kv.second.pPlayerInfo) character_id = kv.second.pPlayerInfo->selected_character_id;
		break;
	}

	auto used = g_lastBeaconUseMs.find(Controller);
	const bool nearBeacon = used != g_lastBeaconUseMs.end() && GetTickCount64() - used->second < kBeaconUseWindowMs;
	const int group = pStartPoint->AvailabilityQuestGroupId;
	const bool unlocked = group <= 0 || Database::QuestGroupSatisfied(character_id, group);
	if (character_id <= 0 || !nearBeacon || !unlocked) {
		Logger::Log("quest", "[BeaconHop] refused: char=%lld node=%d group=%d nearBeacon=%d unlocked=%d\n",
			(long long)character_id, pStartPoint->m_nMapObjectId, group, (int)nearBeacon, (int)unlocked);
		return;
	}

	FRotator rot = pStartPoint->Rotation;
	rot.Pitch = 0; rot.Roll = 0;
	const bool ok = Pawn->SetLocation(pStartPoint->Location);
	if (ok) Controller->ClientSetLocation(Pawn->Location, rot);
	Logger::Log("quest", "[BeaconHop] char=%lld -> node %d (group %d) ok=%d at %.0f %.0f %.0f\n",
		(long long)character_id, pStartPoint->m_nMapObjectId, group, (int)ok,
		Pawn->Location.X, Pawn->Location.Y, Pawn->Location.Z);
}
