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
	//   - The -spawnfriend / -spawnenemy chat command (always — chat-spawned
	//     bots are dev-test and want to opt in to BBM × difficulty scaling
	//     regardless of team).
	// Player respawn, deployable pets, decoys, and any other spawn path that
	// doesn't set this stays at the default false → InitializeDefaultProps
	// leaves stats raw for them.
	static bool bPendingEnemyScaling;

	// Per-spawn override for the difficulty scalar term. 0.0 = "use map
	// default" (Config::GetDifficultyScalar()). Set + consumed alongside
	// bPendingEnemyScaling — currently the only writer is the
	// -spawnfriend / -spawnenemy chat command, where the user picks an
	// explicit difficulty token (low/medium/high/max/umax). Cleared on
	// every InitializeDefaultProps invocation, even if scaling was gated
	// off, so a leftover value can't leak into a later factory spawn.
	static float nPendingDifficultyScalarOverride;

	// Per-factory designer stat knob (ATgBotFactory.fBalance, default 1.0) —
	// multiplies into the enemy-scaling product. Set by SpawnBotById from the
	// caller-passed factory; consumed + reset to 1.0 every invocation.
	static float fPendingFactoryBalance;

	static void __fastcall Call(ATgPawn* Pawn, void* edx);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx) {
		m_original(Pawn, edx);
	};
};
