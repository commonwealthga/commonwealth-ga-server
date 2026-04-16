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
}
