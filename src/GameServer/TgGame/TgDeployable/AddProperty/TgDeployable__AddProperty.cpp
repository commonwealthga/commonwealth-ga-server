#include "src/GameServer/TgGame/TgDeployable/AddProperty/TgDeployable__AddProperty.hpp"
#include "src/GameServer/TgGame/TgDeployable/InitializeDefaultProps/TgDeployable__InitializeDefaultProps.hpp"

// Native exec at 0x10a194e0 is a no-op stub. If anything in UC (or in our own
// code) invokes Deployable->AddProperty(...), route it through the same
// InitializeProperty helper that InitializeDefaultProps uses so the property
// actually lands in Deployable->s_Properties.
void __fastcall TgDeployable__AddProperty::Call(ATgDeployable* Deployable, void* edx, int nPropId, float fBase, float fRaw, float FMin, float FMax) {
	if (!Deployable) return;
	TgDeployable__InitializeDefaultProps::InitializeProperty(Deployable, nPropId, fBase, fRaw, FMin, FMax);
}
