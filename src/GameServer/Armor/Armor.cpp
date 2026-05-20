// Logger channel: "armor"
#include "src/GameServer/Armor/Armor.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cmath>
#include <cstring>
#include <map>

// TgPawn::SetProperty @ 0x109bf420 — writes UTgProperty.m_fRaw then runs
// ApplyProperty (vtable[0x8f4]) which fans the value into r_fMaxHealth and the
// other 6-ish HP storage sites (see reference_pawn_health_sync). The actual
// SetProperty call is now performed by ReapplyCharacterSkillTree's combined
// fanout pass at the very end — both armor and skills land in m_fRaw before
// the client gets the replicated update, so the client sees the final value
// rather than the post-armor / post-skill intermediates.
static void SetPawnProperty(ATgPawn* Pawn, int nPropertyId, float fNewValue) {
	((void(__fastcall*)(ATgPawn*, void*, int, float))0x109bf420)(Pawn, nullptr, nPropertyId, fNewValue);
}

namespace {

// Per-property bookkeeping for a pawn's last ApplyDefaultArmor pass.
//   delta            — the value we added to m_fRaw last time
//   postApplyRaw     — what m_fRaw read as IMMEDIATELY after we wrote it
//
// On RevertDefaultArmor we only subtract `delta` if `prop->m_fRaw ==
// postApplyRaw`. Any mismatch (instance reload reset s_Properties to base,
// another buff added in between, GC clobber, …) tells us our delta is no
// longer in the raw value — reverting would push it below baseline. Skip
// the revert and let the next apply land fresh on top of whatever raw is.
//
// IMPORTANT: composition with other layers (e.g. skills) only works if the
// revert pass happens BEFORE any other layer mutates raw. Earlier we had
// revert+apply both inside ApplyDefaultArmor, called AFTER skill apply had
// shifted raw — the snapshot check failed, both layers silently re-stacked,
// and HP grew unbounded with every skill UI save / respec.
//
// The new contract: caller runs all reverts first (LIFO of apply order),
// then all applies (armor → skills per the orchestration in
// ReapplyCharacterSkillTree). RCST does one combined SetProperty fanout at
// the end.
struct PerProp {
	float delta;
	float postApplyRaw;
};

std::map<ATgPawn*, std::map<int, PerProp>>& Records() {
	static std::map<ATgPawn*, std::map<int, PerProp>> g;
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

// Identify a player pawn without depending on Pawn->Controller being wired.
// SpawnPlayerCharacter sets PRI before calling ReapplyCharacterSkillTree, but
// the engine wires Controller→Pawn asynchronously via Possess() afterward.
// On return-from-mission the controller can be null at our call site, which
// made the previous Controller-based check return false → armor was silently
// skipped. r_bIsBot is CPF_Net + always set at bot spawn time, and PRI is
// always present for both bots and players, so "has PRI AND is not bot" is
// the reliable signal.
bool IsHumanPlayer(ATgPawn* Pawn) {
	if (!Pawn) return false;
	if (!Pawn->PlayerReplicationInfo) return false;
	if (Pawn->r_bIsBot) return false;
	return true;
}

UTgProperty* FindProp(ATgPawn* Pawn, int propId) {
	for (int i = 0; i < Pawn->s_Properties.Count; ++i) {
		UTgProperty* p = Pawn->s_Properties.Data[i];
		if (p && p->m_nPropertyId == propId) return p;
	}
	return nullptr;
}

// Float-tolerant equality. The post-apply raw value we record and the raw
// value we read back next time should be bit-identical (no math happens to
// it between our writes if nothing else touches the property), but a small
// epsilon guards against FPU rounding noise if the engine re-loads the
// float via a value that round-trips through a double.
bool RawMatches(float current, float remembered) {
	return std::fabs(current - remembered) < 0.001f;
}

}  // namespace

void Armor::RevertDefaultArmor(ATgPawn* Pawn) {
	if (!Pawn || Pawn->s_Properties.Data == nullptr) return;
	if (!IsHumanPlayer(Pawn)) return;  // silent for bots — Apply logs the skip

	auto recIt = Records().find(Pawn);
	if (recIt == Records().end()) return;  // first-ever apply: nothing to revert
	auto& records = recIt->second;

	for (auto& kv : records) {
		const int propId = kv.first;
		const PerProp& prev = kv.second;
		UTgProperty* prop = FindProp(Pawn, propId);
		if (!prop) continue;

		const float rawBefore = prop->m_fRaw;
		if (RawMatches(rawBefore, prev.postApplyRaw)) {
			prop->m_fRaw -= prev.delta;
			Logger::Log("armor",
				"[Armor/revert] pawn=%p propId=%d removed %.3f -> raw=%.3f\n",
				(void*)Pawn, propId, prev.delta, prop->m_fRaw);
		} else {
			// Raw no longer matches what we left — instance reload, mission
			// travel, or a layer we don't track touched it. Don't subtract a
			// delta that isn't there.
			Logger::Log("armor",
				"[Armor/revert-skipped] pawn=%p propId=%d raw=%.3f != postApply=%.3f (delta %.3f), "
				"treating as reset\n",
				(void*)Pawn, propId, rawBefore, prev.postApplyRaw, prev.delta);
		}
	}
	records.clear();
}

void Armor::ApplyDefaultArmor(ATgPawn* Pawn) {
	if (!Pawn || Pawn->s_Properties.Data == nullptr) return;
	if (!IsHumanPlayer(Pawn)) {
		Logger::Log("armor",
			"[Armor] skipped pawn=%p (not a human player: PRI=%p r_bIsBot=%d)\n",
			(void*)Pawn,
			(void*)(Pawn ? Pawn->PlayerReplicationInfo : nullptr),
			(int)(Pawn ? Pawn->r_bIsBot : 0));
		return;
	}

	auto& records = Records()[Pawn];

	auto applyTo = [&](int propId, float newDelta) {
		UTgProperty* prop = FindProp(Pawn, propId);
		if (!prop) {
			Logger::Log("armor",
				"[Armor/apply] pawn=%p propId=%d missing in s_Properties — skipped\n",
				(void*)Pawn, propId);
			return;
		}

		prop->m_fRaw += newDelta;
		records[propId] = { newDelta, prop->m_fRaw };
		Logger::Log("armor",
			"[Armor/apply] pawn=%p propId=%d +%.3f -> raw=%.3f\n",
			(void*)Pawn, propId, newDelta, prop->m_fRaw);
	};

	// HEALTH_MAX. Both [n] mods (prop 412) and the implicit Core mods (prop
	// 390) have no engine consumer, so both redirect to prop 304. Compute
	// delta from m_fBase so we recover correct buff size even if raw was
	// reset between calls.
	UTgProperty* propHP = FindProp(Pawn, kPropHealthMax);
	if (propHP) {
		const float baseHP = propHP->m_fBase;
		const float corePerc = kTotalPieces * kCorePerc;
		const float nPerc    = kPiecesN * kModsPerPiece * kNPerc;
		const float deltaHP  = baseHP * (corePerc + nPerc);
		applyTo(kPropHealthMax, deltaHP);
	} else {
		Logger::Log("armor", "[Armor/apply] pawn=%p propId=304 missing — HP buff skipped\n", (void*)Pawn);
	}

	// Ranged Protection (prop 218, calc-method 67 ADD on raw).
	const float deltaR = kPiecesR * kModsPerPiece * kRFlat;
	applyTo(kPropProtectionRanged, deltaR);

	// Fanout is handled by RCST's combined SetProperty pass at the end of
	// ReapplyCharacterSkillTree (skill applies land in m_fRaw AFTER us under
	// the new apply order: armor → skills). RCST adds HEALTH_MAX to its
	// touched set unconditionally after this call so the replicated cap
	// (r_fMaxHealth + cached fields) still gets refreshed. 218 stays raw-only
	// — CalcProtection reads m_fRaw directly with no fan-out needed.
}
