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
// 381 PetRange      → pet's prop 114 (Range Modifier) via BUFF: scales pet's GetPropertyValueById(prop=5)
// 382 PetDmgRadius  → pet's prop 352 (AOE Radius Modifier) via BUFF: scales pet's GetPropertyValueById(prop=6)
// 383 PetAccuracy   → pet's prop 113 (Accuracy Modifier) via BUFF: scales pet's GetPropertyValueById(prop=10)
// 366 PetMaxHealth  → pet's prop 304 (HEALTH_MAX) via DIRECT (also 51 HEALTH); HP is set once at spawn
// 355 PetLifespan   → pet's prop 354 (PET_LIFESPAN) via DIRECT
//
// **prop 350 PetDamage is intentionally NOT bridged.** UC `TgEffectDamage::
// ApplyEffect` calls `CheckOwnerPetBuff(350, ...)` at fire time, which queries
// the owner's `m_EffectBuffInfo` for prop 350 (BUFF_OTHER expansion = {350,
// 385} — also picks up Output Mod). Bridging player's prop 350 entries onto
// the pet's prop 65 caused double-application: the same skill/mod buff hit
// the damage value once via owner-side `CheckOwnerPetBuff` and a second time
// via pet-side `CheckEffectBuffModifier` (which queries pet for prop 51
// expansion {65, 385, …}). This bridge dates from when `CheckOwnerPetBuff`
// was a stripped stub and we needed a spawn-time path; that native is now
// re-implemented (`TgEffect/CheckOwnerPetBuff/`) so the bridge is redundant.
// The other modifier props below stay because there is no analogous fire-
// time owner-side query for range / radius / accuracy / HP / lifespan.
static const PetMapping kPetMap[] = {
	{ 381, 114, -1, BRIDGE_BUFF   },
	{ 382, 352, -1, BRIDGE_BUFF   },
	{ 383, 113, -1, BRIDGE_BUFF   },
	// HP_MAX (304) only — NOT also 51. The binary's
	// TgPawn::ApplyProperty(304, newMax) self-calls SetProperty(51, newMax)
	// to keep Health in sync with the new ceiling (per decompilation of
	// 0x109cc7d0). Listing 51 as a secondary direct target re-reads
	// prop 51's now-already-scaled m_fRaw and applies the buff a second
	// time, producing Health values that overshoot the new max (e.g.
	// 1320 × 1.3 × 1.3 = 2230 instead of 1716 after the +30% skill pass;
	// then × 1.7 × 1.7 = 4958 instead of 2917 after the +70% Output Mod
	// pass). Cosmetic on the server (heal cap reads r_nHealthMaximum =
	// correctly buffed value), but ugly on the wire — the client briefly
	// sees Health > Max until first damage tick clamps it.
	{ 366, 304, -1, BRIDGE_DIRECT },
	// Output Mod (prop 385) — every device carries an innate +70% (or +75%
	// overclocked) Output Mod entry on the player's buff registry. The
	// inventory card surfaces the pet's HP as `base × 1.7`, implying the
	// engine intends Output Mod to scale pet max HP the same way it scales
	// deployable HP (kBuffDeviceModMap routes 385 → deployable's 304). The
	// pet path didn't have the equivalent entry, so the rocket-turret's
	// real max stayed at the bot's base hit_points (1320 → expected 2244).
	{ 385, 304, -1, BRIDGE_DIRECT },
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

	// Diagnostic dump of all matching player-side entries the bridge would see.
	// If pet HP scaling isn't visible in-game, the first thing to verify is
	// whether the relevant entries exist on the deployer at the moment the
	// pet spawns. Spotting "no entry for prop 366" here when Robotics has
	// 4 HP-buffing skills allocated points at a Reapply timing bug upstream
	// (skills sync arriving after first Reapply, etc.).
	Logger::Log("pet_spawn",
		"ApplyPlayerModsToPet: pawn=0x%p pet=0x%p devInst=%d  m_EffectBuffInfo.Count=%d\n",
		deployingPawn, petPawn, sourceDeviceInstId,
		deployingPawn->m_EffectBuffInfo.Num());
	for (int i = 0; i < deployingPawn->m_EffectBuffInfo.Num(); ++i) {
		const FBuffInfo& e = deployingPawn->m_EffectBuffInfo.Data[i];
		// Only dump entries the pet bridge can consume (HP/lifespan/damage/range/etc.).
		const int p = e.BuffHeader.nPropId;
		if (p != 366 && p != 385 && p != 350 && p != 355 &&
		    p != 381 && p != 382 && p != 383)
			continue;
		Logger::Log("pet_spawn",
			"  [%d] prop=%d cat=%d skill=%d devInst=%d  IP=%.2f IM=%.2f SP=%.2f SM=%.2f LP=%.2f LM=%.2f GP=%.2f GM=%.2f\n",
			i, p, e.BuffHeader.nReqCategoryCode, e.BuffHeader.nReqSkillId,
			e.BuffHeader.nReqDeviceInstId,
			e.fItemPercentModifier, e.fItemModifier,
			e.fSkillPercentModifier, e.fSkillModifier,
			e.fSelfPercentModifier, e.fSelfModifier,
			e.fPercentModifier, e.fModifier);
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

				// For HP_MAX (304), sync the PRI for HUD readers. The binary's
				// SetProperty(304) → ApplyProperty(304) (decompiled @
				// 0x109cc7d0) already writes r_nHealthMaximum at +0x43C and
				// self-calls SetProperty(51, newMax) to lift current Health to
				// the new ceiling — so no defensive write to r_nHealthMaximum
				// or Health is needed. But TgRepInfo_Player's
				// r_nHealthMaximum / r_nHealthCurrent (what HUD HP-bar code
				// reads) aren't part of the SetProperty fanout — UC's
				// TgPawn::UpdateHealth normally drives that broadcast, and on
				// pet spawn the path isn't necessarily traversed. Mirror it
				// explicitly so the client HP bar lands on the buffed max.
				if (targetPropId == 304 && petPawn->PlayerReplicationInfo) {
					ATgRepInfo_Player* pri =
						(ATgRepInfo_Player*)petPawn->PlayerReplicationInfo;
					pri->r_nHealthMaximum = petPawn->r_nHealthMaximum;
					pri->r_nHealthCurrent = petPawn->Health;
					pri->bNetDirty        = 1;
					pri->bForceNetUpdate  = 1;
					Logger::Log("pet_spawn",
						"  [pet-direct/304] r_nHealthMaximum=%d Health=%d (PRI synced)\n",
						petPawn->r_nHealthMaximum, petPawn->Health);
				}

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
