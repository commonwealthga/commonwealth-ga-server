#pragma once

#include "src/pch.hpp"
#include <vector>
#include <map>

struct EquippedEntry {
	int deviceId;
	int slot;          // equip point (1-24)
	int quality;       // qualityValueId (0 = none)
	int inventoryId;   // unique ID assigned by Inventory
	int effectGroupId; // hardcoded effect group ID for inventory marshals
};

class Inventory {
public:
	// Equip a device on a pawn. Does NOT call UpdateClientDevices.
	// Call Finalize() after all equips to trigger replication.
	// Returns ATgDevice* (nullptr on failure).
	static ATgDevice* Equip(ATgPawn* Pawn, int deviceId, int slot, int quality = 0);

	// Trigger replication after all equips are done.
	// Calls UpdateClientDevices + sets bNetDirty/bForceNetUpdate on pawn and PRI.
	static void Finalize(ATgPawn* Pawn);

	// Query equipped devices for a pawn (for TcpSession inventory marshals).
	// Returns const ref to the vector (empty vector if pawn not tracked).
	static const std::vector<EquippedEntry>& GetEquipped(ATgPawn* Pawn);

	// Query equipped devices by pawn ID (for inventory marshals that only have pawnId).
	// Returns const ref to the vector (empty vector if pawnId not tracked).
	static const std::vector<EquippedEntry>& GetEquippedByPawnId(int pawnId);

	// Advance and return the next unique inventory ID. Single source of truth.
	static int NextId();

	// Clear tracking for a pawn (call on pawn death/destroy).
	static void ClearTracking(ATgPawn* Pawn);

private:
	static int s_nextInventoryId;  // starts at 10000
	static std::map<ATgPawn*, std::vector<EquippedEntry>> s_equipped;
	static std::map<int, std::vector<EquippedEntry>*> s_equippedByPawnId;  // pawnId -> ptr into s_equipped
	static std::vector<EquippedEntry> s_empty;  // returned by GetEquipped when pawn not found

	// Auto-detect device metadata from equip slot
	static int GetDeviceType(int slot);
	static bool IsOffHand(int slot);
	static bool IsHandDevice(int slot);

	// Hardcoded effect group ID lookup by device ID.
	// Replace with DB lookup when device->effect_group relation is added to the database.
	static int GetEffectGroupId(int deviceId);
};
