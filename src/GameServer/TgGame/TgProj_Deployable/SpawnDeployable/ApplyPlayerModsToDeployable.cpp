#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/ApplyPlayerModsToDeployable.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/ModifierProps.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {
// Find the FBuffInfo entry on the pawn whose nPropId matches `modifierPropId`.
// Sums across multiple matching entries (mirrors the binary's aggregator).
// Out: (IP, IM, SP, SM, LP, LM, GP, GM) — the 8 floats from FBuffInfo summed
// across all matches, used by the layered formula.
static bool GatherBuffSums(ATgPawn* pawn, int modifierPropId,
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

// Find the deployable's UTgProperty* for prop_id (or nullptr if absent).
static UTgProperty* FindDeployableProp(ATgDeployable* deployable, int propId) {
	if (!deployable || !deployable->s_Properties.Data) return nullptr;
	for (int i = 0; i < deployable->s_Properties.Num(); ++i) {
		UTgProperty* p = deployable->s_Properties.Data[i];
		if (p && p->m_nPropertyId == propId) return p;
	}
	return nullptr;
}
} // anonymous namespace

void ApplyPlayerModsToDeployable::Apply(ATgPawn* pawn, ATgDeployable* deployable) {
	if (!pawn || !deployable) return;
	if (!pawn->m_EffectBuffInfo.Data || pawn->m_EffectBuffInfo.Num() == 0) {
		Logger::Log("inventory",
			"ApplyPlayerModsToDeployable: pawn=0x%p deployable=0x%p — no buff registry entries, skipping\n",
			pawn, deployable);
		return;
	}

	const int kNumMaps = ModifierProps::kBuffDeviceModMapCount;
	int totalModified = 0;

	for (int m = 0; m < kNumMaps; ++m) {
		const ModifierProps::BuffDeviceMapping& mm = ModifierProps::kBuffDeviceModMap[m];
		float IP, IM, SP, SM, LP, LM, GP, GM;
		if (!GatherBuffSums(pawn, mm.modifierPropId, IP, IM, SP, SM, LP, LM, GP, GM))
			continue;

		// Apply layered formula to each target prop on the deployable.
		for (int t = 0; t < 5 && mm.targets[t] >= 0; ++t) {
			int targetPropId = mm.targets[t];
			UTgProperty* prop = FindDeployableProp(deployable, targetPropId);
			if (!prop) continue;

			const float base = prop->m_fRaw;

			// Layered formula matching the binary's BuffFormula (FUN_109cd4a0):
			//   v1 = base * (1 + IP/100) + IM
			//   v2 = v1   * (1 + SP/100) + SM
			//   v3 = v2   * (1 + (LP+GP)/100) + LM + GM
			float v1 = base * (1.0f + IP / 100.0f) + IM;
			float v2 = v1   * (1.0f + SP / 100.0f) + SM;
			float v3 = v2   * (1.0f + (LP + GP) / 100.0f) + LM + GM;

			// Clamp to prop's [min, max] when both bounds are populated.
			float final = v3;
			if (prop->m_fMaximum > 0.0f && prop->m_fMinimum <= prop->m_fMaximum) {
				if (final < prop->m_fMinimum) final = prop->m_fMinimum;
				else if (final > prop->m_fMaximum) final = prop->m_fMaximum;
			}

			Logger::Log("inventory",
				"ApplyPlayerModsToDeployable: deployable=0x%p modProp=%d → targetProp=%d  "
				"base=%.2f IP=%.2f IM=%.2f SP=%.2f SM=%.2f LP=%.2f LM=%.2f GP=%.2f GM=%.2f → %.2f\n",
				deployable, mm.modifierPropId, targetPropId,
				base, IP, IM, SP, SM, LP, LM, GP, GM, final);

			prop->m_fRaw = final;
			++totalModified;
		}
	}

	if (totalModified > 0) {
		// SetProperty fan-out: ATgDeployable::ApplyProperty (slot 251 @ 0x10a1ade0)
		// only mirrors props 8 and 278 to engine fields. For the others we just
		// updated m_fRaw directly above; if a prop is also engine-mirrored
		// (proximity radius), call SetProperty so the mirror fans out too.
		// Safe because SetProperty's linear scan finds the entry we just mutated.
		for (int m = 0; m < kNumMaps; ++m) {
			const ModifierProps::BuffDeviceMapping& mm = ModifierProps::kBuffDeviceModMap[m];
			for (int t = 0; t < 5 && mm.targets[t] >= 0; ++t) {
				int propId = mm.targets[t];
				if (propId == 8 || propId == 278) {
					UTgProperty* prop = FindDeployableProp(deployable, propId);
					if (prop) deployable->SetProperty(propId, prop->m_fRaw);
				}
			}
		}

		Logger::Log("inventory",
			"ApplyPlayerModsToDeployable: deployable=0x%p modified %d prop(s)\n",
			deployable, totalModified);
	}

	// Special case: the AoE Radius modifier (prop 352) needs to scale the
	// timer-bomb engine mirrors `s_fProximityRadius` (server-side proximity
	// detection) and `r_fClientProximityRadius` (replicated; what the client
	// "you're in bomb range" warning indicator reads). These are NOT in
	// s_Properties — they're TgDeployable fields populated unconditionally
	// from DB by the timer-bomb block in SpawnDeployableActor. Without this,
	// the warning radius stays at the unmodded base while the damage radius
	// (reading prop 6 from s_Properties) scales — visible asymmetry where the
	// player is hit while standing outside the warning circle.
	//
	// Apply the same layered formula to the proximity fields directly. Only
	// runs when prop 352 has at least one entry on the pawn.
	{
		float IP, IM, SP, SM, LP, LM, GP, GM;
		if (GatherBuffSums(pawn, /*modifierPropId=*/352,
		                   IP, IM, SP, SM, LP, LM, GP, GM)) {
			float proxBase = deployable->s_fProximityRadius;
			if (proxBase > 0.0f) {
				float v1 = proxBase * (1.0f + IP / 100.0f) + IM;
				float v2 = v1       * (1.0f + SP / 100.0f) + SM;
				float v3 = v2       * (1.0f + (LP + GP) / 100.0f) + LM + GM;

				deployable->s_fProximityRadius       = v3;
				deployable->r_fClientProximityRadius = v3;
				deployable->bNetDirty                 = 1;
				deployable->bForceNetUpdate           = 1;

				Logger::Log("inventory",
					"ApplyPlayerModsToDeployable: deployable=0x%p prop352→proximity  base=%.2f → %.2f\n",
					deployable, proxBase, v3);
			}
		}
	}
}
