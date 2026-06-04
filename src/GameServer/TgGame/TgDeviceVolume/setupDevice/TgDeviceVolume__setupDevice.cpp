#include "src/GameServer/TgGame/TgDeviceVolume/setupDevice/TgDeviceVolume__setupDevice.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {
	ATgDeviceVolume* g_domeVrHealPadVolume = nullptr;

	bool IsDomeVrHealPadVolume(ATgDeviceVolume* Volume) {
		if (!Volume) return false;
		if (Config::GetMapNameChar() != "Dome3_VR_Arena_P") return false;
		return Volume->m_nMapObjectId == 11041;
	}
}

ATgDeviceVolume* DomeVrHealPad::GetRegisteredVolume() {
	return g_domeVrHealPadVolume;
}

// TgDeviceVolume::setupDevice — stripped native (0x109aeec0, decompiles to
// `return 1;`). Reimplemented from the UC contract in TgDeviceVolume.uc.
//
// The volume's only member of interest is `s_DeviceFireMode` (a UTgDeviceFire
// pointer). PostBeginPlay reads `GetRefireTime()` off it to set up the pain
// timer; TimerPop → CausePainTo → ApplyHit reads `IsValidTarget` and
// `m_EffectGroups` off it to apply the hazard effect. There's no separate
// `s_Device` field — the device only exists to BE the fire mode's owner.
//
// What we do:
//   1. Pull s_nDeviceId / s_nTeamNumber / s_nTaskForce from MapObjectConfig
//      (per-placement overrides).
//   2. Spawn a base ATgDevice as an Actor via UC's Spawn — `Volume->Spawn(
//      TgDevice, Owner=Volume, …)`. This is the same shape SpawnPlayerCharacter
//      and SpawnBotPawn use; the engine sets the device's Outer to the Level
//      (proper UE3 hierarchy), Owner to the Volume, and registers it in the
//      world's actor list. No SetOuter dance, no Children-array surprises.
//   3. Set `device->r_nDeviceId` before invoking ApplyDeviceSetup. The native
//      reads it to look up the asm.dat row.
//   4. Trigger `TgDevice.ApplyDeviceSetup` (intact native at 0x10a1dab0) via
//      ProcessEvent. It internally calls InstantiateDeviceFire (0x10a26500)
//      which: spawns one UTgDeviceFire per device_mode row, populates each
//      fire mode's m_Properties + m_EffectGroups + m_nDamageType + m_Owner
//      backreference, and stores them in `device->m_FireMode`. Identical
//      mechanism used by TgGame__SpawnBotById::BuildDeviceFireProperties.
//   5. Take `device->m_FireMode[0]` as the volume's fire mode. UC code in
//      ApplyHit accesses `m_Owner` (= the device), `m_Owner.Instigator`
//      (null for us — Spawn from a Volume doesn't set Instigator; UC's
//      TgPawn() cast handles null safely), and `GetEffectGroup(264, …)`
//      (resolved from the populated m_EffectGroups).
//
// FunctionFlags trick: the SDK ApplyDeviceSetup wrapper trips a
// `FunctionFlags |= ~0x400` corruption bug, so we ProcessEvent it manually
// with a save / force-FUNC_Native / restore. Same idiom used by
// NonPersistAddDevice and SpawnBotById::BuildDeviceFireProperties.
static int InvokeApplyDeviceSetup(ATgDevice* Device) {
	if (!Device) return -1;
	static UFunction* pFn = nullptr;
	if (!pFn) {
		pFn = (UFunction*)ObjectCache::Find("Function TgGame.TgDevice.ApplyDeviceSetup");
	}
	if (!pFn) return -1;
	const uint32_t origFlags = pFn->FunctionFlags;
	pFn->FunctionFlags = origFlags | 0x400;  // force FUNC_Native bit
	struct { uint32_t ReturnValue; } parms = { 0 };
	Device->ProcessEvent(pFn, &parms, nullptr);
	pFn->FunctionFlags = origFlags;
	return (int)parms.ReturnValue;
}

bool __fastcall TgDeviceVolume_setupDevice::Call(ATgDeviceVolume* Volume, void* edx) {
	Volume->s_nDeviceId   = MapObjectConfig::GetInt(Volume->m_nMapObjectId, "s_n_device_id",   Volume->s_nDeviceId);
	Volume->s_nTeamNumber = MapObjectConfig::GetInt(Volume->m_nMapObjectId, "s_n_team_number", Volume->s_nTeamNumber);
	Volume->s_nTaskForce  = MapObjectConfig::GetInt(Volume->m_nMapObjectId, "s_n_task_force",  Volume->s_nTaskForce);

	Logger::Log("device-volume",
		"[setupDevice] enter volume=0x%p mapObj=%d deviceId=%d team=%d tf=%d loc=(%.1f, %.1f, %.1f)\n",
		Volume, Volume->m_nMapObjectId, Volume->s_nDeviceId,
		Volume->s_nTeamNumber, (int)Volume->s_nTaskForce,
		Volume->Location.X, Volume->Location.Y, Volume->Location.Z);

	if (Volume->s_nDeviceId == 0) {
		Logger::Log("device-volume",
			"[setupDevice] volume=0x%p: no s_nDeviceId; no fire mode wired\n", Volume);
		return false;
	}

	UClass* DeviceClass = ClassPreloader::GetClass("Class TgGame.TgDevice");
	if (!DeviceClass) {
		Logger::Log("device-volume",
			"[setupDevice] volume=0x%p: TgDevice class not preloaded; aborting\n", Volume);
		return false;
	}

	// UC Spawn: engine wires Outer=Level + Owner=Volume + registers in world
	// actor list. bNoCollisionFail=1 because the device is a logical object;
	// its placement doesn't matter (the volume itself owns the collision
	// shape that triggers TimerPop). Location/Rotation copied from the
	// Volume for cleanliness — they're not read by anything downstream.
	ATgDevice* Device = (ATgDevice*)Volume->Spawn(
		DeviceClass, Volume, FName(),
		Volume->Location, Volume->Rotation,
		/*Template*/ nullptr,
		/*bNoCollisionFail*/ 1);
	if (!Device) {
		Logger::Log("device-volume",
			"[setupDevice] volume=0x%p deviceId=%d: Spawn(TgDevice) returned null\n",
			Volume, Volume->s_nDeviceId);
		return false;
	}

	// Must be set BEFORE ApplyDeviceSetup — the native reads it to find the
	// asm row and drive InstantiateDeviceFire.
	Device->r_nDeviceId = Volume->s_nDeviceId;

	const int rc = InvokeApplyDeviceSetup(Device);
	Logger::Log("device-volume",
		"[setupDevice] volume=0x%p device=0x%p ApplyDeviceSetup ret=%d m_FireMode.Num=%d\n",
		Volume, Device, rc,
		Device->m_FireMode.Data ? Device->m_FireMode.Num() : -1);

	if (Device->m_FireMode.Num() == 0 || Device->m_FireMode.Data[0] == nullptr) {
		Logger::Log("device-volume",
			"[setupDevice] volume=0x%p deviceId=%d: m_FireMode empty after ApplyDeviceSetup\n",
			Volume, Volume->s_nDeviceId);
		return false;
	}

	Volume->s_DeviceFireMode = Device->m_FireMode.Data[0];
	if (IsDomeVrHealPadVolume(Volume)) {
		g_domeVrHealPadVolume = Volume;
		Logger::Log("device-volume",
			"[setupDevice] registered Dome VR heal pad volume=0x%p fireMode=0x%p\n",
			Volume, Volume->s_DeviceFireMode);
	}

	Logger::Log("device-volume",
		"[setupDevice] OK volume=0x%p deviceId=%d -> fireMode=0x%p (refire=%.2fs)\n",
		Volume, Volume->s_nDeviceId, Volume->s_DeviceFireMode,
		Volume->s_DeviceFireMode->GetRefireTime());

	return true;
}
