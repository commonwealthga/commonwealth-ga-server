#include "src/GameServer/TgGame/TgPlayerController/ServerActivateInvItem/TgPlayerController__ServerActivateInvItem.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Database/Database.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include <cmath>
#include <unordered_map>

// Vanity pets (item type 1535; client IsUsableType 0x10a12860 = 1535|1634).
// The exe's TgPawn_Character::SpawnVanityPet/DismissVanityPet are empty stubs (server code
// stripped), so bag Activate summons the pet here: the item's bot (asm items.ref_bot_id,
// pawn TgPawn_VanityPet + TgAIController, behavior 760 _MASTER_Vanity_Pet_AI) is spawned
// owned by the agent. Activate again (same pet) dismisses it; another pet replaces it.
// The active pet is saved (ga_characters.active_pet_inv_id) and resummoned at every
// spawn (map change, respawn) by SummonSavedVanityPet; s_PetByChar remembers the
// pet actor per character and is only trusted when the actor is still in PawnList.
static std::unordered_map<int64_t, ATgPawn_VanityPet*> s_PetByChar;

static ATgPawn_VanityPet* FindLivePet(ATgPawn_Character* Pawn, int64_t character_id) {
	auto it = s_PetByChar.find(character_id);
	if (it == s_PetByChar.end() || !it->second) return nullptr;
	AWorldInfo* wi = Pawn ? Pawn->WorldInfo : nullptr;
	for (APawn* p = wi ? wi->PawnList : nullptr; p != nullptr; p = p->NextPawn) {
		if (p == (APawn*)it->second) return p->bDeleteMe ? nullptr : it->second;
	}
	s_PetByChar.erase(it);
	return nullptr;
}

static void DismissPet(ATgPawn_Character* Pawn, int64_t character_id) {
	ATgPawn_VanityPet* pet = FindLivePet(Pawn, character_id);
	s_PetByChar.erase(character_id);
	Pawn->r_CurrentVanityPet = nullptr;
	Pawn->bNetDirty = 1;
	if (!pet) return;
	AController* c = pet->Controller;
	((AActor*)pet)->Destroy();
	if (c && !c->bDeleteMe) ((AActor*)c)->Destroy();
}

static bool SpawnPet(ATgPawn_Character* Pawn, int64_t character_id, int bot_id, int item_id) {
	// a little behind and to the side of the agent
	const float yaw = Pawn->Rotation.Yaw * 3.14159265f / 32768.0f;
	FVector loc = Pawn->Location;
	loc.X += -cosf(yaw) * 90.0f - sinf(yaw) * 60.0f;
	loc.Y += -sinf(yaw) * 90.0f + cosf(yaw) * 60.0f;
	FRotator rot = Pawn->Rotation;
	rot.Pitch = 0; rot.Roll = 0;
	ATgGame* game = (ATgGame*)Globals::Get().GGameInfo;
	TgGame__SpawnBotById::s_bSpawningVanityPet = true;
	ATgPawn* pet = game ? TgGame__SpawnBotById::Call(game, nullptr, bot_id, loc, rot, false, nullptr, true,
		(ATgPawn*)Pawn, false, nullptr, 0.0f) : nullptr;
	TgGame__SpawnBotById::s_bSpawningVanityPet = false;
	if (pet && !ObjectClassCache::ClassNameContains((UObject*)pet, "TgPawn_VanityPet")) {
		((AActor*)pet)->Destroy();  // never expected (SQL filters pawn class 6909)
		pet = nullptr;
	}
	if (pet) {
		pet->Instigator = (APawn*)Pawn;
		((ATgPawn_VanityPet*)pet)->r_nSpawningItemId = item_id;
		pet->bNetDirty = 1;
		Pawn->r_CurrentVanityPet = (ATgPawn_VanityPet*)pet;
		Pawn->bNetDirty = 1;
		s_PetByChar[character_id] = (ATgPawn_VanityPet*)pet;
	}
	Logger::Log("vendor", "[Pet] summon char=%lld item=%d bot=%d -> %s class=%s\n", (long long)character_id,
		item_id, bot_id, pet ? "OK" : "FAILED", pet && pet->Class ? pet->Class->GetFullName() : "-");
	return pet != nullptr;
}

static bool ToggleVanityPet(ATgPawn_Character* Pawn, int64_t character_id, int nInvId) {
	int item_id = 0;
	const int bot_id = Database::GetVanityPetBotId(character_id, nInvId, &item_id);
	if (bot_id <= 0) return false;
	ATgPawn_VanityPet* cur = FindLivePet(Pawn, character_id);
	const bool same = cur && cur->r_nSpawningItemId == item_id;
	DismissPet(Pawn, character_id);
	if (same) {
		Database::SetActivePetInvId(character_id, 0);
		Logger::Log("vendor", "[Pet] dismiss char=%lld item=%d\n", (long long)character_id, item_id);
		return true;
	}
	Database::SetActivePetInvId(character_id, SpawnPet(Pawn, character_id, bot_id, item_id) ? nInvId : 0);
	return true;
}

void TgPlayerController__ServerActivateInvItem::SummonSavedVanityPet(ATgPawn_Character* Pawn) {
	if (!Pawn || Pawn->s_nCharacterId <= 0) return;
	const int64_t character_id = Pawn->s_nCharacterId;
	const int64_t inv = Database::GetActivePetInvId(character_id);
	if (inv <= 0) return;
	int item_id = 0;
	const int bot_id = Database::GetVanityPetBotId(character_id, inv, &item_id);
	if (bot_id <= 0) {  // pet item gone (sold / deleted)
		Database::SetActivePetInvId(character_id, 0);
		return;
	}
	DismissPet(Pawn, character_id);  // pet of the previous pawn (respawn in the same map)
	SpawnPet(Pawn, character_id, bot_id, item_id);
}

// Bag 'Activate'. The original native asked Hi-Rez's backend;
// the control server owns the bag, so the request goes there (GAME_EVENT
// activate_item, Database::ActivateItem) and is checked against the game's tables.
void __fastcall TgPlayerController__ServerActivateInvItem::Call(ATgPlayerController* Controller, void* edx, int nInvId) {
	LogCallBegin();
	ATgPawn_Character* Pawn = Controller ? (ATgPawn_Character*)Controller->Pawn : nullptr;
	std::string session_guid;
	int64_t character_id = 0;
	for (auto& kv : GClientConnectionsData) {
		if (!Pawn || kv.second.Pawn != Pawn) continue;
		session_guid = kv.second.SessionGuid;
		if (kv.second.pPlayerInfo) character_id = kv.second.pPlayerInfo->selected_character_id;
		break;
	}
	Logger::Log("vendor", "[Bag] activate_item char=%lld inv=%d\n", (long long)character_id, nInvId);
	if (Pawn && character_id > 0 && nInvId > 0 && ToggleVanityPet(Pawn, character_id, nInvId)) {
		LogCallEnd();
		return;
	}
	if (Pawn && !session_guid.empty() && character_id > 0 && nInvId > 0) {
		nlohmann::json ev;
		ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]      = "activate_item";
		ev["instance_id"]  = IpcClient::GetInstanceId();
		ev["session_guid"] = session_guid;
		ev["character_id"] = character_id;
		ev["pawn_id"]      = (int)Pawn->r_nPawnId;
		ev["inventory_id"] = nInvId;
		IpcClient::Send(ev.dump());
	}
	LogCallEnd();
}
