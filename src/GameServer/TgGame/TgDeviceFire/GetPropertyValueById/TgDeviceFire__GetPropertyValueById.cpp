#include "src/GameServer/TgGame/TgDeviceFire/GetPropertyValueById/TgDeviceFire__GetPropertyValueById.hpp"

// Resolve nPropertyId → UTgProperty* by walking the pawn's s_Properties.
// Mirror of the lookup in our TgPawn__GetProperty hook, but inlined here so
// we don't take a dependency on its specific signature.
static inline UTgProperty* FindPropertyOnPawn(ATgPawn* pawn, int propertyId) {
	if (!pawn || !pawn->s_Properties.Data) return nullptr;
	for (int i = 0; i < pawn->s_Properties.Num(); ++i) {
		UTgProperty* p = pawn->s_Properties.Data[i];
		if (p && p->m_nPropertyId == propertyId) {
			return p;
		}
	}
	return nullptr;
}

float __fastcall TgDeviceFire__GetPropertyValueById::Call(UTgDeviceFire* DeviceFire, void* /*edx*/, int nPropertyId, int nPropertyIndex) {
	if (!DeviceFire) return 0.0f;

	// First: try the cached-index path. If something else (now or later) ever
	// populates the per-fire-mode m_Properties + cached indices, the original
	// path is faster and supports buff-modifier hooks. Skip it when the index
	// is the UC-default -1, where the original always returns 0.0.
	if (nPropertyIndex >= 0 && nPropertyIndex < DeviceFire->m_Properties.Num()) {
		float cachedVal = CallOriginal(DeviceFire, nullptr, nPropertyId, nPropertyIndex);
		if (cachedVal != 0.0f) {
			return cachedVal;
		}
	}

	// Fallback: resolve against the pawn's s_Properties by propId.
	// m_Owner (offset 0x3C) is the device; AActor::Instigator gives the pawn.
	ATgDevice* device = (ATgDevice*)DeviceFire->m_Owner;
	if (!device) return 0.0f;
	ATgPawn* pawn = (ATgPawn*)device->Instigator;
	if (!pawn) return 0.0f;

	UTgProperty* prop = FindPropertyOnPawn(pawn, nPropertyId);
	if (!prop) return 0.0f;

	float val = prop->m_fRaw;
	// Match the clamp-to-[min,max] the original applies when both bounds are
	// non-zero (m_fMaximum > epsilon AND m_fMinimum <= m_fMaximum).
	if (prop->m_fMaximum > 0.0f && prop->m_fMinimum <= prop->m_fMaximum) {
		if (val < prop->m_fMinimum) val = prop->m_fMinimum;
		else if (val > prop->m_fMaximum) val = prop->m_fMaximum;
	}
	return val;
}
