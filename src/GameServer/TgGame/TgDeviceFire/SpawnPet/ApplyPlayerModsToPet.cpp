#include "src/GameServer/TgGame/TgDeviceFire/SpawnPet/ApplyPlayerModsToPet.hpp"
#include "src/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {

// Sum the 8 layered-formula floats from all of `deployingPawn`'s
// m_EffectBuffInfo entries that match (modifierPropId, devInst-scoped).
// devInst rule mirrors FUN_109cd890 search-mode:
//   stored devInst == 0 → wildcard (skills), always matches
//   stored devInst == X → only matches when X == sourceDeviceInstId
static bool GatherBuffSums(ATgPawn* pawn, int modifierPropId, int sourceDeviceInstId,
                           float& IP, float& IM,
                           float& SP, float& SM,
                           float& LP, float& LM,
                           float& GP, float& GM) {
	IP = IM = SP = SM = LP = LM = GP = GM = 0.0f;
	if (!pawn || !pawn->m_EffectBuffInfo.Data) return false;
	int matched = 0;
	for (int i = 0; i < pawn->m_EffectBuffInfo.Num(); ++i) {
		FBuffInfo& e = pawn->m_EffectBuffInfo.Data[i];
		if (e.BuffHeader.nPropId != modifierPropId) continue;
		const int storedInst = e.BuffHeader.nReqDeviceInstId;
		if (storedInst > 0 && storedInst != sourceDeviceInstId) continue;
		IP += e.fItemPercentModifier;
		IM += e.fItemModifier;
		SP += e.fSkillPercentModifier;
		SM += e.fSkillModifier;
		LP += e.fSelfPercentModifier;
		LM += e.fSelfModifier;
		GP += e.fPercentModifier;
		GM += e.fModifier;
		++matched;
	}
	return matched > 0;
}

static UTgProperty* FindPetProp(ATgPawn* petPawn, int propId) {
	if (!petPawn || !petPawn->s_Properties.Data) return nullptr;
	for (int i = 0; i < petPawn->s_Properties.Num(); ++i) {
		UTgProperty* p = petPawn->s_Properties.Data[i];
		if (p && p->m_nPropertyId == propId) return p;
	}
	return nullptr;
}

// Apply layered formula matching binary's BuffFormula (FUN_109cd4a0).
static float ApplyFormula(float base,
                          float IP, float IM,
                          float SP, float SM,
                          float LP, float LM,
                          float GP, float GM) {
	float v1 = base * (1.0f + IP / 100.0f) + IM;
	float v2 = v1   * (1.0f + SP / 100.0f) + SM;
	float v3 = v2   * (1.0f + (LP + GP) / 100.0f) + LM + GM;
	return v3;
}

// Bridge entries: copy player's modifier-prop entry onto pet's
// m_EffectBuffInfo as a `petModifierPropId` entry. Pet's fire-time
// GetBuffedProperty (e.g. BUFF_OTHER prop 65 for damage scaling, or
// BUFF_DEVICE prop 5 for range) consults the pet's registry.
//
// Preserve the player-side 3-layer multiplicative structure on the pet:
// BuffFormula (FUN_109cd4a0) computes
//   v1 = base × (1 + ΣIP/100) + ΣIM        — ITEM layer
//   v2 = v1   × (1 + ΣSP/100) + ΣSM        — SKILL layer
//   v3 = v2   × (1 + (ΣLP+ΣGP)/100) + …    — SELF+GENERIC layer
// Each layer compounds with the next. Folding all four player-side
// sources into one SELF slot collapses the formula to a single factor
// and loses the cross-terms — a player with skill +20% AND rolled-mod
// +30% on the same pet stat would see 1.50× instead of 1.20×1.30 = 1.56×.
//
// Issuing one ApplyBuff per srcType populates separate slots on the
// SAME entry (find-or-add by propId/cat/skill/devInst — all four
// filter fields are identical here so they collapse into one entry).
// The aggregator then sees all four slots populated and BuffFormula
// reproduces the player's layered scaling on the pet.
static void RegisterBuffOnPet(ATgPawn* petPawn, int targetPropId,
                              float IP, float IM,
                              float SP, float SM,
                              float LP, float LM,
                              float GP, float GM) {
	auto issue = [&](unsigned char srcType, float pct, float abs, const char* tag) {
		if (pct == 0.0f && abs == 0.0f) return;
		FBuffHeader header{};
		header.nPropId          = targetPropId;
		header.nReqCategoryCode = 0;
		header.nReqSkillId      = 0;
		header.nReqDeviceInstId = 0;  // wildcard on the pet — pet queries with 0
		if (pct != 0.0f) {
			TgPawn__ApplyBuff::Call(petPawn, /*edx=*/nullptr, header,
				/*calc=68 PERC_INCREASE*/68, pct,
				/*bRemove=*/0, srcType);
		}
		if (abs != 0.0f) {
			TgPawn__ApplyBuff::Call(petPawn, /*edx=*/nullptr, header,
				/*calc=67 ADD*/67, abs,
				/*bRemove=*/0, srcType);
		}
		Logger::Log("pet_spawn",
			"  [pet-buff/%s] propId=%d  pct=%.2f abs=%.2f  on pet=%p\n",
			tag, targetPropId, pct, abs, petPawn);
	};

	// Per-layer mirror so the pet's BuffFormula produces the same
	// multiplicative composition the player would see on their own stat.
	issue(/*ITEM=*/1, IP, IM, "item");
	issue(/*SKILL=*/0, SP, SM, "skill");
	issue(/*SELF=*/3, LP, LM, "self");
	issue(/*OTHER=*/4, GP, GM, "other");
}

// Two kinds of mappings:
//   BUFF mode: register on pet's m_EffectBuffInfo so the pet's fire-time
//     reads (GetBuffedProperty / formula) pick it up. Used for modifier
//     props that scale fire output (damage, range, accuracy).
//   DIRECT mode: write directly to pet's s_Properties[m_fRaw]. Used for
//     stats set once at spawn (HP, lifespan).
enum BridgeMode { BRIDGE_BUFF, BRIDGE_DIRECT };

struct PetMapping {
	int  modifierPropId;   // player's m_EffectBuffInfo prop id
	int  petTargetPropId;  // pet's prop to scale (modifier prop for BUFF, base prop for DIRECT)
	int  petTargetExtra;   // optional secondary direct target (e.g. HEALTH_MAX → also HEALTH); -1 if none
	BridgeMode mode;
};

// Map player modifier props → pet-side scaling.
//
// 350 PetDamage     → pet's prop 65 (Effect Damage Modifier) via BUFF: scales pet's TgEffectDamage at fire time
// 381 PetRange      → pet's prop 114 (Range Modifier) via BUFF: scales pet's GetPropertyValueById(prop=5)
// 382 PetDmgRadius  → pet's prop 352 (AOE Radius Modifier) via BUFF: scales pet's GetPropertyValueById(prop=6)
// 383 PetAccuracy   → pet's prop 113 (Accuracy Modifier) via BUFF: scales pet's GetPropertyValueById(prop=10)
// 366 PetMaxHealth  → pet's prop 304 (HEALTH_MAX) via DIRECT (also 51 HEALTH); HP is set once at spawn
// 355 PetLifespan   → pet's prop 354 (PET_LIFESPAN) via DIRECT
static const PetMapping kPetMap[] = {
	{ 350, 65,  -1, BRIDGE_BUFF   },
	{ 381, 114, -1, BRIDGE_BUFF   },
	{ 382, 352, -1, BRIDGE_BUFF   },
	{ 383, 113, -1, BRIDGE_BUFF   },
	{ 366, 304, 51, BRIDGE_DIRECT },
	{ 355, 354, -1, BRIDGE_DIRECT },
};

}  // namespace

void ApplyPlayerModsToPet::Apply(ATgPawn* deployingPawn, ATgPawn* petPawn, int sourceDeviceInstId) {
	if (!deployingPawn || !petPawn) return;
	if (!deployingPawn->m_EffectBuffInfo.Data || deployingPawn->m_EffectBuffInfo.Num() == 0) {
		Logger::Log("pet_spawn",
			"ApplyPlayerModsToPet: pawn=0x%p pet=0x%p — no buff entries on deployer, skipping\n",
			deployingPawn, petPawn);
		return;
	}

	int touched = 0;
	for (size_t m = 0; m < sizeof(kPetMap) / sizeof(kPetMap[0]); ++m) {
		const PetMapping& pm = kPetMap[m];
		float IP, IM, SP, SM, LP, LM, GP, GM;
		if (!GatherBuffSums(deployingPawn, pm.modifierPropId, sourceDeviceInstId,
		                    IP, IM, SP, SM, LP, LM, GP, GM))
			continue;

		Logger::Log("pet_spawn",
			"ApplyPlayerModsToPet: mode=%s modProp=%d → petProp=%d  "
			"IP=%.2f IM=%.2f SP=%.2f SM=%.2f LP=%.2f LM=%.2f GP=%.2f GM=%.2f\n",
			pm.mode == BRIDGE_BUFF ? "BUFF" : "DIRECT",
			pm.modifierPropId, pm.petTargetPropId,
			IP, IM, SP, SM, LP, LM, GP, GM);

		if (pm.mode == BRIDGE_BUFF) {
			// Register modifier entry on pet's buff registry; pet's
			// fire-time GetBuffedProperty consults this.
			RegisterBuffOnPet(petPawn, pm.petTargetPropId,
			                  IP, IM, SP, SM, LP, LM, GP, GM);
			++touched;
		} else {
			// Direct write to pet's s_Properties — used for one-shot
			// spawn-time stats (HP, lifespan).
			for (int target_idx = 0; target_idx < 2; ++target_idx) {
				int targetPropId = (target_idx == 0) ? pm.petTargetPropId : pm.petTargetExtra;
				if (targetPropId < 0) break;
				UTgProperty* prop = FindPetProp(petPawn, targetPropId);
				if (!prop) continue;
				const float base = prop->m_fRaw;
				if (base == 0.0f) continue;
				float scaled = ApplyFormula(base, IP, IM, SP, SM, LP, LM, GP, GM);
				if (prop->m_fMaximum > 0.0f && prop->m_fMinimum <= prop->m_fMaximum) {
					if (scaled < prop->m_fMinimum) scaled = prop->m_fMinimum;
					else if (scaled > prop->m_fMaximum) scaled = prop->m_fMaximum;
				}
				prop->m_fRaw = scaled;
				// Fan out via SetProperty so engine fields (r_nMaxHealth,
				// r_nHealth, etc.) mirror.
				petPawn->SetProperty(targetPropId, prop->m_fRaw);

				// PetLifespan special: UC's TGPID_PET_LIFESPAN constant (354)
				// has no UC-side reader — the spawned bot's auto-destroy
				// timer is UE3's `Actor.LifeSpan` field, which the binary's
				// stripped bot-init may or may not seed from prop 354.
				// Write it explicitly to make 355 PetLifespan modifier
				// actually extend the drone/turret duration regardless.
				// Skip when LifeSpan is 0 (Actor's "no auto-destroy" sentinel)
				// so we don't introduce a timer where one wasn't intended.
				if (targetPropId == 354 && petPawn->LifeSpan > 0.0f) {
					const float oldLS = petPawn->LifeSpan;
					petPawn->LifeSpan = scaled;
					Logger::Log("pet_spawn",
						"  [pet-direct] Actor.LifeSpan %.2f → %.2f  (from prop354 base=%.2f scaled=%.2f)\n",
						oldLS, petPawn->LifeSpan, base, scaled);
				}

				Logger::Log("pet_spawn",
					"  [pet-direct] propId=%d  base=%.2f → %.2f\n",
					targetPropId, base, scaled);
				++touched;
			}
		}
	}

	if (touched > 0) {
		Logger::Log("pet_spawn",
			"ApplyPlayerModsToPet: pet=0x%p modified %d slot(s) from deployer 0x%p (devInst=%d)\n",
			petPawn, touched, deployingPawn, sourceDeviceInstId);
	}
}
