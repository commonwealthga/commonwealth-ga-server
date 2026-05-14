#include "src/GameServer/TgGame/TgEffect/CheckOwnerPetBuff/TgEffect__CheckOwnerPetBuff.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>

// Same GetBuffedProperty binding as the CheckEffectBuffModifier impl.
typedef void(__fastcall* GetBuffedPropertyFn)(
	ATgPawn*, void*,
	unsigned char, int, int, int, int, int,
	float, float*, void*);
static const GetBuffedPropertyFn GetBuffedPropertyNative =
	(GetBuffedPropertyFn)0x109d7ff0;

// Reimplementation of TgEffect::CheckOwnerPetBuff — stripped stub at
// 0x10a6f290.
//
// Lookup chain:
//   1. effect->m_EffectGroup->m_Instigator → the firing actor.
//   2. If the firing actor is a pawn AND has r_Owner set, treat it as a pet
//      whose damage we want owner-side scaling for. r_Owner points to the
//      deploying / spawning pawn (per reference_henchman_pet_r_owner_for_isenemy).
//   3. Query owner pawn's GetBuffedProperty(BUFF_OTHER, nPropertyId, ...) →
//      writes scaled amount back to *fNewAmount.
//
// **Device-instance scoping:** the query MUST pass the SPAWNING device's
// invId (the player's Personal Turret / Pet Deployable item, e.g. invId
// 10009), not 0. Rolled mods on the spawning device are stored against
// `nReqDeviceInstId = device's invId` (per `Inventory::ApplyRolledModEffects`),
// and the binary's GetBuffIndex search treats `entry.devInst > 0` as
// "must equal query exactly". Querying with devInst=0 was missing every
// rolled prop-350 pet-damage mod AND the ModResolver-prepended Output Mod
// (prop 385) — concrete failure: Rocket Turret 1000 base × 1.40 actual
// instead of 1000 × ~1.70+ from Output Mod alone (2026-05-14 audit).
//
// The pet pawn carries `s_nSpawnerDeviceInstId` (offset 0x1070) — set by
// SpawnBotById/SpawnPet at deploy time — that points back to the owner's
// spawning device. Use it here.
//
// For non-pet instigators (player firing their own weapon, deployable firing
// without a pet middleman), r_Owner is null and we leave fNewAmount at its
// passed-in value.
void __fastcall TgEffect__CheckOwnerPetBuff::Call(UTgEffect* effect, void* /*edx*/,
                                                  int nPropertyId, float fCurrAmount,
                                                  float* fNewAmount) {
	if (!fNewAmount) return;
	*fNewAmount = fCurrAmount;  // default: pass-through

	if (!effect || fCurrAmount == 0.0f) return;
	UTgEffectGroup* g = effect->m_EffectGroup;
	if (!g) return;

	AActor* inst = g->m_Instigator;
	if (!inst || !inst->Class) return;
	const char* cn = inst->Class->GetFullName();
	if (!cn || strstr(cn, "TgPawn") == nullptr) return;
	ATgPawn* petPawn = (ATgPawn*)inst;

	// r_Owner: ATgPawn* on this pawn pointing at the owning pawn. SDK header
	// at TgGame_classes.h:13109 — `class ATgPawn* r_Owner;` at offset 0x106C
	// (CPF_Net). Player-spawned pets / henchmen carry it; AI bots without an
	// owner have it null.
	ATgPawn* ownerPawn = petPawn->r_Owner;
	if (!ownerPawn || !ownerPawn->Class) return;
	const char* on = ownerPawn->Class->GetFullName();
	if (!on || strstr(on, "TgPawn") == nullptr) return;

	// Spawning-device invId on the pet (offset 0x1070, populated by
	// SpawnBotById / SpawnPet at deploy time). Falls back to 0 (wildcard
	// match against entries stored with devInst=0) when missing.
	const int spawnerDevInst = petPawn->s_nSpawnerDeviceInstId;

	float buffedValue = fCurrAmount;
	GetBuffedPropertyNative(
		ownerPawn, /*edx=*/nullptr,
		/*eRequestContext=*/4,         // BUFF_OTHER → expansion {350, 385} for prop 350
		/*nPropId=*/nPropertyId,
		/*nReqCategoryCode=*/g->m_nCategoryCode,
		/*nReqSkillId=*/0,
		/*nReqDeviceInstId=*/spawnerDevInst,
		/*bUsePotencyModifier=*/0,
		/*fBaseValue=*/fCurrAmount,
		/*fBuffedValue=*/&buffedValue,
		/*Effect=*/effect);

	*fNewAmount = buffedValue;

	if (buffedValue != fCurrAmount && Logger::IsChannelEnabled("effects")) {
		Logger::Log("effects",
			"[OWNER-PET-BUFF] effect=%p propId=%d  %.3f -> %.3f  pet=%p owner=%p egId=%d spawnerDevInst=%d\n",
			effect, nPropertyId, fCurrAmount, buffedValue,
			petPawn, ownerPawn, g->m_nEffectGroupId, spawnerDevInst);
	}
}
