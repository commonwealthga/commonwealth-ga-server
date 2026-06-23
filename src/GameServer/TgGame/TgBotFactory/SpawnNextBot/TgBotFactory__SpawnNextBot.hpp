#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__SpawnNextBot : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*),
	0x10a8c2f0,
	TgBotFactory__SpawnNextBot> {
public:
	static std::map<int, ATgPawn*> m_lastSpawnedBot;

	// Per-factory escort tracking for the Raid_DomeCityDefense_P juggernaut
	// spawn table (149): repair drones spawn ahead of the juggernaut (last
	// group) and we adopt them as its followers when it spawns. We keep the
	// drone's r_nPawnId alongside the pointer so a drone that died in the gap
	// is rejected before deref. The list lives one batch only — ClearEscort is
	// called from ResetQueue (queue rebuild) so pointers never cross waves.
	struct EscortDroneRef { int pawnId; ATgPawn* pawn; };
	static std::map<int, std::vector<EscortDroneRef>> m_escortDrones;
	static void ClearEscort(int mid);

	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx) {
		m_original(BotFactory, edx);
	};
};

