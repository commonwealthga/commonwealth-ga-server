#include "src/GameServer/TgGame/TgPlayerActions/FullHeal/FullHeal.hpp"

#include "src/Config/Config.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Constants/TgProperties.h"
#include <vector>
#include "src/Utils/Logger/Logger.hpp"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

namespace TgPlayerActions::FullHealCmd {

namespace {

constexpr uint64_t kCooldownMs = 15000;  // 15s per player

std::map<std::string, uint64_t> g_lastUseMs;

ATgPawn_Character* FindPawnBySessionGuid(const std::string& guid) {
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.SessionGuid == guid) {
			return kv.second.Pawn;
		}
	}
	return nullptr;
}

void AlertPlayer(ATgPawn_Character* pawn, const char* msg) {
	if (!pawn->Controller) return;
	const char* raw = pawn->Controller->Class ? pawn->Controller->Class->GetFullName() : nullptr;
	const std::string ctrlClass(raw ? raw : "");
	if (ctrlClass.find("PlayerController") == std::string::npos) return;

	APlayerController* PC = (APlayerController*)pawn->Controller;
	if (!PC->Player) return;
	UNetConnection* Connection = (UNetConnection*)PC->Player;

	// ASCII zero-extend widen (message text is ASCII-only).
	const std::wstring wmsg(msg, msg + std::strlen(msg));
	SendAlert::SendText(Connection, wmsg.c_str(), 1, 0, 3.0f);
}

} // namespace

void Execute(const std::string& session_guid) {
	// VR arena only (Dome3_VR_Arena_P + _reserved 1v1 instances).
	if (Config::GetMapNameChar().rfind("Dome3_VR_Arena_P", 0) != 0) {
		Logger::Log("chat-command",
			"[ChatCmd][DLL] fullheal guid=%s: blocked outside VR arena (map=%s)\n",
			session_guid.c_str(), Config::GetMapNameChar().c_str());
		return;
	}

	ATgPawn_Character* pawn = FindPawnBySessionGuid(session_guid);
	if (!pawn) {
		Logger::Log("chat-command", "[ChatCmd][DLL] fullheal guid=%s: no pawn\n",
			session_guid.c_str());
		return;
	}
	if (pawn->Health <= 0) {
		Logger::Log("chat-command", "[ChatCmd][DLL] fullheal guid=%s: pawn dead; ignored\n",
			session_guid.c_str());
		return;
	}

	const uint64_t now = GetTickCount64();
	auto it = g_lastUseMs.find(session_guid);
	if (it != g_lastUseMs.end() && now - it->second < kCooldownMs) {
		char buf[64];
		std::snprintf(buf, sizeof(buf), "Full heal on cooldown (%llus)",
			(unsigned long long)((kCooldownMs - (now - it->second) + 999) / 1000));
		AlertPlayer(pawn, buf);
		return;
	}
	g_lastUseMs[session_guid] = now;

	// Cleanse first: remove every timed debuff group (poison, venom, slow,
	// fire DoT, EMP stun, ...), self-inflicted included. Two selection rules:
	//  - !IsBuff(): TgEffectGroup.uc:836 — hard-debuff categories are false
	//    regardless of instigator; default categories are false when the
	//    instigator is an enemy.
	//  - CC posture: self-inflicted defaults (medic self-poison, recon
	//    self-EMP) pass IsBuff (instigator not an enemy), but anything that
	//    forces a crowd-control posture is a debuff.
	// Lifetime<=0 groups (skill-tree/armor passives, instant hits) are never
	// touched. RemoveEffectGroup is the intact native — reverses removable
	// effects and tears down HUD icons/FX.
	int cleansed = 0;
	bool resetPosture = false;
	ATgEffectManager* mgr = pawn->r_EffectManager;
	if (mgr && mgr->s_AppliedEffectGroups.Data) {
		auto isCcPosture = [](int p) { return (p >= 3 && p <= 14) || p == 24; };
		std::vector<UTgEffectGroup*> debuffs;
		for (int i = 0; i < mgr->s_AppliedEffectGroups.Num(); i++) {
			UTgEffectGroup* g = mgr->s_AppliedEffectGroups.Data[i];
			if (!g || g->m_fLifeTime <= 0.0f) continue;
			if (!g->IsBuff() || isCcPosture(g->m_nPosture)) debuffs.push_back(g);
		}
		for (UTgEffectGroup* g : debuffs) {
			const bool hadCcPosture = isCcPosture(g->m_nPosture);
			if (mgr->RemoveEffectGroup(g)) {
				cleansed++;
				if (hadCcPosture) resetPosture = true;
			}
		}
	}
	// End the lingering stun/disease animation: SetPosture writes replicated
	// r_ePosture (repnotify -> client OnPostureChange). 0 = TG_POSTURE_DEFAULT.
	if (resetPosture) pawn->eventSetPosture(0, 1.0f, false);

	const int oldHealth = pawn->Health;
	// Heal-only: restore current HP to the live maximum. Do NOT use
	// SyncPawnHealth::Apply here — it rebases property m_fBase and rewrites
	// HealthMax/r_nHealthMaximum, baking any temporary max-HP buff in
	// permanently. Spawn is the only place that full sync belongs.
	const int maxHp = pawn->r_nHealthMaximum;
	pawn->Health = maxHp;
	if (pawn->s_Properties.Data) {
		for (int i = 0; i < pawn->s_Properties.Num(); ++i) {
			UTgProperty* p = pawn->s_Properties.Data[i];
			if (p && p->m_nPropertyId == GA_PROPERTY::TGPID_HEALTH) {
				p->m_fRaw = (float)maxHp;
			}
		}
	}
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
	if (pri) {
		pri->r_nHealthCurrent = maxHp;
		pri->bNetDirty = 1;
		pri->eventUpdateHealth(maxHp, maxHp);
	}
	pawn->bNetDirty = 1;

	Logger::Log("chat-command",
		"[ChatCmd][DLL] fullheal guid=%s: %d -> %d (cleansed %d debuff groups%s)\n",
		session_guid.c_str(), oldHealth, pawn->Health, cleansed,
		resetPosture ? ", posture reset" : "");

	AlertPlayer(pawn, "Healed to full");
}

} // namespace TgPlayerActions::FullHealCmd
