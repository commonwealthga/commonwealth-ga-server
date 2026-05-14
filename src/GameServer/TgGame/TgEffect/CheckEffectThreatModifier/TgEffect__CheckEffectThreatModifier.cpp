#include "src/GameServer/TgGame/TgEffect/CheckEffectThreatModifier/TgEffect__CheckEffectThreatModifier.hpp"

// Intentional pass-through. Hook installed so future threat-scaling work has
// a place to land; current binary state has no canonical "threat modifier"
// prop to query. See header for rationale.
void __fastcall TgEffect__CheckEffectThreatModifier::Call(UTgEffect* /*effect*/, void* /*edx*/, float* /*NewValue*/) {
	// no-op
}
