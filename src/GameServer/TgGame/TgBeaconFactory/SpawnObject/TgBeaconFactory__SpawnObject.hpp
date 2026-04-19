#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBeaconFactory__SpawnObject : public HookBase<
	void(__fastcall*)(ATgBeaconFactory*, void*),
	0x10A8C260,
	TgBeaconFactory__SpawnObject> {
public:
	static void __fastcall* Call(ATgBeaconFactory* BeaconFactory, void* edx);
	static inline void __fastcall* CallOriginal(ATgBeaconFactory* BeaconFactory, void* edx) {
		m_original(BeaconFactory, edx);
	}

	// Set to true after first player connects. Before that, pre-spawn attempts
	// are deferred — otherwise the beacon DRI's replicated object refs (most
	// notably r_TaskforceInfo) serialize at initial channel open with NetGUIDs
	// the client's NetGUID table doesn't have yet, resolving to null
	// permanently.  See reference_netguid_resolution_race.md.
	static bool s_firstPlayerConnected;

	// Invoke the full spawn body now that a client exists.  Called from
	// MarshalChannel__NotifyControlMessage::HandlePlayerConnected once on
	// first connect, for each BeaconFactory that tried to spawn early.
	static void RunSpawn(ATgBeaconFactory* BeaconFactory);

	// Replay all deferred spawns that tried to run before any client existed.
	static void FlushPendingSpawns();
};

