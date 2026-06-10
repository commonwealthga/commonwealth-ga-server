#include "src/GameServer/TgGame/TgDeviceFire/InitializeProjectile/TgDeviceFire__InitializeProjectile.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

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

	// Projectile-state diagnostic at throw time (after the UC ProjectileFire
	// field stamps + native InitFromFireMode). Spider grenades (TgProj_Bot) fly
	// invisible on the client while mines / normal grenades render — dumping
	// EVERY TgProj_* lets a single capture diff the working classes against the
	// broken one (bNetTemporary anomaly, rep flags, ids).
	if (Logger::IsChannelEnabled("pet_spawn") &&
	    ObjectClassCache::ClassNameContains(Projectile, "TgProj")) {
		ATgProjectile* proj = (ATgProjectile*)Projectile;
		const std::string projCls(ObjectClassCache::GetClassName(Projectile));
		Logger::Log("pet_spawn",
			"[Proj init] class=%s proj=0x%p mode=%d  r_nProjectileId=%d r_nOwnerFireModeId=%d "
			"s_OwnerFireMode=0x%p s_LastDefaultMode=0x%p r_Owner=0x%p s_nSpawnBotId=%d\n"
			"               loc=(%.1f,%.1f,%.1f) vel=(%.1f,%.1f,%.1f) speed=%.1f physics=%d\n"
			"               Role=%d RemoteRole=%d bNetTemporary=%d bUpdateSimulatedPosition=%d "
			"bReplicateMovement=%d bHidden=%d LifeSpan=%.2f tossZ=%.1f Instigator=0x%p\n",
			projCls.c_str(),
			(void*)proj, DeviceFire->m_nId,
			proj->r_nProjectileId, proj->r_nOwnerFireModeId,
			(void*)proj->s_OwnerFireMode, (void*)proj->s_LastDefaultMode,
			(void*)proj->r_Owner, proj->s_nSpawnBotId,
			proj->Location.X, proj->Location.Y, proj->Location.Z,
			proj->Velocity.X, proj->Velocity.Y, proj->Velocity.Z,
			proj->Speed, (int)proj->Physics,
			(int)proj->Role, (int)proj->RemoteRole,
			(int)proj->bNetTemporary, (int)proj->bUpdateSimulatedPosition,
			(int)proj->bReplicateMovement, (int)proj->bHidden,
			proj->LifeSpan, proj->m_fTossZ, (void*)proj->Instigator);
	}
}
