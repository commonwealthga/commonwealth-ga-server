// Logger channel: "armor"
#include "src/GameServer/Armor/Armor.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstring>
#include <map>

// TgPawn::SetProperty @ 0x109bf420 — writes UTgProperty.m_fRaw then runs
// ApplyProperty (vtable[0x8f4]) which fans the value into r_fMaxHealth and the
// other 6-ish HP storage sites (see reference_pawn_health_sync). Without
// calling this after raw writes, max-HP changes never reach the client.
static void SetPawnProperty(ATgPawn* Pawn, int nPropertyId, float fNewValue) {
	((void(__fastcall*)(ATgPawn*, void*, int, float))0x109bf420)(Pawn, nullptr, nPropertyId, fNewValue);
}

namespace {

// Per-pawn record of what the last ApplyDefaultArmor pass wrote to m_fRaw,
// keyed by propId. Reversed at the top of each subsequent call before the
// new pass runs, so respawning doesn't double-stack.
std::map<ATgPawn*, std::map<int, float>>& Deltas() {
	static std::map<ATgPawn*, std::map<int, float>> g;
	return g;
}

// Property ids consumed by armor.
constexpr int kPropHealthMax        = 304;  // base stat for max HP
constexpr int kPropHealthMaxModif   = 412;  // [n] mod — redirected to 304
constexpr int kPropHealthMod        = 390;  // Core mod — redirected to 304
constexpr int kPropProtectionRanged = 218;  // [r] mod — direct add

// Hardcoded loadout. Values picked to feel like a fully-modded epic set in
// classic GA tuning. Tune these constants and recompile to rebalance.
//
// Health Max stacking shape:
//   3 pieces × 6 [n] mods = 18 entries, each +N_PERC of base HP.
//   7 pieces × 1 Core mod =  7 entries, each +CORE_PERC of base HP.
//   Total HP multiplier ≈ 1 + (18·N_PERC + 7·CORE_PERC).
//   With defaults below: 1 + (18·0.005 + 7·0.10) = 1 + 0.79 = 1.79x base.
//
// Ranged Protection stacking shape:
//   4 pieces × 6 [r] mods = 24 entries, each +R_FLAT to prop 218 raw.
//   ~99.5% of devices have attack_rating=100, so UC CalcProtection's
//   fPercProtection = nProt/nDevRating treats raw 218 as percentage points.
//   24·0.5 = 12 → 12% Ranged mitigation layer (composes multiplicatively
//   with any other protection-layer entries; algebraically identical to
//   the classic spreadsheet's D5/D6 formula).
constexpr float kCorePerc = 0.10f;   // each piece's implicit +10% MaxHP
constexpr float kNPerc    = 0.005f;  // each [n] mod = +0.5% MaxHP
constexpr float kRFlat    = 0.5f;    // each [r] mod = +0.5 raw Ranged-Protection (= +0.5 pt)

constexpr int kPiecesR    = 4;
constexpr int kPiecesN    = 3;
constexpr int kModsPerPiece = 6;
constexpr int kTotalPieces  = 7;

bool IsHumanPlayer(ATgPawn* Pawn) {
	if (!Pawn || !Pawn->Controller || !Pawn->Controller->Class) return false;
	const char* cls = Pawn->Controller->Class->GetFullName();
	return cls && std::strstr(cls, "PlayerController") != nullptr;
}

UTgProperty* FindProp(ATgPawn* Pawn, int propId) {
	for (int i = 0; i < Pawn->s_Properties.Count; ++i) {
		UTgProperty* p = Pawn->s_Properties.Data[i];
		if (p && p->m_nPropertyId == propId) return p;
	}
	return nullptr;
}

}  // namespace

void Armor::ApplyDefaultArmor(ATgPawn* Pawn) {
	if (!Pawn || Pawn->s_Properties.Data == nullptr) return;
	if (!IsHumanPlayer(Pawn)) return;

	auto& perPawn = Deltas()[Pawn];

	// 1. Reverse anything we wrote last time so respawns don't stack.
	for (auto& kv : perPawn) {
		int   pid   = kv.first;
		float delta = kv.second;
		UTgProperty* prop = FindProp(Pawn, pid);
		if (!prop) continue;
		prop->m_fRaw -= delta;
		Logger::Log("armor",
			"[Armor/revert] pawn=%p propId=%d delta=%.3f -> raw=%.3f\n",
			(void*)Pawn, pid, delta, prop->m_fRaw);
	}
	perPawn.clear();

	// 2. HEALTH_MAX deltas. Both [n] mods (prop 412) and the implicit Core
	//    mods (prop 390) have no engine consumer, so both redirect to prop 304
	//    (HEALTH_MAX). Same pattern ReapplyCharacterSkillTree uses for 412.
	UTgProperty* propHP = FindProp(Pawn, kPropHealthMax);
	if (propHP) {
		const float baseHP = propHP->m_fBase;
		const float corePerc = kTotalPieces * kCorePerc;
		const float nPerc    = kPiecesN * kModsPerPiece * kNPerc;
		const float deltaHP  = baseHP * (corePerc + nPerc);
		propHP->m_fRaw += deltaHP;
		perPawn[kPropHealthMax] = deltaHP;
		Logger::Log("armor",
			"[Armor/apply] pawn=%p HEALTH_MAX(304): base=%.1f +core=%.0f%% +nMods=%.0f%% delta=%.1f raw=%.1f\n",
			(void*)Pawn, baseHP, corePerc*100.0f, nPerc*100.0f, deltaHP, propHP->m_fRaw);
	} else {
		Logger::Log("armor", "[Armor/apply] pawn=%p propId=304 missing — HP buff skipped\n", (void*)Pawn);
	}

	// 3. Ranged Protection (prop 218, calc-method 67 ADD on raw).
	UTgProperty* propR = FindProp(Pawn, kPropProtectionRanged);
	if (propR) {
		const float deltaR = kPiecesR * kModsPerPiece * kRFlat;
		propR->m_fRaw += deltaR;
		perPawn[kPropProtectionRanged] = deltaR;
		Logger::Log("armor",
			"[Armor/apply] pawn=%p PROTECTION_RANGED(218): +%d mods × %.3f = %.3f raw=%.3f\n",
			(void*)Pawn, kPiecesR * kModsPerPiece, kRFlat, deltaR, propR->m_fRaw);
	} else {
		Logger::Log("armor", "[Armor/apply] pawn=%p propId=218 missing — Ranged Protection skipped\n", (void*)Pawn);
	}

	// 4. Fan out HEALTH_MAX so r_fMaxHealth and the cached fields update on
	//    the client. PROTECTION_RANGED is read straight off s_Properties.m_fRaw
	//    by CalcProtection so no fan-out needed there.
	if (perPawn.count(kPropHealthMax)) {
		UTgProperty* p = FindProp(Pawn, kPropHealthMax);
		if (p) SetPawnProperty(Pawn, kPropHealthMax, p->m_fRaw);
	}
}
