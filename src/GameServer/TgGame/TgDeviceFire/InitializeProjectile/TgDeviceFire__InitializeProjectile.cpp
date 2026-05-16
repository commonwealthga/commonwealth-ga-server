#include "src/GameServer/TgGame/TgDeviceFire/InitializeProjectile/TgDeviceFire__InitializeProjectile.hpp"

void __fastcall TgDeviceFire__InitializeProjectile::Call(UTgDeviceFire* DeviceFire, void* /*edx*/, AActor* Projectile) {
	// Run original first so per-mode asm.dat config gets applied.
	CallOriginal(DeviceFire, nullptr, Projectile);

	if (!DeviceFire || !Projectile) return;

	ATgDevice* device = (ATgDevice*)DeviceFire->m_Owner;
	APawn* deviceInstigator = device ? device->Instigator : nullptr;

	// UE3's Spawn() iterator is supposed to do
	//   NewActor->Instigator = (this is APawn ? this : this->Instigator);
	// …on the spawning actor. In this binary that propagation is broken for
	// TgDevice-spawned projectiles — they come out with Instigator=null,
	// which breaks replicated team color on the client and the server-side
	// IsEnemy / damage-attribution checks (every hit no-ops because
	// projectile.Instigator->PlayerReplicationInfo->Team is null.null).
	//
	// Force-repair from the firing device's Instigator, plus Owner if
	// SpawnActor somehow didn't wire it either. Bump bNetDirty /
	// bForceNetUpdate because Projectile defaults bNetTemporary=true, so
	// there's only one replication window — the corrected Instigator has to
	// ride that window.
	if (!Projectile->Instigator && deviceInstigator) {
		Projectile->Instigator = deviceInstigator;
		if (!Projectile->Owner) {
			Projectile->Owner = device;
		}
		Projectile->bNetDirty = 1;
		Projectile->bForceNetUpdate = 1;
	}

	// NOTE: previously also rewrote Projectile->Location to the firing
	// shoulder for Boss Shrike. That was a band-aid — the projectile got
	// the right spawn point but the AI's aim direction (computed from body
	// center) still pointed parallel to "body→target", causing every shot
	// to miss to the side by the shoulder offset. The correct fix lives in
	// TgPawn__GetWeaponStartTraceLocationFromSocketOffsetInfo: by making
	// UC's GetWeaponStartTraceLocation return the shoulder, the AI's lead
	// prediction (GetAdjustedAim) recomputes from the shoulder and the
	// projectile spawns there with correct aim. No patch needed here.
}
