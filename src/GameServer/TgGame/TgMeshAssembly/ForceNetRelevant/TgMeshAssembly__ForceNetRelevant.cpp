#include "src/GameServer/TgGame/TgMeshAssembly/ForceNetRelevant/TgMeshAssembly__ForceNetRelevant.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Reimplements the Actor.ForceNetRelevant event logic that TgMeshAssembly's
// native override stubbed out. Without this, bNoDelete actors like
// TgMissionObjective never get RemoteRole set, so their properties
// never replicate to clients.
void __fastcall TgMeshAssembly__ForceNetRelevant::Call(ATgMeshAssembly* Actor, void* edx) {
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
