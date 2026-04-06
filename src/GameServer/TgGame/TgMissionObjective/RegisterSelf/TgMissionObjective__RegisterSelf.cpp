#include "src/GameServer/TgGame/TgMissionObjective/RegisterSelf/TgMissionObjective__RegisterSelf.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"

// RegisterSelf is called from TgMissionObjective.PostBeginPlay().
// The original is a stub. For proximity/KOTH objectives we need to spawn
// a TgCollisionProxy so that player-in-zone detection works.
void __fastcall TgMissionObjective__RegisterSelf::Call(ATgMissionObjective* Objective, void* edx) {
	LogCallBegin();

	char* className = Objective->Class->GetFullName();
	Logger::Log(GetLogChannel(), "Objective class: %s\n", className);

	bool bIsProximity = strcmp(className, "Class TgGame.TgMissionObjective_Proximity") == 0;
	bool bIsKOTH = strcmp(className, "Class TgGame.TgBaseObjective_KOTH") == 0;

	if (bIsProximity || bIsKOTH) {
		ATgMissionObjective_Proximity* ProxObj = (ATgMissionObjective_Proximity*)Objective;

		if (ProxObj->s_CollisionProxy != nullptr) {
			Logger::Log(GetLogChannel(), "CollisionProxy already exists, skipping spawn\n");
			LogCallEnd();
			return;
		}

		FVector loc = Objective->Location;
		FRotator rot = Objective->Rotation;

		ATgCollisionProxy* Proxy = (ATgCollisionProxy*)Objective->Spawn(
			ClassPreloader::GetClass("Class TgGame.TgCollisionProxy"),
			Objective,
			FName(),
			loc,
			rot,
			nullptr,
			1
		);

		if (Proxy == nullptr) {
			Logger::Log(GetLogChannel(), "Failed to spawn TgCollisionProxy\n");
			LogCallEnd();
			return;
		}

		Proxy->SetOwner(Objective);

		// Set cylinder dimensions from the objective's proximity settings
		float radius = (float)ProxObj->m_fProximityRadius;
		float height = ProxObj->m_fProximityHeight;
		if (radius <= 0.0f) radius = 512.0f;
		if (height <= 0.0f) height = 256.0f;

		UCylinderComponent* Cylinder = (UCylinderComponent*)Proxy->CollisionComponent;
		if (Cylinder != nullptr) {
			Cylinder->SetCylinderSize(radius, height);
		}

		ProxObj->s_CollisionProxy = Proxy;

		Logger::Log(GetLogChannel(), "Spawned TgCollisionProxy for %s (radius=%.0f, height=%.0f)\n",
			Objective->GetFullName(), radius, height);
	}

	LogCallEnd();
}
