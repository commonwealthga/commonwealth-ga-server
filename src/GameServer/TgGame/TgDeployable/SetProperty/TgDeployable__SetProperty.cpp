#include "src/GameServer/TgGame/TgDeployable/SetProperty/TgDeployable__SetProperty.hpp"
#include "src/GameServer/Combat/SendCombatMessage/SendCombatMessage.hpp"
#include "src/GameServer/Constants/TgProperties.h"
#include "src/GameServer/Globals.hpp"
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
	UWorld* World = (UWorld*)Globals::Get().GWorld;
	if (!World || !World->NetDriver) return;
	for (int i = 0; i < World->NetDriver->ClientConnections.Num(); ++i) {
		UNetConnection* Conn = World->NetDriver->ClientConnections.Data[i];
		if (!Conn || (uintptr_t)Conn < 0x10000) continue;
		SendCombatMessage::CallRaw(
			Conn,
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

			Logger::Log("heal_tick",
				"[SetProperty mirror] deployable=0x%p id=%d  HEALTH %d -> %d  hpMax=%d  combatMsg=%s\n",
				D, D->r_nDeployableId, oldHp, newHp, hpMax,
				(oldHp > 0 ? (newHp > oldHp ? "HEAL" : "DAMAGE") : "(skipped — spawn)"));
			break;
		}

		// Both 304 (TGPID_HEALTH_MAX, the pawn variant) and 339
		// (TGPID_HEALTH_MAX_DEPLOYABLES, the deployable-specific variant) land
		// here.  Either flavour ends up writing the same DRI field; the
		// duplicate handling is intentional so whichever effect-group rolls a
		// max-HP buff actually moves the bar regardless of which property id
		// the DB row chose.
		case GA_PROPERTY::TGPID_HEALTH_MAX:
		case GA_PROPERTY::TGPID_HEALTH_MAX_DEPLOYABLES: {
			int newMax = (int)fValue;
			if (newMax < 1) newMax = 1;

			int oldMax = D->r_DRI ? D->r_DRI->r_nHealthMaximum : 0;
			if (D->r_DRI) {
				D->r_DRI->r_nHealthMaximum = newMax;
				D->r_DRI->bNetDirty        = 1;
				D->r_DRI->bForceNetUpdate  = 1;
			}
			D->bNetDirty        = 1;
			D->bForceNetUpdate  = 1;

			// Re-clamp current HP against the new ceiling.
			if (D->r_nHealth > newMax) {
				D->r_nHealth = newMax;
				if (D->r_DRI) D->r_DRI->r_nHealthCurrent = newMax;
				D->UpdateHealth(newMax);
			}

			Logger::Log("heal_tick",
				"[SetProperty mirror] deployable=0x%p id=%d  prop=%d (HEALTH_MAX) %d -> %d\n",
				D, D->r_nDeployableId, nPropId, oldMax, newMax);
			break;
		}

		default:
			break;
	}
}
