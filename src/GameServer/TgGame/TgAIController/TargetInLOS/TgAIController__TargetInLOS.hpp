#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Hook on `TgAIController::TargetInLOS` (0x10a86100). Diagnostic-only — used
// to investigate the "owner blocks turret LOS" gameplay regression. The AI
// runs this every fire-tick during continuous fire; if it returns 1 the AI
// keeps firing through whatever's in between. Original game dropped the
// lock-on when the deploying player walked into the LOS path.
class TgAIController__TargetInLOS : public HookBase<
	int(__fastcall*)(ATgAIController*, void*),
	0x10a86100,
	TgAIController__TargetInLOS> {
public:
	static int __fastcall Call(ATgAIController* aic, void* edx);
	static inline int __fastcall CallOriginal(ATgAIController* aic, void* edx) {
		return m_original(aic, edx);
	};
};
