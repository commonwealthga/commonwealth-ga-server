#include "src/GameServer/TgGame/TgChestActor/OpenChest/TgChestActor__OpenChest.hpp"

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/WorldGroups/WorldGroups.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <random>

namespace {
constexpr uintptr_t kVtable = 0x116bc6b8;
void** const kOpenSlot    = reinterpret_cast<void**>(kVtable + 0x37c);
void** const kRespawnSlot = reinterpret_cast<void**>(kVtable + 0x378);
void*  const kOpenStub    = reinterpret_cast<void*>(0x109af320);
void*  const kRespawnStub = reinterpret_cast<void*>(0x109af330);
// Intact SpawnChest (vtable +0x374): load mesh if needed, ShowMesh event.
using SpawnFn = void(__fastcall*)(ATgChestActor*, void*);
SpawnFn const kSpawnChest = reinterpret_cast<SpawnFn>(0x109b2de0);

constexpr unsigned char kChestClosed = 0;
constexpr unsigned char kChestOpen   = 1;

bool PatchSlot(void** slot, void* expected, void* fn, const char* what) {
	if (*slot != expected) {
		Logger::Log("loot", "TgChestActor %s install SKIPPED: slot holds %p, expected stub %p\n",
			what, *slot, expected);
		return false;
	}
	DWORD oldProtect = 0;
	VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	*slot = fn;
	VirtualProtect(slot, sizeof(void*), oldProtect, &oldProtect);
	return true;
}

float RandomSeconds(int lo, int hi) {
	static std::mt19937 gen(std::random_device{}());
	if (hi < lo) hi = lo;
	return (float)std::uniform_int_distribution<int>(lo, hi)(gen);
}
}

void TgChestActor__OpenChest::Install() {
	const bool a = PatchSlot(kOpenSlot, kOpenStub, reinterpret_cast<void*>(&Open), "OpenChest");
	const bool b = PatchSlot(kRespawnSlot, kRespawnStub, reinterpret_cast<void*>(&Respawn), "RespawnChest");
	Logger::Log("loot", "TgChestActor vtable slots patched: open=%d respawn=%d\n", a, b);
}

void __fastcall TgChestActor__OpenChest::Respawn(ATgChestActor* Chest, void* edx) {
	if (Chest == nullptr) return;
	Chest->r_eChestState = kChestClosed;
	Chest->bNetDirty = 1;
	kSpawnChest(Chest, nullptr);
}

void __fastcall TgChestActor__OpenChest::Open(ATgChestActor* Chest, void* edx, ATgPawn* aPawn) {
	if (Chest == nullptr || aPawn == nullptr) return;
	if (WorldGroups::OnChestUse(Chest, aPawn)) return;  // placed chest: use reported only
	if (Chest->r_eChestState != kChestClosed) return;  // one opener per spawn

	Chest->r_eChestState = kChestOpen;
	Chest->bNetDirty = 1;
	Chest->m_LastUsePawn = aPawn;
	Chest->DisplayUseMessage(aPawn, false);
	if (aPawn->m_CurrentUseActor == Chest) aPawn->m_CurrentUseActor = nullptr;

	// Close (hide) after m_nTimeTakeToClose, come back after Min..Max seconds.
	const int closeAfter = Chest->m_nTimeTakeToClose > 0 ? Chest->m_nTimeTakeToClose : 1;
	Chest->SetTimer((float)closeAfter, false, FName("CloseChest"), nullptr);
	if (Chest->m_MinRespawnTime > 0) {
		Chest->SetTimer((float)closeAfter + RandomSeconds(Chest->m_MinRespawnTime, Chest->m_MaxRespawnTime),
			false, FName("RespawnChest"), nullptr);
	}

	// Loot goes to the opener only; the control server rolls the table.
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.bClosed || (ATgPawn*)kv.second.Pawn != aPawn) continue;
		const int64_t characterId = kv.second.pPlayerInfo
			? kv.second.pPlayerInfo->selected_character_id
			: kv.second.PlayerInfo.selected_character_id;
		if (characterId == 0 || kv.second.SessionGuid.empty()) break;

		nlohmann::json ev;
		ev["type"]          = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]       = "chest_loot";
		ev["instance_id"]   = IpcClient::GetInstanceId();
		ev["session_guid"]  = kv.second.SessionGuid;
		ev["character_id"]  = characterId;
		ev["loot_table_id"] = Chest->m_nLootTableId;
		ev["pawn_id"]       = (int)aPawn->r_nPawnId;
		IpcClient::Send(ev.dump());
		Logger::Log("loot", "OpenChest: table=%d char=%lld\n", Chest->m_nLootTableId, (long long)characterId);
		break;
	}
}
