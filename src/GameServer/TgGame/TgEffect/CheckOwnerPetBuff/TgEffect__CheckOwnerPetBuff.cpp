#include "src/GameServer/TgGame/TgEffect/CheckOwnerPetBuff/TgEffect__CheckOwnerPetBuff.hpp"
#include "src/GameServer/TgGame/_effect_core/DeviceLookup.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Reimplementation of TgEffect::CheckOwnerPetBuff — stripped stub at 0x10a6f290.
// Scales a pet/turret's outgoing value by its OWNER pawn's buff registry, so an
// engineer's turret/drone inherits the owner's device mods + class skills.
// See .planning/effect-buff-property-canonical.md §2/§4 (Q3).
//
// Clean-room rebuild: device class-skill resolved via the canonical
// DeviceLookup (scan the owner's equipped devices) — NO DeviceCategorySkill
// side-map; class checks via ObjectClassCache, not IsA()/GetFullName().
//
// Chain: effect's group instigator (the firing pet) → r_Owner (the spawning
// pawn) → query owner's GetBuffedProperty scoped to the spawning device.
//
// Device scoping is load-bearing: rolled pet-damage mods (prop 350) and the
// ModResolver Output Mod (385) are stored against the spawning device's
// instance id; the spawning device's class-skill id (m_nSkillId) gates
// device-class skills (Heavy Artillery req 514, Robotics req 486, …). The pet
// carries the spawning device instance in s_nSpawnerDeviceInstId (+0x1070, set
// by SpawnBotById/SpawnPet); context 4 (BUFF_OTHER) expands prop 350 → {350,385}.

typedef void(__fastcall* GetBuffedPropertyFn)(
	ATgPawn*, void*, unsigned char, int, int, int, int, int, float, float*, void*);
static const GetBuffedPropertyFn GetBuffedPropertyNative = (GetBuffedPropertyFn)0x109d7ff0;

void __fastcall TgEffect__CheckOwnerPetBuff::Call(UTgEffect* effect, void* /*edx*/,
                                                  int nPropertyId, float fCurrAmount,
                                                  float* fNewAmount) {
	if (!fNewAmount) return;
	*fNewAmount = fCurrAmount;  // default: pass-through

	if (!effect || fCurrAmount == 0.0f) return;
	UTgEffectGroup* g = effect->m_EffectGroup;
	if (!g) return;

	AActor* inst = g->m_Instigator;
	if (!inst || !ObjectClassCache::ClassNameContains(inst, "TgPawn")) return;
	ATgPawn* petPawn = static_cast<ATgPawn*>(inst);

	// r_Owner (+0x106C): the owning pawn for player-spawned pets / henchmen;
	// null for owner-less AI bots (nothing to scale by).
	ATgPawn* ownerPawn = petPawn->r_Owner;
	if (!ownerPawn || !ObjectClassCache::ClassNameContains(ownerPawn, "TgPawn")) return;

	// Spawning-device instance on the pet (+0x1070), and its class-skill id from
	// the owner's equipped devices (canonical, replaces DeviceCategorySkill).
	const int spawnerDevInst = petPawn->s_nSpawnerDeviceInstId;
	const int spawnerSkillId = DeviceLookup::SkillIdForDevice(ownerPawn, spawnerDevInst);

	float buffedValue = fCurrAmount;
	GetBuffedPropertyNative(
		ownerPawn, /*edx=*/nullptr,
		/*eRequestContext=*/4,          // BUFF_OTHER → {350, 385} for pet-damage prop 350
		/*nPropId=*/nPropertyId,
		/*nReqCategoryCode=*/g->m_nCategoryCode,
		/*nReqSkillId=*/spawnerSkillId,
		/*nReqDeviceInstId=*/spawnerDevInst,
		/*bUsePotencyModifier=*/0,
		/*fBaseValue=*/fCurrAmount,
		/*fBuffedValue=*/&buffedValue,
		/*Effect=*/effect);

	*fNewAmount = buffedValue;

	if (buffedValue != fCurrAmount && Logger::IsChannelEnabled("effects")) {
		Logger::Log("effects",
			"[OWNER-PET-BUFF] effect=%p propId=%d  %.3f -> %.3f  pet=%p owner=%p egId=%d spawnerDevInst=%d spawnerSkillId=%d\n",
			effect, nPropertyId, fCurrAmount, buffedValue,
			petPawn, ownerPawn, g->m_nEffectGroupId, spawnerDevInst, spawnerSkillId);
	}
}
