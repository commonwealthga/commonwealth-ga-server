#include "src/GameServer/TgGame/TgDeviceVolume/setupDevice/TgDeviceVolume__setupDevice.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"

#include <string>

namespace {
	ATgDeviceVolume* g_domeVrHealPadVolume = nullptr;
	// Stashed at registration — UC PostBeginPlay nulls the volume's
	// s_DeviceFireMode when the stripped exec thunk misreports our return.
	UTgDeviceFire*   g_padFireMode         = nullptr;
	uint64_t         g_padRegisteredAtMs   = 0;
	bool             g_padArmChecked       = false;
	// egIds of the kept (instant-heal) groups, recorded by the ArmCheck prune.
	int              g_padHealEgIds[4]     = {};
	int              g_padHealEgCount      = 0;

	bool IsDomeVrHealPadVolume(ATgDeviceVolume* Volume) {
		if (!Volume) return false;
		if (Config::GetMapNameChar() != "Dome3_VR_Arena_P") return false;
		return Volume->m_nMapObjectId == 11041;
	}
}

ATgDeviceVolume* DomeVrHealPad::GetRegisteredVolume() {
	return g_domeVrHealPadVolume;
}

bool DomeVrHealPad::IsPadFireMode(UTgDeviceFire* fm) {
	return fm && fm == g_padFireMode;
}

bool DomeVrHealPad::IsPadHealEffectGroup(int egId) {
	for (int i = 0; i < g_padHealEgCount; i++) {
		if (g_padHealEgIds[i] == egId) return true;
	}
	return false;
}

void DomeVrHealPad::TickArmCheck() {
	if (!g_domeVrHealPadVolume) return;
	if (g_padArmChecked) {
		ATgDeviceVolume* V = g_domeVrHealPadVolume;
		const uint64_t now = GetTickCount64();

		// (Server-driven pulse removed — logs/292 proved the engine's own
		// Timer→TimerPop dispatch works; the pulse was double-healing on top
		// of it. The engine PainTimer now runs at 1s via the m_fFireTime
		// override at registration.)

		// Diagnostic monitor (only while the "healpad" channel is on): the pad
		// is armed but IsValidTarget never fires — this shows whether pawns
		// ever enter the volume's Touching list, and whether they'd pass
		// TimerPop's bCanBeDamaged/!bStatic filter.
		static uint64_t lastMonitorMs = 0;
		if (!Logger::IsChannelEnabled("healpad") || now - lastMonitorMs < 3000) return;
		lastMonitorMs = now;
		ATgDeviceVolumeInfo* info = (ATgDeviceVolumeInfo*)V->PainTimer;
		Logger::Log("healpad",
			"[Monitor] touching=%d painTimer=0x%p painTimer.V=0x%p fireMode=0x%p bPainCausing=%d s_bDeviceActive=%d\n",
			V->Touching.Data ? V->Touching.Num() : -1,
			info, info ? info->V : nullptr,
			V->s_DeviceFireMode, (int)V->bPainCausing, (int)V->s_bDeviceActive);
		// Brush bounds — tests the "volume isn't where the visible pad is"
		// theory. Origin/BoxExtent are world-space.
		if (V->CollisionComponent) {
			const FBoxSphereBounds& b = V->CollisionComponent->Bounds;
			Logger::Log("healpad",
				"[Monitor]   bounds origin=(%.1f, %.1f, %.1f) extent=(%.1f, %.1f, %.1f) radius=%.1f\n",
				b.Origin.X, b.Origin.Y, b.Origin.Z,
				b.BoxExtent.X, b.BoxExtent.Y, b.BoxExtent.Z, b.SphereRadius);
		}
		// Player pawn positions vs the volume — shows how far the player is
		// from the brush while standing on the visible pad.
		for (auto& kv : GClientConnectionsData) {
			ATgPawn_Character* p = kv.second.Pawn;
			if (!p) continue;
			Logger::Log("healpad",
				"[Monitor]   pawn=0x%p loc=(%.1f, %.1f, %.1f)\n",
				p, p->Location.X, p->Location.Y, p->Location.Z);
		}
		// Touching list: statics are noise — count them, print everything else.
		if (V->Touching.Data) {
			int staticCount = 0;
			for (int i = 0; i < V->Touching.Num(); i++) {
				AActor* a = V->Touching.Data[i];
				if (!a) continue;
				const char* raw = a->Class ? a->Class->GetFullName() : nullptr;
				const std::string cls(raw ? raw : "<null>");
				if (cls.find("StaticMeshActor") != std::string::npos) { staticCount++; continue; }
				// Team data feeds the client's FX friend/foe tint — dump it to
				// chase the inverted heal-pulse colors (yellow self / green enemy).
				int tf = -1, teamIdx = -1;
				if (cls.find("TgPawn") != std::string::npos) {
					ATgPawn* tp = (ATgPawn*)a;
					ATgRepInfo_Player* pri = (ATgRepInfo_Player*)tp->PlayerReplicationInfo;
					if (pri) {
						tf = pri->r_TaskForce ? pri->r_TaskForce->r_nTaskForce : -1;
						teamIdx = pri->Team ? pri->Team->TeamIndex : -1;
					}
				}
				Logger::Log("healpad",
					"[Monitor]   touching[%d]=0x%p (%s) bCanBeDamaged=%d bStatic=%d tf=%d teamIdx=%d loc=(%.1f, %.1f, %.1f)\n",
					i, a, cls.c_str(), (int)a->bCanBeDamaged, (int)a->bStatic,
					tf, teamIdx,
					a->Location.X, a->Location.Y, a->Location.Z);
			}
			Logger::Log("healpad", "[Monitor]   (+%d StaticMeshActors)\n", staticCount);
		}
		return;
	}
	if (GetTickCount64() - g_padRegisteredAtMs < 5000) return;  // let PostBeginPlay settle
	g_padArmChecked = true;

	ATgDeviceVolume* V = g_domeVrHealPadVolume;
	Logger::Log("healpad",
		"[ArmCheck] fireMode=0x%p bPainCausing=%d s_bDeviceActive=%d painTimer=0x%p\n",
		V->s_DeviceFireMode, (int)V->bPainCausing, (int)V->s_bDeviceActive, V->PainTimer);

	// Build + prune the pad's effect groups: keep only the instant
	// (lifetime<=0) type-264 heal group. Device 5134's mode also carries two
	// lifetime>0 264 groups (HoT/buff) that register HUD buff icons on every
	// pulse and stack extra healing — the original pad showed neither
	// (logs/291 + screenshot). Call the hook's Call directly to force the
	// lazy list build (the SDK GetEffectGroup wrapper corrupts its out-param).
	if (g_padFireMode) {
		int idx = 0;
		TgDeviceFire__GetEffectGroup::Call(g_padFireMode, nullptr, 264, &idx);
		TArray<UTgEffectGroup*>& gl = g_padFireMode->s_EffectGroupList;
		if (gl.Data) {
			int kept = 0;
			for (int i = 0; i < gl.Num(); i++) {
				UTgEffectGroup* g = gl.Data[i];
				if (!g) continue;
				if (g->m_nType == 264 && g->m_fLifeTime <= 0.0f) {
					gl.Data[kept++] = g;
					if (g_padHealEgCount < 4)
						g_padHealEgIds[g_padHealEgCount++] = g->m_nEffectGroupId;
					// Pad heals +100/s (original rate); device 2064's Medical
					// Station pulse ships +154. Patch only this fire mode's
					// group copies — the DB row is shared with the real
					// Medical Station deployable.
					if (g->m_Effects.Data) {
						for (int e = 0; e < g->m_Effects.Num(); e++) {
							UTgEffect* eff = g->m_Effects.Data[e];
							if (eff && eff->m_fBase > 100.0f) {
								Logger::Log("healpad",
									"[ArmCheck] heal base %.0f -> 100 (egId=%d propId=%d)\n",
									eff->m_fBase, g->m_nEffectGroupId, eff->m_nPropertyId);
								eff->m_fBase = 100.0f;
							}
						}
					}
				} else {
					Logger::Log("healpad",
						"[ArmCheck] pruned egId=%d type=%d lifetime=%.1f\n",
						g->m_nEffectGroupId, g->m_nType, g->m_fLifeTime);
				}
			}
			gl.Count = kept;
		}
	}

	if (V->s_DeviceFireMode && V->bPainCausing && V->s_bDeviceActive && V->PainTimer) {
		return;  // UC PostBeginPlay armed it — nothing to do
	}

	// Redo TgDeviceVolume.uc PostBeginPlay's arming.
	V->s_DeviceFireMode    = g_padFireMode;
	V->bPainCausing        = 1;
	V->BACKUP_bPainCausing = 1;
	V->s_bDeviceActive     = 1;

	if (!V->PainTimer) {
		UClass* infoClass = ClassPreloader::GetClass("Class TgGame.TgDeviceVolumeInfo");
		if (!infoClass) {
			Logger::Log("healpad",
				"[ArmCheck] TgDeviceVolumeInfo class not resident; cannot spawn pain timer\n");
			return;
		}
		// The info's own PostBeginPlay sets a 1s looping timer and V=Owner —
		// matches this device's 1.00s refire, so no SetTimer call needed here.
		V->PainTimer = (AInfo*)V->Spawn(infoClass, V, FName(),
			V->Location, V->Rotation, nullptr, 1);
	}

	Logger::Log("healpad", "[ArmCheck] re-armed: fireMode=0x%p painTimer=0x%p\n",
		V->s_DeviceFireMode, V->PainTimer);
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

	// Mirror the VR pad's whole setupDevice trace onto "healpad" so a single
	// channel shows why arming failed (the generic logs stay on device-volume).
	const bool isPad = IsDomeVrHealPadVolume(Volume);
	if (isPad) {
		Logger::Log("healpad",
			"[setupDevice] enter mapObj=%d deviceId=%d team=%d tf=%d\n",
			Volume->m_nMapObjectId, Volume->s_nDeviceId,
			Volume->s_nTeamNumber, (int)Volume->s_nTaskForce);
	}

	if (Volume->s_nDeviceId == 0) {
		Logger::Log("device-volume",
			"[setupDevice] volume=0x%p: no s_nDeviceId; no fire mode wired\n", Volume);
		if (isPad) Logger::Log("healpad", "[setupDevice] FAIL: no s_nDeviceId\n");
		return false;
	}

	UClass* DeviceClass = ClassPreloader::GetClass("Class TgGame.TgDevice");
	if (!DeviceClass) {
		Logger::Log("device-volume",
			"[setupDevice] volume=0x%p: TgDevice class not preloaded; aborting\n", Volume);
		if (isPad) Logger::Log("healpad", "[setupDevice] FAIL: TgDevice class not preloaded\n");
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
		if (isPad) Logger::Log("healpad", "[setupDevice] FAIL: Spawn(TgDevice) null\n");
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
		if (isPad) {
			Logger::Log("healpad",
				"[setupDevice] FAIL: deviceId=%d ApplyDeviceSetup ret=%d m_FireMode.Num=%d\n",
				Volume->s_nDeviceId, rc,
				Device->m_FireMode.Data ? Device->m_FireMode.Num() : -1);
		}
		return false;
	}

	Volume->s_DeviceFireMode = Device->m_FireMode.Data[0];
	if (isPad) {
		g_domeVrHealPadVolume = Volume;
		g_padFireMode         = Volume->s_DeviceFireMode;
		g_padRegisteredAtMs   = GetTickCount64();
		g_padArmChecked       = false;
		g_padHealEgCount      = 0;
		// Hazard-volume semantics: the pad affects anyone standing on it,
		// like the lava/toxic volume devices which ship target="All"
		// (device_volumes.md). Device 5134 is a handheld heal gun whose mode
		// ships a Friend targeter — and the pad's spawned TgDevice has no
		// team/instigator, so IsValidTarget rejected every pawn (logs/290).
		// Force All and clear the physicality gate.
		g_padFireMode->m_eTargeterType      = 6;  // TGDTT All
		// Physicality gate: 860 = the player-character physical type (logs/296:
		// TgPawn_Character=860, pets=861, deployables differ). IsValidTarget
		// natively rejects mismatches, so drones/turrets/pets/deployables get
		// neither heal nor FX from the pad.
		g_padFireMode->m_nTargetAffectsType = 860;
		// Original pad cadence is +100 per second; 5134's gun mode fires every
		// 0.5s. UC PostBeginPlay reads GetRefireTime() (= m_fFireTime) right
		// after setupDevice returns to arm the engine PainTimer, so setting it
		// here slows the engine-driven TimerPop to 1s. (logs/292 proved the
		// engine Timer→TimerPop dispatch works — the only real defect was the
		// Friend-targeter rejection.)
		g_padFireMode->m_fFireTime = 1.0f;
		Logger::Log("device-volume",
			"[setupDevice] registered Dome VR heal pad volume=0x%p fireMode=0x%p\n",
			Volume, Volume->s_DeviceFireMode);
		Logger::Log("healpad",
			"[setupDevice] armed: volume=0x%p deviceId=%d fireMode=0x%p refire=%.2fs "
			"team=%d tf=%d collision=%d loc=(%.1f, %.1f, %.1f)\n",
			Volume, Volume->s_nDeviceId, Volume->s_DeviceFireMode,
			Volume->s_DeviceFireMode->GetRefireTime(),
			Volume->s_nTeamNumber, (int)Volume->s_nTaskForce,
			(int)Volume->bCollideActors,
			Volume->Location.X, Volume->Location.Y, Volume->Location.Z);
	}

	Logger::Log("device-volume",
		"[setupDevice] OK volume=0x%p deviceId=%d -> fireMode=0x%p (refire=%.2fs)\n",
		Volume, Volume->s_nDeviceId, Volume->s_DeviceFireMode,
		Volume->s_DeviceFireMode->GetRefireTime());

	return true;
}
