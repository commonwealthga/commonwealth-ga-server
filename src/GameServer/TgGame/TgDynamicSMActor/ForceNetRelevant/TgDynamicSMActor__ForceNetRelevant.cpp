#include "src/GameServer/TgGame/TgDynamicSMActor/ForceNetRelevant/TgDynamicSMActor__ForceNetRelevant.hpp"

// Reimplements the Actor.ForceNetRelevant event logic that
// TgDynamicSMActor's native override stubbed out.
void __fastcall TgDynamicSMActor__ForceNetRelevant::Call(ATgDynamicSMActor* Actor, void* edx) {
	if (Actor->bClientSideOnly) {
		return;
	}

	if (Actor->RemoteRole == 0 && Actor->bNoDelete && !Actor->bStatic) {
		Actor->RemoteRole = 1; // ROLE_SimulatedProxy
		Actor->bAlwaysRelevant = 1;
		Actor->NetUpdateFrequency = 0.1f;
	}

	Actor->bForceNetUpdate = 1;
}
