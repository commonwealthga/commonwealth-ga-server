#pragma once
#include "src/pch.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstdio>

// The retail level-up feedback, driven from the server.
// TgPawn.FlashLevelupFx (native, vtable +0x558 = 0x109bf250) on the authority pushes
// flash event 0x310001 into the replicated r_nFlashEvent ring; the client's flash
// processor (0x109c5920) sees type 0x31 and calls TgPawn.OnLevelUp (vt +0x894 =
// 0x109c87a0): spawns c_LevelUpFX (PART_DEV_EFX_LevelUp ring + text) on the pawn and
// refreshes the HUD. Messages: 21310 "Congratulations!  You have reached level
// @@level@@!" as a toast to the agent, 21209 "... has reached level @@level@@!" as a
// chat line to the other players in the instance (the alert path has no LEVEL token,
// so both go as free text with the same wording).
namespace LevelUpFeedback {
	inline void Announce(ATgPawn_Character* Pawn, int level) {
		if (Pawn == nullptr || level <= 1) return;
		Pawn->FlashLevelupFx();
		const int hp0 = Pawn->Health; const float pw0 = Pawn->r_fCurrentPowerPool;  // Refill proof in the log
		// Original 1.5 notes: "Leveling up gives a full health + power
		// restore". Living pawn -> health + power pool to max (replicated fields).
		if (Pawn->Health > 0 && Pawn->r_nHealthMaximum > Pawn->Health) Pawn->Health = Pawn->r_nHealthMaximum;
		if (Pawn->Health > 0 && Pawn->r_fMaxPowerPool > Pawn->r_fCurrentPowerPool)
			Pawn->r_fCurrentPowerPool = Pawn->r_fMaxPowerPool;

		wchar_t self[96];
		swprintf(self, 96, L"Congratulations!  You have reached level %d!", level);
		APlayerReplicationInfo* Pri = Pawn->PlayerReplicationInfo;
		const wchar_t* name = (Pri && Pri->PlayerName.Data) ? Pri->PlayerName.Data : L"An agent";
		wchar_t other[160];
		swprintf(other, 160, L"%ls has reached level %d!", name, level);

		int others = 0;
		for (auto& kv : GClientConnectionsData) {
			if (kv.second.bClosed || kv.second.Pawn == nullptr) continue;
			UNetConnection* Conn = (UNetConnection*)(intptr_t)kv.first;
			if (kv.second.Pawn == Pawn) {
				SendAlert::SendTextAlert(Conn, self, /*priority*/ 2, /*type*/ 1, 5.0f);
			} else {
				SendAlert::SendText(Conn, other, 1, 1, 3.0f);
				others++;
			}
		}
		Logger::Log("loot", "[LevelUp] pawn=%d level=%d fx+toast, %d other(s) told; hp %d->%d power %.0f->%.0f\n",
			(int)Pawn->r_nPawnId, level, others, hp0, Pawn->Health, pw0, Pawn->r_fCurrentPowerPool);
	}
}
