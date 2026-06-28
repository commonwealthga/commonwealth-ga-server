#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Hook on `TgAIController::TargetInLOS` (FUN_10a85c60).
// Fixes the multi-dome turret bug: ForceField domes are transparent to friendly
// fire (CheckTeamPassThrough), so they should also be transparent to LOS checks.
// We disable bCollideActors on all live domes before the trace runs so the
// recursive tracer (FUN_10a806e0) never hits them at all, then restore.
class TgAIController__TargetInLOS : public HookBase<
	int(__fastcall*)(ATgAIController*, void*),
	0x10a85c60,
	TgAIController__TargetInLOS> {
public:
	static int __fastcall Call(ATgAIController* aic, void* edx);
	static inline int __fastcall CallOriginal(ATgAIController* aic, void* edx) {
		return m_original(aic, edx);
	};
};
