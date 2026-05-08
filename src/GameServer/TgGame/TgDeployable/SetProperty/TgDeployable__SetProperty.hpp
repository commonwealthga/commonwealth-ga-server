#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Native at 0x10a1c940 — __thiscall(this, int nPropId, float fValue).
//
// SetProperty walks s_Properties for the matching prop_id, writes the new
// value into that prop's m_fRaw (+0x44), and dispatches ApplyProperty
// (vtable slot 251 @ 0x10a1ade0). The native ApplyProperty for TgDeployable
// only fans out props 8 (proximity) and 278 (deploy_rate_modifier) — every
// other prop id silently no-ops, so heal effects calling SetProperty(51, hp)
// update s_Properties[51]->m_fRaw but never touch r_nHealth /
// r_DRI->r_nHealthCurrent. The client never sees the heal.
//
// Compare with TgPawn::ApplyProperty @ 0x109cc7d0 which has explicit cases
// for prop 51 (HEALTH) and 304 (HEALTH_MAX) that mirror to r_nHealth /
// r_nHealthMaximum and fan out to the PRI — that's why repair arms heal
// turrets (pets are pawns) but not stations (deployables).
//
// Hook strategy: let the original run (linear scan + m_fRaw write +
// ApplyProperty for the two natively-handled props), then post-process for
// HEALTH / HEALTH_MAX so the visible HP fields and the DRI replicate.
class TgDeployable__SetProperty : public HookBase<
	void(__fastcall*)(ATgDeployable*, void*, int, float),
	0x10a1c940,
	TgDeployable__SetProperty> {
public:
	static void __fastcall Call(ATgDeployable* Deployable, void* edx, int nPropId, float fValue);
	static inline void __fastcall CallOriginal(ATgDeployable* Deployable, void* edx, int nPropId, float fValue) {
		m_original(Deployable, edx, nPropId, fValue);
	};
};
