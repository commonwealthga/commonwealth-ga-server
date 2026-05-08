#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UTgDeviceFire::IsValidTarget — vtable slot 103 of UTgDeviceFire (vtable @
// 0x116f03b8 + 0x19c = 0x116f0554 → function 0x10a28280).
//
// Final filter in the hitscan path.  After `CheckTeamPassThrough` decides to
// register a hit, GetTraceImpact (0x10a1d180) calls this on the hit actor.
// If it returns FALSE, the hit is discarded (HitActor cleared to null) and
// the trace effectively "missed".  If TRUE, the hit propagates back to UC's
// `CalcInstantFire` → `ProcessInstantHit` → `m_FireMode.ApplyHit(...)` which
// applies damage / repair / effects.
//
// Decompile shows multi-stage filtering:
//   1. class-not-allowed casts (DAT_119a407c, DAT_119a3adc) → return false
//   2. class-immediately-valid casts (FUN_1097dc60, FUN_1097dc30) → return true
//   3. melee-vs-deployable (Cast<TgDeploy_ForceField> + IsMeleeAttack vtable[0x1bc])
//      → return false
//   4. must be Pawn or TgDeployable, else return false
//   5. m_nTargetAffectsType (FireMode+0x9c) check — if > 0, must equal
//      target->r_nPhysicalType (TgDeployable+0x1d8 / TgPawn+0x6d8) OR target
//      type == 0x3cd (TG_PHYSICALITY_CYBORG wildcard) else return false
//   6. switch on m_eTargeterType (FireMode+0x42):
//      case 2,5 (Friend/Friend_Only): return !IsEnemy(target)
//      case 3   (Enemy):              return  IsEnemy(target)
//      case 4   (Enemy_And_Self):     return  IsEnemy(target) || IsSelfOrOwner
//      case 6   (All):                return true
//      default:                       return false
//
// Diagnostic-only hook.  Logs every call where target is a TgDeployable, so
// we can pinpoint exactly which filter rejects the hit.  Channel:
// `combat-trace`.
class TgDeviceFire__IsValidTarget : public HookBase<
	bool(__fastcall*)(UTgDeviceFire*, void*, AActor*, char),
	0x10a28280,
	TgDeviceFire__IsValidTarget> {
public:
	static bool __fastcall Call(UTgDeviceFire* FireMode, void* edx, AActor* TargetActor, char OverrideTargeterType);
	static inline bool __fastcall CallOriginal(UTgDeviceFire* FireMode, void* edx, AActor* TargetActor, char OverrideTargeterType) {
		return m_original(FireMode, edx, TargetActor, OverrideTargeterType);
	}
};
