#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__TgFindPlayerStart : public HookBase<
	ANavigationPoint*(__fastcall*)(ATgGame*, void*, AController*, FString*),
	0x10a744b0,
	TgGame__TgFindPlayerStart> {
public:
	// Yaw of the start last chosen for each controller. SpawnPlayerCharacter
	// consumes it so the pawn faces out of the spawn room instead of keeping
	// the controller's stale rotation.
	static inline std::unordered_map<AController*, int> s_ChosenYaw;

	// Mission checkpoints. Order in which each start was first seen
	// enabled (0 = enabled at the first pick). PvE Kismet turns checkpoint
	// starts on as the team advances; the latest one wins the pick.
	static inline std::unordered_map<ATgTeamPlayerStart*, int> s_EnabledSeq;
	static inline int s_NextEnabledSeq = 1;

	// PvE mission / tutorial chapter (not PvP, hub or open world).
	static bool IsPvECheckpointMap(ATgGame* Game);

	// After a revive (wave / Release Now) in a PvE mission, send the
	// pawn through the team's deployed checkpoint beacon (the pad path).
	static void CheckpointRespawn(ATgGame* Game, AController* Controller, const char* why);
	// Dev shortcut (dev-kill touch op ToggleFactory n=<mapObjId>): what Kismet
	// SeqAct_Toggle 'Turn On' does to a TgBeaconFactory (s_bAutoSpawn=1 + SpawnObject).
	static bool DevToggleFactory(int mapObjId);

	// Open-zone respawn (matches the original game). TgPawn::TrackDeath notes the death spot + agent per
	// controller; the next pick for that controller takes the nearest beacon start point (TgStartPoint
	// on a portal network) whose AvailabilityQuestGroupId the agent has unlocked. Nothing unlocked /
	// no death noted (zone entry) -> the stock pick, unchanged.
	struct ZoneDeath { FVector loc; int64_t charId; };
	static inline std::unordered_map<AController*, ZoneDeath> s_ZoneDeaths;
	static void NoteDeath(ATgPawn* Pawn);
	static ANavigationPoint* PickZoneRespawn(ATgGame* Game, AController* Controller);
	// Revive paths keep the pawn; move it to the PickZoneRespawn point (no-op if already consumed).
	static void ZoneReviveMove(ATgGame* Game, AController* Controller, const char* why);

	static ANavigationPoint* __fastcall Call(ATgGame* Game, void* edx, AController* Controller, FString* IncomingName);
	static inline ANavigationPoint* __fastcall CallOriginal(ATgGame* Game, void* edx, AController* Controller, FString* IncomingName) {
		return m_original(Game, edx, Controller, IncomingName);
	};
};


