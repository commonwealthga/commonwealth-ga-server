#include "src/GameServer/TgGame/TgDeviceFire/IsValidTarget/TgDeviceFire__IsValidTarget.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <string>

// GetFullName returns a pointer into a single static buffer; consecutive
// calls clobber each other.  Capture into std::string immediately.
// (memory rule reference_getfullname_shared_buffer.md)
static std::string SafeGetFullName(UObject* obj) {
	if (!obj) return "<null-obj>";
	if (!obj->Class) return "<null-class>";
	const char* fn = obj->Class->GetFullName();
	return fn ? std::string(fn) : std::string("<null-fullname>");
}

static bool Contains(const std::string& s, const char* n) {
	return s.find(n) != std::string::npos;
}

// Decode TGDTT_* enum to a readable string for the log.
static const char* TargeterTypeName(int v) {
	switch (v) {
		case 0: return "None";
		case 1: return "Self";
		case 2: return "Friend";
		case 3: return "Enemy";
		case 4: return "Enemy_And_Self";
		case 5: return "Friend_Only";
		case 6: return "All";
		default: return "?";
	}
}

bool __fastcall TgDeviceFire__IsValidTarget::Call(
		UTgDeviceFire* FireMode, void* edx, AActor* TargetActor, char OverrideTargeterType) {

	bool result = CallOriginal(FireMode, edx, TargetActor, OverrideTargeterType);

	if (!TargetActor) return result;

	// Capture target class name BEFORE other GetFullName calls clobber buffer.
	std::string targetClass = SafeGetFullName(TargetActor);

	// Only log when target is a TgDeployable.  Strings come from real per-class
	// ClassFullName so this is reliable (unlike strstr on shared buffer).
	bool isDeployable = Contains(targetClass, "TgDeployable") || Contains(targetClass, "TgDeploy_");
	if (!isDeployable) return result;

	// Target is a TgDeployable → r_nPhysicalType is at +0x1D8 (per SDK header).
	// (For TgPawn it would be at +0x6D8; we filtered to deployable only.)
	int targetPhysType = *(int*)((char*)TargetActor + 0x1D8);

	// FireMode internals.
	uint8_t targeterRaw    = FireMode ? *(uint8_t*)((char*)FireMode + 0x42) : 0;
	uint8_t effectiveTargeter = (uint8_t)OverrideTargeterType ? (uint8_t)OverrideTargeterType : targeterRaw;
	int     targetAffects  = FireMode ? *(int*)((char*)FireMode + 0x9c) : 0;
	uint8_t fireType       = FireMode ? *(uint8_t*)((char*)FireMode + 0x40) : 0;  // m_nFireType: 0=Instant 1=Projectile 2=Custom 3=Arcing
	AActor* owner          = FireMode ? FireMode->m_Owner : nullptr;
	APawn*  instigator     = (owner && owner->Instigator) ? owner->Instigator : nullptr;

	// Roles + r_bTakeDamage gate value — these are the suspect gates.
	uint8_t instigatorRole = instigator ? *(uint8_t*)((char*)instigator + 0x92) : 0xff;
	uint8_t instigatorRRole = instigator ? *(uint8_t*)((char*)instigator + 0x91) : 0xff;
	uint8_t ownerRole      = owner ? *(uint8_t*)((char*)owner + 0x92) : 0xff;
	// TgDeployable.r_bTakeDamage — bitfield at +0x1D0 mask 0x00000008.
	uint32_t targetDword1D0 = *(uint32_t*)((char*)TargetActor + 0x1D0);
	int      targetTakeDamage = (targetDword1D0 & 0x00000008) ? 1 : 0;

	std::string ownerClass     = SafeGetFullName(owner);
	std::string instigatorClass = SafeGetFullName(instigator);
	std::string firemodeClass  = SafeGetFullName(FireMode);

	const char* fireTypeName =
		fireType == 0 ? "Instant"   :
		fireType == 1 ? "Projectile":
		fireType == 2 ? "Custom"    :
		fireType == 3 ? "Arcing"    : "?";

	Logger::Log("combat-trace",
		"[IsValidTarget] firemode=%p (%s)  m_nFireType=%d (%s)\n"
		"                owner=%p (%s) ownerRole=%d\n"
		"                instigator=%p (%s) Role=%d RemoteRole=%d\n"
		"                target=%p (%s)  target.r_nPhysicalType=%d  r_bTakeDamage=%d (raw +0x1D0=0x%08x)\n"
		"                m_eTargeterType=%d (%s) override=%d effective=%d (%s)\n"
		"                m_nTargetAffectsType=%d\n"
		"                returned=%d (true=valid hit, false=reject)\n",
		FireMode, firemodeClass.c_str(), (int)fireType, fireTypeName,
		owner, ownerClass.c_str(), (int)ownerRole,
		instigator, instigatorClass.c_str(), (int)instigatorRole, (int)instigatorRRole,
		TargetActor, targetClass.c_str(),
		targetPhysType, targetTakeDamage, targetDword1D0,
		(int)targeterRaw, TargeterTypeName(targeterRaw),
		(int)(uint8_t)OverrideTargeterType,
		(int)effectiveTargeter, TargeterTypeName(effectiveTargeter),
		targetAffects,
		(int)result);
	return result;
}
