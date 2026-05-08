#include "src/GameServer/TgGame/TgDeviceFire/ApplyFireModeSetup/TgDeviceFire__ApplyFireModeSetup.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Log channel: "fire_mode_setup". Add to enabled channels in config to see.
static constexpr const char* CH = "fire_mode_setup";

int __fastcall TgDeviceFire__ApplyFireModeSetup::Call(UTgDeviceFire* DeviceFire, void* /*edx*/, void* Owner, void* SetupStruct) {
	// Best-effort owner classification.
	const char* ownerClass = "?";
	if (Owner) {
		auto* asObj = (UObject*)Owner;
		if (asObj->Class) {
			const char* full = asObj->Class->GetFullName();
			if (full) ownerClass = full;
		}
	}

	// Pre-call snapshot: how many props are already in m_Properties, and what
	// the cache fields look like. Should be 0/-1 for fresh fire modes.
	const int   preCount   = DeviceFire ? DeviceFire->m_Properties.Num() : -1;
	const int   preAcc     = DeviceFire ? DeviceFire->m_nAccuracyIndex          : -1;
	const int   preAccWalk = DeviceFire ? DeviceFire->m_nAccuracyWalkIndex      : -1;
	const int   preAccCr   = DeviceFire ? DeviceFire->m_nAccuracyCrouchIndex    : -1;
	const int   preDmgRad  = DeviceFire ? DeviceFire->m_nDamageRadiusIndex      : -1;
	const int   preRange   = DeviceFire ? DeviceFire->m_nRangeIndex             : -1;
	const int   preCool    = DeviceFire ? DeviceFire->m_nCoolDownTimeIndex      : -1;

	Logger::Log(CH,
		"[ENTER] devFire=%p owner=%p ownerClass='%s' setup=%p  PRE: props=%d "
		"accIdx=%d walkIdx=%d crouchIdx=%d dmgRadIdx=%d rangeIdx=%d coolIdx=%d\n",
		DeviceFire, Owner, ownerClass, SetupStruct,
		preCount, preAcc, preAccWalk, preAccCr, preDmgRad, preRange, preCool);

	const int ret = CallOriginal(DeviceFire, /*edx=*/nullptr, Owner, SetupStruct);

	const int postCount   = DeviceFire ? DeviceFire->m_Properties.Num() : -1;
	const int postAcc     = DeviceFire ? DeviceFire->m_nAccuracyIndex          : -1;
	const int postAccWalk = DeviceFire ? DeviceFire->m_nAccuracyWalkIndex      : -1;
	const int postAccCr   = DeviceFire ? DeviceFire->m_nAccuracyCrouchIndex    : -1;
	const int postDmgRad  = DeviceFire ? DeviceFire->m_nDamageRadiusIndex      : -1;
	const int postRange   = DeviceFire ? DeviceFire->m_nRangeIndex             : -1;
	const int postCool    = DeviceFire ? DeviceFire->m_nCoolDownTimeIndex      : -1;
	const int postId      = DeviceFire ? DeviceFire->m_nId                     : -1;

	Logger::Log(CH,
		"[EXIT ] devFire=%p ret=%d  POST: props=%d accIdx=%d walkIdx=%d "
		"crouchIdx=%d dmgRadIdx=%d rangeIdx=%d coolIdx=%d  m_nId=%d\n\n",
		DeviceFire, ret,
		postCount, postAcc, postAccWalk, postAccCr, postDmgRad, postRange, postCool,
		postId);

	return ret;
}
