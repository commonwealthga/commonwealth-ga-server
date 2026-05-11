#include "src/GameServer/TgGame/TgEffect/TrackStats/TgEffect__TrackStats.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <string>

// We do NOT call the binary's TgPawn::AddMoralePoints @ 0x109d2f00. Its
// validation chain begins with `FUN_109422b0(DAT_ASSEMBLY_MANAGER, nDeviceModeID)`
// then unconditionally dereferences `[result + 0xC]`. With the original asm
// data this presumably always resolved (likely a sentinel entry for id=0 plus
// real entries for every fire-mode id), but in our environment it returns
// null for arbitrary target-fire-mode ids passed from TgEffectDamage, so the
// dereference at 0x109d2f4e access-violates on first damage event. We inline
// the surviving gates instead.
//
// Pawn-side gates we replicate (matches the asm at 0x109d2f6d..0x109d2f8c):
//   - test [esi+0x3D8], 0x10000000   → r_bAllowAddMoralePoints set
//   - cmp  [esi+0x1520], -1          → r_nMoraleDeviceSlot >= 0
//   - test [edi+0x264],  0x200       → GRI->r_bAllowBuildMorale set (skipped;
//                                       we always set this in InitGameRepInfo)
// Then add and clamp to [0, r_fRequiredMoralePoints].

// Morale-points-per-damage-or-heal ratio. 1 morale per 4 HP delta. With the
// default 100-required threshold, killing four full-HP 100-HP enemies fills the
// bar; takes about as many full heals to charge. Tunable; eventually should
// scale from prop 337 ("Boost Charge Rate") via the buff registry if any DB
// effect ever lands there.
static constexpr float kMoralePointsPerHealthDelta = 0.25f;

void __fastcall TgEffect__TrackStats::Call(UTgEffect* /*Effect*/, void* /*edx*/,
                                           ATgPawn* Instigator, AActor* Target, FImpactInfoBytes Impact,
                                           float fDamage, int iTargetDeviceModeId,
                                           unsigned long bIsEnemy, float /*fMissingHealth*/) {
	if (Instigator == nullptr) return;
	if (Target     == nullptr) return;

	// UC dispatches both damage AND heal through here:
	//   damage path: fDamage = +fHealthChange (post-clamp HP drop)
	//   heal path:   fDamage = -fProratedAmount (negated by caller)
	// Self-effects don't credit (a player healing themselves with their own
	// adrenaline gun, or eating splash from their own grenade).
	if (Target == reinterpret_cast<AActor*>(Instigator)) return;

	const bool isHeal = (fDamage < 0.0f);
	const float magnitude = isHeal ? -fDamage : fDamage;
	if (magnitude <= 0.0f) return;

	// Damage on a friendly target = team damage. UC also calls TrackTeamDamage
	// in that case (a separate stub) which originally awarded zero morale.
	// Don't credit.
	if (!isHeal && !bIsEnemy) {
		if (Logger::IsChannelEnabled("morale")) {
			Logger::Log("morale",
				"[TrackStats] team-damage no credit: instigator=0x%p target=0x%p fDamage=%.2f\n",
				Instigator, Target, fDamage);
		}
		return;
	}

	// Block morale credit when the damage source traces back to a morale
	// device. UC's TgDevice_Morale.DeviceFiring.BeginState already calls
	// SetAllowAddMoralePoints(false) (which our `r_bAllowAddMoralePoints` gate
	// below honors), but that flag is reset on DeviceFiring.EndState — typically
	// 1–2 seconds after the morale fires the animation. Bombs spawned by the
	// morale device (Shatter Bomb deployable, etc.) detonate seconds later,
	// AFTER DeviceFiring has ended and the flag is back to true. Their
	// explosion damage would otherwise credit morale → instant ultimate refill
	// → infinite loop.
	//
	// Trace chain (handles both direct morale-fire and bomb-spawned-by-morale):
	//   Impact.DeviceModeReference (UTgDeviceFire* @ FImpactInfo+0x54)
	//     → fireMode.m_Owner (AActor* @ UTgDeviceFire+0x3C)
	//       Case A — TgDevice_Morale directly (e.g. melee/hitscan morale device):
	//         skip.
	//       Case B — TgDeployable (bomb): chase .r_Owner (ATgDevice* @ +0x2AC)
	//             to the source device; if that's a TgDevice_Morale, skip.
	//
	// Class-name strstr per `reference_sdk_staticclass_misalignment` — IsA() is
	// unreliable on this server build.
	//
	// We previously copied each GetFullName() into std::string to dodge the
	// shared-buffer clobber when both names cohabit a log line. That's only
	// needed when we actually log; for the gate decision itself, evaluating
	// each name+strstr in sequence is allocation-free and immune to clobber
	// (we don't keep the pointer around after strstr).
	{
		UObject* deviceModeRef = (UObject*)Impact.dw[0x54 / 4];
		UTgDeviceFire* fireMode  = (UTgDeviceFire*)deviceModeRef;
		AActor*        fireOwner = fireMode ? fireMode->m_Owner : nullptr;
		ATgDevice*     sourceDevice = nullptr;

		bool ownerIsDeployable = false;
		bool ownerIsMorale     = false;
		if (fireOwner && fireOwner->Class) {
			const char* oc = fireOwner->Class->GetFullName();
			if (oc) {
				ownerIsMorale     = (strstr(oc, "TgDevice_Morale") != nullptr);
				ownerIsDeployable = (strstr(oc, "TgDeploy")        != nullptr);
			}
		}

		bool srcIsMorale = false;
		if (ownerIsDeployable) {
			sourceDevice = ((ATgDeployable*)fireOwner)->r_Owner;
			if (sourceDevice && sourceDevice->Class) {
				const char* sc = sourceDevice->Class->GetFullName();
				if (sc) srcIsMorale = (strstr(sc, "TgDevice_Morale") != nullptr);
			}
		}

		if (ownerIsMorale || srcIsMorale) {
			if (Logger::IsChannelEnabled("morale")) {
				// Re-evaluate the names into std::strings just for the log so
				// the two GetFullName() pointers don't share a buffer in the
				// same printf line.
				std::string oc = (fireOwner && fireOwner->Class && fireOwner->Class->GetFullName())
				                   ? fireOwner->Class->GetFullName() : std::string("<null>");
				std::string sc = (sourceDevice && sourceDevice->Class && sourceDevice->Class->GetFullName())
				                   ? sourceDevice->Class->GetFullName() : std::string("<n/a>");
				Logger::Log("morale",
					"[TrackStats] morale-device source (no credit): instigator=0x%p target=0x%p "
					"fireOwner.class=%s sourceDevice.class=%s fDamage=%.2f\n",
					Instigator, Target, oc.c_str(), sc.c_str(), fDamage);
			}
			return;
		}

		// Diagnostic: record the source attribution we DID see, so when the
		// chain breaks we can tell whether DeviceModeReference was null,
		// fireMode.m_Owner was null, or r_Owner on the deployable was missing.
		if (Logger::IsChannelEnabled("morale")) {
			std::string oc = (fireOwner && fireOwner->Class && fireOwner->Class->GetFullName())
			                   ? fireOwner->Class->GetFullName() : std::string("<null>");
			std::string sc = (sourceDevice && sourceDevice->Class && sourceDevice->Class->GetFullName())
			                   ? sourceDevice->Class->GetFullName() : std::string("<n/a>");
			Logger::Log("morale",
				"[TrackStats] credit-eligible: instigator=0x%p target=0x%p "
				"deviceModeRef=%p fireOwner=%p ownerClass=%s srcDevice=%p srcClass=%s fDamage=%.2f\n",
				Instigator, Target,
				(void*)deviceModeRef, (void*)fireOwner, oc.c_str(),
				(void*)sourceDevice, sc.c_str(),
				fDamage);
		}
	}

	// Pawn must have a morale device equipped (otherwise nothing to charge).
	if (Instigator->r_nMoraleDeviceSlot < 0) return;

	// r_bAllowAddMoralePoints — UC clears it during the morale device's
	// DeviceFiring state to prevent re-charging mid-fire. SDK declares this as
	// a 1-bit field at offset 0x3D8 with mask 0x10000000; read raw to side-step
	// SDK bitfield-pack risk.
	const uint32_t allowBits = *(uint32_t*)((char*)Instigator + 0x3D8);
	if ((allowBits & 0x10000000u) == 0) return;

	const float points    = magnitude * kMoralePointsPerHealthDelta;
	const float before    = Instigator->m_fCurrentMoralePoints;
	const float threshold = Instigator->r_fRequiredMoralePoints;
	float after = before + points;
	if (after < 0.0f)            after = 0.0f;
	else if (after > threshold)  after = threshold;

	if (after != before) {
		// m_fCurrentMoralePoints is the server-canonical accumulator; the
		// replicated field r_fCurrentServerMoralePoints is what the client's
		// repnotify copies back into its local m_fCurrentMoralePoints and the
		// morale-bar UI reads from. Write both.
		Instigator->m_fCurrentMoralePoints      = after;
		Instigator->r_fCurrentServerMoralePoints = after;
		Instigator->bForceNetUpdate = 1;
		Instigator->bNetDirty       = 1;
	}

	if (Logger::IsChannelEnabled("morale")) {
		Logger::Log("morale",
			"[TrackStats] %s: instigator=0x%p target=0x%p mag=%.2f points=%.2f  morale %.2f -> %.2f / %.2f  targetDevMode=%d\n",
			isHeal ? "heal" : "dmg",
			Instigator, Target, magnitude, points,
			before, after, Instigator->r_fRequiredMoralePoints,
			iTargetDeviceModeId);
	}
}
