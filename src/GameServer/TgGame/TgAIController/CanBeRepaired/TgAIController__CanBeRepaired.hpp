#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Diagnostic hook on `TgAIController::CanBeRepaired` (0x10a7ed90).
//
// Backs DB AI test_type_value_id 723 (AI_TEST_TARGET_CAN_BE_REPAIRED), the
// discriminator between assassin DRIVE BY HUMAN (test 723==0) and DRIVE BY
// ROBOT (test 723==1) actions. The native's body:
//   target = vtable[0x440](this);
//   return target && (target->r_nPhysicalType == 861 || target->r_nPhysicalType == 973);
// i.e. it checks the **target pawn's r_nPhysicalType** (offset 0x6d8) against
// two physical-type value_ids that classify "repairable" things (turret/robot).
//
// Ghidra mislabelled this address as `IsIdle`; the correct address was found
// manually by inspection of function bodies.
//
// Hook forces return=1 only for AI bots so we can confirm whether this test
// is actually the gate. Logs target ptr + r_nPhysicalType so we can see what
// it was evaluating against.
class TgAIController__CanBeRepaired : public HookBase<
	int(__fastcall*)(ATgAIController*, void*),
	0x10a7ed90,
	TgAIController__CanBeRepaired> {
public:
	static int __fastcall Call(ATgAIController* aic, void* edx);
	static inline int __fastcall CallOriginal(ATgAIController* aic, void* edx) {
		return m_original(aic, edx);
	};
};
