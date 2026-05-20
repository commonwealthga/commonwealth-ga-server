#include "src/GameServer/TgGame/Morale/MoraleCredit.hpp"

#include <set>

#include "sqlite3.h"

#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MoraleCredit {

namespace {

// Base rates: HP magnitude required to earn 1 morale point at default
// threshold of 100, BEFORE Output Mod is applied. Retail playtest evidence:
//   27000 damage / 32000 heal = 100 morale, with the innate +70% Output Mod
// active on retail morale devices. Reversed: base × 1.70 ≈ retail-effective
// rate (within ~2%). Rounded to nice multiples of 90.
constexpr float kBaseDamagePerMoralePoint = 450.0f;
constexpr float kBaseHealPerMoralePoint   = 540.0f;

// Set of device_mode_ids whose parent device is `slot_used_value_id == 476`
// ("Morale Device" category). Damage sourced from one of these blocks morale
// credit — anti-feedback gate matching the binary's intact AddMoralePoints
// native (which does this lookup at runtime via the asm manager).
std::set<int> g_moraleDeviceModeIds;
bool g_inited = false;

void DoInit() {
	sqlite3* db = Database::GetConnection();
	if (!db) {
		Logger::Log("db",
			"[MoraleCredit] no DB connection — anti-feedback filter disabled\n");
		return;
	}

	const char* sql =
		"SELECT m.device_mode_id "
		"FROM asm_data_set_devices_data_set_device_modes m "
		"JOIN asm_data_set_devices d ON d.device_id = m.device_id "
		"WHERE d.slot_used_value_id = 476";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("db",
			"[MoraleCredit] prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		g_moraleDeviceModeIds.insert(sqlite3_column_int(stmt, 0));
	}
	sqlite3_finalize(stmt);

	Logger::Log("morale",
		"[MoraleCredit] indexed %d morale-category device_mode_ids (anti-feedback set)\n",
		(int)g_moraleDeviceModeIds.size());
}

inline bool IsMoraleSourceDeviceMode(int deviceModeId) {
	if (!g_inited) { DoInit(); g_inited = true; }
	return g_moraleDeviceModeIds.count(deviceModeId) > 0;
}

// Compute the multiplicative gain-rate factor for the recipient's morale
// device. Two contributors, both walked out of `m_EffectBuffInfo`:
//
//   prop 385 "Output Mod"                — innate device output multiplier.
//                                          Retail morale devices carry +70%;
//                                          maps directly to faster fill.
//
//   prop 357 "Required Morale Points Modifier" — skill/mod-kit threshold
//                                          reductions. Retail applied these
//                                          to the threshold (denominator);
//                                          we apply them inversely to the
//                                          gain rate (numerator). Same
//                                          observable fill time, no
//                                          GetBuffedProperty(BUFF_DEVICE,318)
//                                          explosion. Skills bound to prop
//                                          357: Team Boost Increase (550,
//                                          -25%), Super Healer (703, -10%),
//                                          Rapid Engineering (907, -10%),
//                                          Point Tank (912, -10%); plus
//                                          [m] mod-kit letters on the
//                                          morale device itself (-1% each).
//
// Both props' percent contributions go through the BUFF formula's three-
// layer multiplicative stack:
//   factor = (1 + ΣitemPct/100) × (1 + ΣskillPct/100) × (1 + (Σselfpct+Σpct)/100)
//
// For prop 357 the sum is INVERTED in sense: the registry stores positive
// percent values (calc_method=69 PERC_DECREASE has its sign flipped during
// ApplyBuff so the stored value is positive-but-treated-as-reduction). We
// flip them back to negative percent before feeding into the formula, then
// take the reciprocal of the resulting factor so a -25% threshold becomes
// a +33% speedup ((1/(1-0.25)) ≈ 1.333, matching retail's 20000 dmg-to-fill
// empirical).
float GetMoraleGainFactor(ATgPawn* recipient) {
	if (!recipient) return 1.0f;
	if (recipient->r_nMoraleDeviceSlot < 0)            return 1.0f;
	if (recipient->r_nMoraleDeviceSlot >= 0x19)        return 1.0f;
	ATgDevice* dev = recipient->m_EquippedDevices[recipient->r_nMoraleDeviceSlot];
	if (!dev) return 1.0f;
	const int moraleInvId = dev->r_nInventoryId;

	const int n = recipient->m_EffectBuffInfo.Num();
	FBuffInfo* data = recipient->m_EffectBuffInfo.Data;
	if (n <= 0 || data == nullptr) return 1.0f;

	// Output Mod sums (prop 385).
	float outItemP = 0.0f, outSkillP = 0.0f, outSelfP = 0.0f, outGenP = 0.0f;
	// Threshold-modifier sums (prop 357). ApplyBuff sign-flips cm=69
	// (PERC_DECREASE) entries during the slot write — Team Boost Increase
	// 25% becomes `+= -25`. So a negative sum means the threshold should
	// be reduced; we feed it directly into `(1 + sum/100)` and the layer
	// product is < 1.
	float reqItemP = 0.0f, reqSkillP = 0.0f, reqSelfP = 0.0f, reqGenP = 0.0f;
	for (int i = 0; i < n; ++i) {
		const FBuffInfo& bi = data[i];
		const int devInst = bi.BuffHeader.nReqDeviceInstId;
		if (devInst != 0 && devInst != moraleInvId) continue;
		if (bi.BuffHeader.nPropId == 385) {
			outItemP  += bi.fItemPercentModifier;
			outSkillP += bi.fSkillPercentModifier;
			outSelfP  += bi.fSelfPercentModifier;
			outGenP   += bi.fPercentModifier;
		} else if (bi.BuffHeader.nPropId == 357) {
			reqItemP  += bi.fItemPercentModifier;
			reqSkillP += bi.fSkillPercentModifier;
			reqSelfP  += bi.fSelfPercentModifier;
			reqGenP   += bi.fPercentModifier;
		}
	}

	const float outputModFactor =
		(1.0f + outItemP  / 100.0f) *
		(1.0f + outSkillP / 100.0f) *
		(1.0f + (outSelfP + outGenP) / 100.0f);

	// Threshold reduction → gain speedup. Layer sums for prop 357 are
	// already negative for reductions (per the ApplyBuff sign-flip), so the
	// natural layer formula `(1 + sum/100)` gives a factor < 1 — that's the
	// retail-style threshold scale. We want the equivalent gain-rate
	// speedup, which is the reciprocal: -25% threshold → 1/0.75 = 1.333.
	const float thresholdFactor =
		(1.0f + reqItemP  / 100.0f) *
		(1.0f + reqSkillP / 100.0f) *
		(1.0f + (reqSelfP + reqGenP) / 100.0f);
	const float speedupFactor = (thresholdFactor > 0.001f)
		? (1.0f / thresholdFactor)
		: 1.0f;

	return outputModFactor * speedupFactor;
}

}  // namespace

// One-shot diagnostic: dump every prop=385 (Output Mod) and prop=357
// (Required Morale Points Modifier) entry in the recipient's m_EffectBuffInfo
// the first time Award fires for a given pawn. Distinguishes:
//
//   Case A — duplicate-row accumulation: many entries, same (prop,cat,skill,
//            devInst), each carrying a small fPercentModifier (e.g. 70 each).
//            Indicates ApplyBuff's GetBuffIndex is failing to find existing
//            entries, so each registration appends a fresh row.
//
//   Case B — runaway single row: one entry with a huge fPercentModifier
//            (e.g. 31316). Indicates ApplyBuff is being called repeatedly
//            for the same buff without matching bRemove calls, additively
//            accumulating on the same slot.
//
// Both cases produce the same threshold explosion via the BuffFormula sum.
// One pass through the registry tells them apart.
static void DumpMoraleBuffEntriesOnce(ATgPawn* recipient) {
	if (!recipient) return;
	if (!Logger::IsChannelEnabled("morale")) return;
	static std::set<ATgPawn*> g_dumped;
	if (g_dumped.count(recipient) > 0) return;
	g_dumped.insert(recipient);

	const int n = recipient->m_EffectBuffInfo.Num();
	FBuffInfo* data = recipient->m_EffectBuffInfo.Data;

	int moraleInvId = -1;
	if (recipient->r_nMoraleDeviceSlot >= 0 &&
	    recipient->r_nMoraleDeviceSlot < 0x19) {
		ATgDevice* dev = recipient->m_EquippedDevices[recipient->r_nMoraleDeviceSlot];
		if (dev) moraleInvId = dev->r_nInventoryId;
	}

	Logger::Log("morale",
		"[BuffDump] recipient=0x%p total m_EffectBuffInfo entries=%d  moraleInvId=%d\n",
		recipient, n, moraleInvId);

	if (n <= 0 || data == nullptr) return;

	int seen385 = 0, seen357 = 0;
	for (int i = 0; i < n; ++i) {
		const FBuffInfo& bi = data[i];
		const int prop = bi.BuffHeader.nPropId;
		if (prop != 385 && prop != 357) continue;
		const bool scopeMatch = (bi.BuffHeader.nReqDeviceInstId == 0 ||
		                        bi.BuffHeader.nReqDeviceInstId == moraleInvId);
		Logger::Log("morale",
			"[BuffDump]   idx=%d prop=%d cat=%d skill=%d devInst=%d scopeMatch=%d  "
			"SP=%.3f SM=%.3f IP=%.3f IM=%.3f selfP=%.3f selfM=%.3f genP=%.3f genM=%.3f\n",
			i, prop, bi.BuffHeader.nReqCategoryCode,
			bi.BuffHeader.nReqSkillId, bi.BuffHeader.nReqDeviceInstId,
			scopeMatch ? 1 : 0,
			bi.fSkillPercentModifier, bi.fSkillModifier,
			bi.fItemPercentModifier,  bi.fItemModifier,
			bi.fSelfPercentModifier,  bi.fSelfModifier,
			bi.fPercentModifier,      bi.fModifier);
		if (prop == 385) ++seen385;
		else             ++seen357;
	}
	Logger::Log("morale",
		"[BuffDump] summary: prop385 entries=%d  prop357 entries=%d\n",
		seen385, seen357);
}

void Award(ATgPawn* recipient, float magnitude, bool isHeal,
           float fMissingHealth, int deviceModeId) {
	if (!recipient) return;
	if (magnitude <= 0.0f) return;

	DumpMoraleBuffEntriesOnce(recipient);

	// Anti-feedback: morale-device sources don't credit (catches both
	// direct morale-fire damage and bomb-spawned-by-morale damage —
	// retail registers both under category 476).
	if (IsMoraleSourceDeviceMode(deviceModeId)) {
		if (Logger::IsChannelEnabled("morale")) {
			Logger::Log("morale",
				"[MoraleCredit] morale-device source (no credit): recipient=0x%p devModeId=%d mag=%.2f isHeal=%d\n",
				recipient, deviceModeId, magnitude, (int)isHeal);
		}
		return;
	}

	// Recipient gates (mirrors the surviving gates of the binary's
	// AddMoralePoints native — we don't call the native because it AVs
	// on unknown device-mode ids in our environment).
	if (recipient->r_nMoraleDeviceSlot < 0) return;  // no morale device equipped
	const uint32_t allowBits = *(uint32_t*)((char*)recipient + 0x3D8);
	if ((allowBits & 0x10000000u) == 0) return;      // r_bAllowAddMoralePoints

	// Overheal clamp (heal-only). Matches the STYPE_HEALING credit contract
	// in TgPawn__TrackHealing: only the actually-restored HP counts.
	float creditMagnitude = magnitude;
	if (isHeal) {
		float cap = fMissingHealth > 0.0f ? fMissingHealth : 0.0f;
		if (creditMagnitude > cap) creditMagnitude = cap;
		if (creditMagnitude <= 0.0f) {
			if (Logger::IsChannelEnabled("morale")) {
				Logger::Log("morale",
					"[MoraleCredit] heal overheal no credit: recipient=0x%p attempt=%.2f missingHealth=%.2f\n",
					recipient, magnitude, fMissingHealth);
			}
			return;
		}
	}

	// Rate: morale points per HP, scaled by combined morale-device factor
	// (Output Mod prop 385 + skill/mod threshold-reduction prop 357,
	// applied here as gain-rate speedup so threshold can stay at 100).
	const float gainFactor = GetMoraleGainFactor(recipient);
	const float basePerPoint = isHeal ? kBaseHealPerMoralePoint
	                                  : kBaseDamagePerMoralePoint;
	const float points = (creditMagnitude / basePerPoint) * gainFactor;
	if (points <= 0.0f) return;

	// Accumulator math (mirrors the AddMoralePoints native body, minus the
	// device-mode lookup we already handled above and the GameInfo state
	// flag gate which we always satisfy on a running match).
	const float before    = recipient->m_fCurrentMoralePoints;
	const float threshold = recipient->r_fRequiredMoralePoints;
	float after = before + points;
	if (after < 0.0f)            after = 0.0f;
	else if (after > threshold)  after = threshold;

	if (after != before) {
		// m_fCurrentMoralePoints is the server-canonical accumulator;
		// r_fCurrentServerMoralePoints is what the client's repnotify
		// copies back into its local m_fCurrentMoralePoints for the bar
		// UI. Write both — the native at 0x109d2f00 only writes the
		// server-canonical field and relies on a separate PreReplication
		// sync we haven't reimplemented.
		recipient->m_fCurrentMoralePoints      = after;
		recipient->r_fCurrentServerMoralePoints = after;
		recipient->bForceNetUpdate = 1;
		recipient->bNetDirty       = 1;
	}

	if (Logger::IsChannelEnabled("morale")) {
		Logger::Log("morale",
			"[MoraleCredit] %s: recipient=0x%p mag=%.2f gainFactor=%.3f points=%.4f  morale %.2f -> %.2f / %.2f  devMode=%d\n",
			isHeal ? "heal" : "dmg",
			recipient, creditMagnitude, gainFactor, points,
			before, after, threshold, deviceModeId);
	}
}

}  // namespace MoraleCredit
