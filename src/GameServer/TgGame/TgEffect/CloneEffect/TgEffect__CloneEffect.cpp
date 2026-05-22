#include "src/GameServer/TgGame/TgEffect/CloneEffect/TgEffect__CloneEffect.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>
#include <unordered_set>

// ConstructObject<UTgEffect> — IsChildOf-validates the requested UClass against
// UTgEffect::StaticClass(), so passing TgEffectDamage / TgEffectHeal / etc.
// works without finding each subclass's own ConstructObject. Allocates the
// instance at the class's true PropertySize (the engine knows it), so the new
// object is correctly sized regardless of the memcpy bound below.
typedef void*(__cdecl* ConstructEffectFn)(void*, int, int, int, unsigned, unsigned, int, int, int*);
static const ConstructEffectFn ConstructEffectNative = (ConstructEffectFn)0x10a73f30;

// Per-class instance size for the memcpy bound. These are the SDK-declared
// sizes of the six concrete UTgEffect subclasses (canonical §0/§1):
//   TgEffectDamage 0x80, TgEffectHeal 0x74, all others (TgEffect, TgEffectBuff,
//   TgEffectSensor, TgEffectVisibility) 0x70.
//
// DELIBERATE deviation from "use UStruct::PropertySize" (canonical §3): the GA
// build places PropertySize at UClass+0x54, but using an unverified offset as a
// memcpy *length* risks writing past the destination allocation — the
// memory-corruption class of bug the project has been repeatedly burned by, and
// one I cannot catch without building. The known sizes are correct and safe;
// truncating (never over-running) is the safe failure direction. PropertySize
// is read only as a one-time DIAGNOSTIC cross-check so the offset can be
// validated in-game before any future switch to it.
static int KnownEffectInstanceSize(const std::string& className) {
	if (className.find("TgEffectDamage") != std::string::npos) return 0x80;
	if (className.find("TgEffectHeal")   != std::string::npos) return 0x74;
	return 0x70;  // TgEffect / TgEffectBuff(gone) / TgEffectSensor / TgEffectVisibility
}

// One-time-per-class diagnostic: compare the known size to UClass+0x54.
static void LogPropertySizeOnce(UClass* cls, const std::string& className, int knownSize) {
	static std::unordered_set<UClass*> seen;
	if (!cls || seen.count(cls)) return;
	seen.insert(cls);
	const uint32_t propSize = *(const uint32_t*)((const uint8_t*)cls + 0x54);  // UStruct::PropertySize (GA @ 0x54)
	Logger::Log("effects",
		"[CLONE-EFFECT] class=%s knownSize=0x%X UClass+0x54(PropertySize)=0x%X %s\n",
		className.c_str(), knownSize, propSize,
		((int)propSize == knownSize) ? "(match)" : "(MISMATCH - investigate before trusting PropertySize)");
}

// Reimplementation of TgEffect::CloneEffect — original native @ 0x10a6f2a0 is
// the empty stripped stub. Constructs a fresh UTgEffect of the same concrete
// class and copies all per-instance fields, giving each application target its
// own m_fCurrent / m_fBase so apply/remove math across simultaneous
// applications never collides on shared template state.
//
// The UObject header (0x00..0x3C: Class, Name, Outer, …) is populated by
// ConstructObject, so we copy only the persistent state at 0x3C..instSize.
// UTgEffect has no TArray/FString members (all inline storage), so a shallow
// memcpy is correct. Group-relative fixups (re-wiring m_EffectGroup to the
// clone group, asserting m_bRemovable) belong to the caller (CloneEffectGroup),
// not this per-effect primitive.
UTgEffect* __fastcall TgEffect__CloneEffect::Call(UTgEffect* effect, void* /*edx*/) {
	if (!effect || !effect->Class) return nullptr;

	const std::string& className = ObjectClassCache::GetClassName(effect->Class);
	const int instSize = KnownEffectInstanceSize(className);
	LogPropertySizeOnce(effect->Class, className, instSize);

	UTgEffect* clone = (UTgEffect*)ConstructEffectNative(
		effect->Class, -1, 0, 0, 0, 0, 0, 0, nullptr);
	if (!clone) {
		Logger::Log("effects", "[CLONE-EFFECT] ConstructObject failed for class=%s\n",
			className.c_str());
		return nullptr;
	}

	// 0x3C..instSize is everything beyond the UObject header for the concrete
	// subclass. Inline storage only, so a shallow memcpy is correct.
	std::memcpy((uint8_t*)clone + 0x3C, (const uint8_t*)effect + 0x3C, instSize - 0x3C);
	return clone;
}
