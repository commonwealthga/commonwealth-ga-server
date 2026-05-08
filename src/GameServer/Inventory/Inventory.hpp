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
	//
	// `mods` is the list of rolled mod effect_group_ids stored in
	// ga_character_devices.mod_effect_group_ids — one entry per [letter] in
	// the item's UI suffix. Each effect_group's effects get applied to the
	// pawn's s_Properties at equip time so the modifiers actually affect
	// gameplay (not just the tooltip).
	static ATgDevice* Equip(ATgPawn* Pawn, int deviceId, int slot, int quality = 0,
	                        int inventoryId = 0, const std::vector<int>& mods = {});

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

	// Apply the device's permanent (lifetime_sec=0) equip-effect groups to the
	// pawn's properties. Reimplements what UC `ApplyEquipEffects` would do if
	// the asm.dat → device->m_EquipEffect setter (a stripped native) were
	// running. Source for the "30% physical protection by default" baseline:
	// device 864 ("HUMAN BASE ATTRIBUTES", slot 14) → effect group 3575 →
	// prop 155 +30 cm=67.
	//
	// Called automatically by Equip(); call manually after CreateEquipDevice
	// in bot-spawn paths that bypass Equip().
	static void ApplyDeviceEquipEffects(ATgPawn* Pawn, int deviceId);

	// Apply each effect attached to the listed effect_group_ids to the pawn's
	// s_Properties (calc-method semantics same as ApplyDeviceEquipEffects).
	// Used for rolled mods carried on UTgInventoryObject::m_nStateEffectGroupIdArray.
	// If a target property is missing on the pawn, it is initialized on demand
	// (m_fBase=1.0 so percent-style modifiers — calc method 68/69 — multiply
	// correctly; m_fRaw=0 so additive starts at zero; m_fMaximum=10000 to
	// avoid clipping). Logs the resulting m_fRaw per (prop, device).
	static void ApplyRolledModEffects(ATgPawn* Pawn, int deviceId, const std::vector<int>& effectGroupIds);

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
