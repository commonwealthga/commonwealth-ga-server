#include "src/GameServer/TgGame/TgPawn/SyncPawnHealth/SyncPawnHealth.hpp"
#include "src/GameServer/Constants/TgProperties.h"

void SyncPawnHealth::Apply(ATgPawn* pawn, int hp, int maxHp) {
	if (!pawn) return;

	const float fHp    = (float)hp;
	const float fMaxHp = (float)maxHp;

	// (1)(2)(3) Engine + Tg replicated max
	pawn->Health           = hp;
	pawn->HealthMax        = maxHp;
	pawn->r_nHealthMaximum = maxHp;

	// (4)(5) Property descriptors. Walk s_Properties because GetProperty's
	// TMap may not be populated. Intentionally do NOT touch m_fMaximum here
	// — InitializeDefaultProps sets it to hitPoints*10 to leave headroom for
	// skill/item buffs that push current/max HP above the base hit-point
	// value. ApplyProperty clamps current HP against both the UTgProperty
	// ceiling AND r_nHealthMaximum, so keeping UTgProperty.m_fMaximum high
	// is what lets r_nHealthMaximum itself grow above the base.
	if (pawn->s_Properties.Data) {
		for (int i = 0; i < pawn->s_Properties.Num(); ++i) {
			UTgProperty* p = pawn->s_Properties.Data[i];
			if (!p) continue;
			if (p->m_nPropertyId == GA_PROPERTY::TGPID_HEALTH) {
				p->m_fBase = fMaxHp;
				p->m_fRaw  = fHp;
			} else if (p->m_nPropertyId == GA_PROPERTY::TGPID_HEALTH_MAX) {
				p->m_fBase = fMaxHp;
				p->m_fRaw  = fMaxHp;
			}
		}
	}

	// (6)(7) PRI fan-out via the same UC event ApplyProperty would have
	// dispatched. Skips silently when PRI isn't wired yet (early in spawn).
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
	if (pri) {
		pri->eventUpdateHealth(hp, maxHp);
	}

	pawn->bNetDirty = 1;
}
