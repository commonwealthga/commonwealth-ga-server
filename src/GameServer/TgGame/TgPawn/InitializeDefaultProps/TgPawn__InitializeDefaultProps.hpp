#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__InitializeDefaultProps : public HookBase<
	void(__fastcall*)(ATgPawn*, void*),
	0x109BF400,
	TgPawn__InitializeDefaultProps> {
public:
	// SpawnBotById writes the spawning bot's id here right before the engine
	// invokes our hook (the native takes no args); we consume + clear it.
	// SpawnPlayerCharacter also writes this (it stores the player's profileId),
	// so nPendingBotId != 0 does NOT mean "this is a bot-factory spawn".
	static int nPendingBotId;

	// "This spawn should receive enemy difficulty scaling." Set by the two
	// enemy-spawn paths immediately before they invoke SpawnBotById:
	//   - TgBotFactory bot-spawn pipeline (only when a non-null factory is
	//     actually passed — fallback-resolved factories don't count).
	//   - TgMissionObjective_Bot__SpawnObjectiveBot (bosses / escort targets;
	//     no factory exists, but they still want the same HP+damage scaling).
	// Player respawn, deployable pets, decoys, and any other spawn path that
	// doesn't set this stays at the default false → InitializeDefaultProps
	// leaves stats raw for them.
	static bool bPendingEnemyScaling;

	static void __fastcall* Call(ATgPawn* Pawn, void* edx);
	static inline void __fastcall* CallOriginal(ATgPawn* Pawn, void* edx) {
		m_original(Pawn, edx);
	};
};
