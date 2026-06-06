#include "src/GameServer/TgGame/TgMissionObjective/RegisterSelf/TgMissionObjective__RegisterSelf.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
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
	bool bIsPayload = strcmp(className, "Class TgGame.TgMissionObjective_Escort") == 0;

	if (bIsProximity || bIsKOTH || bIsPayload) {
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

		// Positionally chain the collision proxy to the objective via SetBase.
		// SetOwner alone is just a logical link (used for Owner.Touch forwarding
		// in TgCollisionProxy.uc) — it does NOT make the proxy follow the
		// objective when the objective moves. Escort objectives later call
		// SetBase(r_AttachedActor) on themselves in TgMissionObjective_Escort
		// PostBeginPlay, so the chain becomes proxy → objective → payload and
		// the proxy's collision cylinder tracks the moving payload. For static
		// objectives (Proximity / KOTH) this is harmless — the base just never
		// moves.
		Proxy->SetBase(Objective, FVector(0, 0, 0), nullptr, FName());

		// Set cylinder dimensions from the objective's proximity settings
		// float radius = (float)ProxObj->m_fProximityRadius * 10.f;
		// float height = ProxObj->m_fProximityHeight * 10.f;
		// if (radius <= 0.0f) radius = 512.0f;
		// if (height <= 0.0f) height = 256.0f;
		float radius = 250.0f;
		UObject* Game = (UObject*)Globals::Get().GGameInfo;
		bool bIsEscortGame = Game != nullptr && ObjectClassCache::ClassNameContains(Game, "TgGame_Escort");
		float height = bIsEscortGame ? 200.0f : 70.0f;

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
