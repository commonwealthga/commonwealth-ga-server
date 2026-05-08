#include "src/GameServer/TgGame/TgDeviceFire/CheckTeamPassThrough/TgDeviceFire__CheckTeamPassThrough.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <string>

// UObject::GetFullName / GetName both return pointers into a single static
// buffer that is overwritten by the next call.  Multiple GetFullName() calls
// in one log line clobber each other (memory rule
// reference_getfullname_shared_buffer.md).  Capture each one into its own
// std::string immediately, before the next call.
static std::string SafeGetFullName(UObject* obj) {
	if (!obj) return "<null-obj>";
	if (!obj->Class) return "<null-class>";
	const char* fn = obj->Class->GetFullName();
	return fn ? std::string(fn) : std::string("<null-fullname>");
}

static bool ContainsAny(const std::string& s, std::initializer_list<const char*> needles) {
	for (auto* n : needles) if (s.find(n) != std::string::npos) return true;
	return false;
}

bool __fastcall TgDeviceFire__CheckTeamPassThrough::Call(
		UTgDeviceFire* FireMode, void* edx, AActor* HitActor) {
	bool result = CallOriginal(FireMode, edx, HitActor);

	if (!HitActor) return result;

	// Capture class name BEFORE any other GetFullName call clobbers the buffer.
	std::string hitClass = SafeGetFullName(HitActor);

	// Only log deployable hits.  Match TgDeployable + all subclasses.
	if (!ContainsAny(hitClass, {"TgDeployable", "TgDeploy_"})) return result;

	AActor* owner = FireMode ? FireMode->m_Owner : nullptr;
	APawn* instigator = (owner && owner->Instigator) ? owner->Instigator : nullptr;

	std::string ownerClass     = SafeGetFullName(owner);
	std::string instigatorClass = SafeGetFullName(instigator);

	Logger::Log("combat-trace",
		"[CheckTeamPassThrough] firemode=%p owner=%p (%s) instigator=%p (%s)\n"
		"                       hitActor=%p (%s)\n"
		"                       returned=%d (false=register-hit, true=pass-through)\n",
		FireMode, owner, ownerClass.c_str(),
		instigator, instigatorClass.c_str(),
		HitActor, hitClass.c_str(),
		(int)result);
	return result;
}
