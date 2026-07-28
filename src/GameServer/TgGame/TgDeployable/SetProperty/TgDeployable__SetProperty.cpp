#include "src/GameServer/TgGame/TgDeployable/SetProperty/TgDeployable__SetProperty.hpp"
#include "src/GameServer/Combat/SendCombatMessage/SendCombatMessage.hpp"
#include "src/GameServer/Constants/TgProperties.h"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/TgGame/_deployable_classify/DeployableClassify.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Broadcast a COMBAT_MESSAGE (0x9F) to every connected client. Each client's
// CGameClient::OnCombatMessage handler resolves the victim ID — sign-encoded
// per the wire format (negative = unique deployable id at DRI+0x1DC) — and
// calls AddDamageInfo on the resolved actor, surfacing the floating number.
//
// Why broadcast instead of sending to one connection: at the SetProperty
// mirror we don't know which player initiated the heal (the firing pawn isn't
// passed through the s_Properties → SetProperty path), so we let the client
// decide visibility from its own state (relevance / friendship / on-screen).
static void BroadcastDeployableCombatMessage(
	int16_t sourceId, int16_t victimId, int16_t amount,
	SendCombatMessage::Type msgType)
{
	return;
	UWorld* World = (UWorld*)Globals::Get().GWorld;
	if (!World || !World->NetDriver) return;
	// Fan out by walking the per-client PlayerController's Pawn — the marshal
	// has to ride the engine's native per-pawn send path (vtable[0x266]) and
	// each client's own pawn is the receiver that delivers the bunch.
	for (int i = 0; i < World->NetDriver->ClientConnections.Num(); ++i) {
		UNetConnection* Conn = World->NetDriver->ClientConnections.Data[i];
		if (!Conn || (uintptr_t)Conn < 0x10000) continue;
		APlayerController* PC = Conn->Actor;
		if (!PC || !PC->Pawn) continue;
		ATgPawn* recipPawn = (ATgPawn*)PC->Pawn;

		SendCombatMessage::CallRaw(
			recipPawn,
			(uint16_t)msgType,
			sourceId,
			/*nIdAssist=*/0,
			victimId,
			amount,
			/*nValueShield=*/0);
	}
}

void __fastcall TgDeployable__SetProperty::Call(ATgDeployable* D, void* edx, int nPropId, float fValue) {
	// Original: walks s_Properties for nPropId, writes fValue to m_fRaw,
	// dispatches ApplyProperty (handles props 8 + 278 only).
	CallOriginal(D, edx, nPropId, fValue);

	if (!D) return;

	// Mirror props the native ApplyProperty drops on the floor.
	switch (nPropId) {
		case GA_PROPERTY::TGPID_HEALTH: {
			int newHp = (int)fValue;
			int hpMax = D->r_DRI ? D->r_DRI->r_nHealthMaximum : 0;
			if (hpMax <= 0) hpMax = D->r_nHealth > 0 ? D->r_nHealth : newHp;
			if (newHp < 0)        newHp = 0;
			if (newHp > hpMax)    newHp = hpMax;

			int oldHp = D->r_nHealth;
			if (oldHp == newHp) break;

			D->r_nHealth        = newHp;
			D->bNetDirty        = 1;
			D->bForceNetUpdate  = 1;
			if (D->r_DRI) {
				D->r_DRI->r_nHealthCurrent = newHp;
				D->r_DRI->bNetDirty        = 1;
				D->r_DRI->bForceNetUpdate  = 1;
			}

			// Floating combat number. Skip the spawn-time write where oldHp is
			// the still-zero actor default — that's not a heal, it's the
			// initial r_nHealth seed from InitializeDefaultProps.
			if (oldHp > 0 && D->r_DRI) {
				int delta = newHp - oldHp;
				int amount = delta > 0 ? delta : -delta;
				SendCombatMessage::Type msgType = (delta > 0)
					? SendCombatMessage::Type::HEAL
					: SendCombatMessage::Type::DAMAGE;

				// Source-id attribution: the SetProperty path doesn't carry the
				// firing pawn through, so we emit anonymously (id=0). The
				// floating number still shows; only the "from <player>" side
				// info is missing. Better attribution would require capturing
				// the effect instigator one frame upstream.
				int16_t sourceId = 0;
				int16_t victimId = -(int16_t)D->r_DRI->r_nUniqueDeployableId;

				BroadcastDeployableCombatMessage(
					sourceId, victimId, (int16_t)amount, msgType);
			}

			// Fire UC TgDeployable.UpdateHealth so any UC-side hooks (FX,
			// destroyed-state transition at hp<=0) run as if the native
			// ApplyProperty had reached them.
			D->UpdateHealth(newHp);

			// UC TakeDamage destroys on the 0-crossing (TgDeployable.uc:1217-1228);
			// this mirror is the only other runtime writer that can land
			// r_nHealth at 0. Without the same destroy, the deployable survives
			// at 0 HP and TakeDamage's health<=0 early-out (uc:1211) makes it
			// permanently indestructible. Honor the same r_bTakeDamage gate
			// TakeDamage does — indestructible deployables (PvE checkpoint
			// beacons) must never be destroyed through the mirror either.
			bool destroyDispatched = false;
			if (D->r_bTakeDamage && oldHp > 0 && newHp <= 0 && !D->m_bInDestroyedState) {
				DeployableClassify::DispatchDestroyIt(D, 0);
				destroyDispatched = true;
			}

			Logger::Log("heal_tick",
				"[SetProperty mirror] deployable=0x%p id=%d  HEALTH %d -> %d  hpMax=%d  combatMsg=%s\n",
				D, D->r_nDeployableId, oldHp, newHp, hpMax,
				(oldHp > 0 ? (newHp > oldHp ? "HEAL" : "DAMAGE") : "(skipped — spawn)"));

			// Indestructible-beacon hunt: mirror the line onto the "beacon"
			// channel so the repro capture has the effect-pipeline health
			// writes in the same timeline as the deploy/pickup/TakeDamage logs.
			if (ObjectClassCache::ClassNameContains((UObject*)D, "TgDeploy_Beacon")) {
				Logger::Log("beacon",
					"SetProperty HEALTH beacon=0x%p %d -> %d hpMax=%d destroyDispatched=%d\n",
					D, oldHp, newHp, hpMax, destroyDispatched ? 1 : 0);
			}
			break;
		}

		// Prop 6 (DAMAGE_RADIUS) → r_fClientProximityRadius for timer bombs.
		//
		// The HUD "you are in a bomb's danger zone" warning gates on
		// `r_fClientProximityRadius > 0` (UC `CheckLocalPlayerWithinProximity`,
		// TgDeployable.uc:1110, line 1127 specifically). For MINES, that field
		// is populated by the native ApplyProperty's prop-8 mirror (×16 ft→uu);
		// for BOMBS — which carry no prop 8 on their device-mode rows, only
		// prop 6 (damage radius) — the field would otherwise stay 0 and the
		// HUD warning never appears.
		//
		// CRITICAL: we mirror prop 6 ONLY to r_fClientProximityRadius, NEVER to
		// s_fProximityRadius. The latter is the server-side "this is a mine"
		// discriminator in UC's Active.BeginState (TgDeployable.uc:1992) —
		// writing it on a bomb routes Active into the mine LifeSpan path and
		// the bomb expires without firing StartFire. See the FIELD DICHOTOMY
		// comment in TgProj_Deployable__SpawnDeployable.cpp for the full
		// rationale.
		//
		// IsTimerBomb gate keeps stations / turrets / mines untouched here —
		// their prop 6 (turret AoE) drives weapon-fire damage radius via
		// CalcInstantFire, not the HUD warning. Mirror runs both at spawn
		// (after InitializeDefaultProps seeds prop 6's base) AND on every
		// buffed write from ScaleTargetProperties (Wide Blast / AOE_RADIUS_
		// MODIFIER skill expansion of prop 6 via ConvertPropToPropList).
		case GA_PROPERTY::TGPID_DAMAGE_RADIUS: {
			if (DeployableClassify::IsTimerBomb(D->r_nDeployableId)) {
				const float radiusUU = fValue * 16.0f;  // ft→uu, matches native ApplyProperty(prop 8)
				D->r_fClientProximityRadius = radiusUU;
				D->bNetDirty        = 1;
				D->bForceNetUpdate  = 1;
				Logger::Log("inventory",
					"[SetProperty mirror] deployable=0x%p id=%d  DAMAGE_RADIUS %.2fft -> r_fClientProximityRadius=%.2fuu (bomb HUD)\n",
					D, D->r_nDeployableId, fValue, radiusUU);
			}
			break;
		}

		// Prop 8 (PROXIMITY_DISTANCE) → arms a proximity mine: set
		// s_fProximityRadius (opens the Touch fire-gate + mine LifeSpan branch)
		// and spawn the s_CollisionProxy sensor whose cylinder is the trigger
		// volume. Both stripped natives our spawn path never replaced. Only
		// mines carry prop 8 (bombs use prop 6); fValue>0 skips the identity
		// default. radius/height are feel-based knobs — tune to taste.
		case GA_PROPERTY::TGPID_PROXIMITY_DISTANCE: {
			if (fValue <= 0.0f) break;
			const float radiusUU = fValue * 16.0f;  // ft→uu
			const float heightUU = 80.0f;

			D->s_fProximityRadius = radiusUU;
			D->bNetDirty       = 1;
			D->bForceNetUpdate = 1;

			if (D->s_CollisionProxy == nullptr) {
				// Same proxy spawn as TgMissionObjective__RegisterSelf.
				ATgCollisionProxy* Proxy = (ATgCollisionProxy*)D->Spawn(
					ClassPreloader::GetClass("Class TgGame.TgCollisionProxy"),
					D, FName(), D->Location, D->Rotation, nullptr, 1);
				if (Proxy) {
					Proxy->SetOwner(D);
					Proxy->SetBase(D, FVector(0, 0, 0), nullptr, FName());
					UCylinderComponent* Cyl =
						(UCylinderComponent*)Proxy->CollisionComponent;
					if (Cyl) Cyl->SetCylinderSize(radiusUU, heightUU);
					D->s_CollisionProxy = Proxy;
				}
				Logger::Log("mine_prox",
					"[SetProperty] dep=0x%p id=%d  prox=%.2fft -> radius=%.0fuu proxy=0x%p\n",
					D, D->r_nDeployableId, fValue, radiusUU, (void*)D->s_CollisionProxy);
			} else if (D->s_CollisionProxy->CollisionComponent) {
				((UCylinderComponent*)D->s_CollisionProxy->CollisionComponent)
					->SetCylinderSize(radiusUU, heightUU);
			}
			break;
		}

		// Prop 7 (REMOTE_ACTIVATION_TIME) → s_fActivationTime always; plus
		// r_nTickingTime for timer bombs.
		//
		// The native ApplyProperty drops every prop except 8 / 278 on the floor;
		// our InitializeDefaultProps seeds s_fActivationTime once at spawn from
		// prop 7's base value, but every subsequent SetProperty(7, …) — most
		// notably the buffed write from ScaleTargetProperties (which folds e.g.
		// Short Fuses' -10% prop 349 REMOTE_TIME_MODIFIER via
		// GetBuffedProperty(BUFF_DEVICE, 7)) — silently drops because there's
		// no mirror. Result: UC `Active.BeginState` reads the stale unbuffed
		// s_fActivationTime → SetTimer fires at the base time → the player's
		// remote-time skill never takes effect. Mirror here so the buffed
		// value propagates.
		//
		// `r_nTickingTime` (replicated int seconds) drives the bomb's HUD
		// countdown via UC `CheckLocalPlayerWithinProximity` (early-returns at
		// `r_nTickingTime == 0`) and the "X seconds left" counter at
		// TgDeployable.uc:1518. UC never sets this field; the native that
		// would populate it from asm-data is stripped. Bombs only — mines
		// have no countdown HUD.
		case GA_PROPERTY::TGPID_REMOTE_ACTIVATION_TIME: {
			D->s_fActivationTime = fValue;
			D->bNetDirty        = 1;
			D->bForceNetUpdate  = 1;

			const bool isBomb = DeployableClassify::IsTimerBomb(D->r_nDeployableId);
			if (isBomb) {
				D->r_nTickingTime      = (int)fValue;
				D->c_fStartTickingTime = 0.0f;  // client repnotify will stamp WorldInfo.TimeSeconds
			}

			Logger::Log("inventory",
				"[SetProperty mirror] deployable=0x%p id=%d  REMOTE_ACTIVATION_TIME -> s_fActivationTime=%.3fs%s\n",
				D, D->r_nDeployableId, fValue,
				isBomb ? " (+r_nTickingTime for bomb HUD)" : "");
			break;
		}

		// Prop 150 (PERSIST_TIME) → s_fPersistTime. Same mirror rationale as
		// prop 7: InitializeDefaultProps seeds it from the base, but any
		// buffed SetProperty(150, …) wouldn't otherwise reach the field UC
		// reads in Active.BeginState.
		case GA_PROPERTY::TGPID_PERSIST_TIME: {
			D->s_fPersistTime  = fValue;
			D->bNetDirty       = 1;
			D->bForceNetUpdate = 1;
			Logger::Log("inventory",
				"[SetProperty mirror] deployable=0x%p id=%d  PERSIST_TIME -> s_fPersistTime=%.3fs\n",
				D, D->r_nDeployableId, fValue);
			break;
		}

		// Both 304 (TGPID_HEALTH_MAX, the pawn-side variant) and 339
		// (TGPID_HEALTH_MAX_DEPLOYABLES, the deployable-specific variant) land
		// here. Either flavour writes the same DRI field; the duplicate
		// handling is intentional so whichever effect-group rolls a max-HP
		// buff actually moves the bar regardless of which property id the DB
		// row chose.
		case GA_PROPERTY::TGPID_HEALTH_MAX:
		case GA_PROPERTY::TGPID_HEALTH_MAX_DEPLOYABLES: {
			int newMax = (int)fValue;
			if (newMax < 1) newMax = 1;

			// Read current HP from the replicated mirror — InitializeDefault-
			// Props seeds `r_DRI->r_nHealthCurrent` at spawn, but the actor's
			// own `r_nHealth` can remain at the CDO default (typically 0)
			// until UC's damage path writes it. Using r_DRI gives the
			// canonical current HP for the full-vs-damaged check below.
			int oldMax = D->r_DRI ? D->r_DRI->r_nHealthMaximum : 0;
			int curHp  = D->r_DRI ? D->r_DRI->r_nHealthCurrent : D->r_nHealth;

			// Spawn-window guard (CONFIRMED indestructible-beacon producer,
			// 2026-07-19 log): the base HEALTH_MAX write from Initialize-
			// DefaultProps lands BEFORE the DRI current is seeded, so DRI cur
			// is still 0 while the spawn path already seeded the actor's
			// r_nHealth. Trusting the DRI 0 here made the sync below zero the
			// seed → live 0-HP deployable → UC TakeDamage early-outs forever.
			// If the DRI says 0 but the actor is alive, the actor value is the
			// real current.
			if (curHp <= 0 && D->r_nHealth > 0) curHp = D->r_nHealth;

			if (D->r_DRI) {
				D->r_DRI->r_nHealthMaximum = newMax;
				D->r_DRI->bNetDirty        = 1;
				D->r_DRI->bForceNetUpdate  = 1;
			}
			D->bNetDirty        = 1;
			D->bForceNetUpdate  = 1;

			// Resolve where current HP should end up after the cap change:
			//
			//   • At-full (curHp == oldMax) → stay at full at the new cap.
			//     This is the standard "max-HP buff while at full → stay
			//     full" semantic AND the spawn-time fix: ScaleTargetProps
			//     applies HP_MAX buffs right after InitializeDefaultProps
			//     seeds current to base, so curHp == oldMax holds and we
			//     scale current up. Without this a freshly-spawned force
			//     field reads ~45% HP (its initial 1500 against the buffed
			//     3315 cap when Station Buff + Output Mod compound).
			//
			//   • Damaged (curHp > newMax after a debuff) → clamp down to
			//     the new ceiling — preserves the prior clamp behavior.
			//
			//   • Damaged & still below new cap → leave current alone;
			//     only the ceiling moves.
			int newCur = curHp;
			if (oldMax > 0 && curHp == oldMax) {
				newCur = newMax;
			} else if (curHp > newMax) {
				newCur = newMax;
			}

			if (newCur != curHp || D->r_nHealth != newCur) {
				D->r_nHealth = newCur;
				if (D->r_DRI) D->r_DRI->r_nHealthCurrent = newCur;
				D->UpdateHealth(newCur);
			}

			Logger::Log("heal_tick",
				"[SetProperty mirror] deployable=0x%p id=%d  prop=%d (HEALTH_MAX) %d -> %d  cur %d -> %d\n",
				D, D->r_nDeployableId, nPropId, oldMax, newMax, curHp, newCur);

			if (ObjectClassCache::ClassNameContains((UObject*)D, "TgDeploy_Beacon")) {
				Logger::Log("beacon",
					"SetProperty HEALTH_MAX beacon=0x%p max %d -> %d cur %d -> %d\n",
					D, oldMax, newMax, curHp, newCur);
			}
			break;
		}

		default:
			break;
	}
}
