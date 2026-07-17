#include "src/GameServer/TgGame/TgMissionObjective/RegisterSelf/TgMissionObjective__RegisterSelf.hpp"
#include "src/GameServer/GameModes/CtrPointRotation/CtrPointRotation.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// RegisterSelf is called from TgMissionObjective.PostBeginPlay().
// The original is a stub. For proximity/KOTH objectives we need to spawn
// a TgCollisionProxy so that player-in-zone detection works.
void __fastcall TgMissionObjective__RegisterSelf::Call(ATgMissionObjective* Objective, void* edx) {
	LogCallBegin();

	// On the repurposed CTR maps, stock CTR objectives run their PostBeginPlay
	// (AddToList + RegisterSelf) AFTER CtrPointRotation::Init seeds our points,
	// so they re-pollute the rotation list. Drop + disable any such straggler.
	// No-op on every non-CTR map and before seeding completes.
	CtrPointRotation::ExcludeLateStockObjective(Objective);

	// "Register self" with the GRI's replicated r_Objectives[0x4B] mirror —
	// the array the client HUD reads. On our server the only other writer is
	// TgGame__InitGameRepInfo's one-time ActorCache pass, which runs before
	// kismet-streamed sublevels arrive, so their objectives (Solar Farm's
	// BossSolo/BossGrp bosses) never reached any client: bosshud playtest
	// 2026-07-17 showed boss 14073 spawning with r_Objectives slot=-1 while
	// the client had the sublevel confirmed visible. Gates:
	//  - streamed-package actors only (outermost package differs from
	//    WorldInfo's, i.e. the actor lives in a streamed sublevel package):
	//    persistent objectives are covered by the init pass, dynamic ones
	//    (SuperAgent/CTR) are spawned INTO the persistent level and inserted
	//    explicitly by their modes — auto-adding them here duplicated the
	//    SuperAgent points on the HUD (added at class-default priority during
	//    Spawn's PostBeginPlay, then again by the mode at real priority; the
	//    old gate compared GetFullName against Config::GetMapNameChar and
	//    missed on package-name case differences);
	//  - present in m_MissionObjectives (own AddToList ran, not CTR-excluded);
	//  - absent from r_Objectives (idempotent).
	{
		ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)(Objective->WorldInfo != nullptr
			? Objective->WorldInfo->GRI : nullptr);
		UObject* objPkg = (UObject*)Objective;
		while (objPkg->Outer != nullptr) objPkg = objPkg->Outer;
		UObject* worldPkg = (UObject*)Objective->WorldInfo;
		while (worldPkg != nullptr && worldPkg->Outer != nullptr) worldPkg = worldPkg->Outer;
		const bool bStreamedIn = (worldPkg != nullptr && objPkg != worldPkg);
		if (GRI != nullptr && bStreamedIn) {
			bool bInMissionList = false;
			for (int i = 0; i < GRI->m_MissionObjectives.Num(); i++) {
				if (GRI->m_MissionObjectives.Data[i] == Objective) { bInMissionList = true; break; }
			}
			bool bInRepList = false;
			for (int i = 0; i < 0x4B; i++) {
				if (GRI->r_Objectives[i] == Objective) { bInRepList = true; break; }
			}
			if (bInMissionList && !bInRepList) {
				using AddObjectivePointToList_t =
					void(__fastcall*)(ATgRepInfo_Game*, void*, ATgMissionObjective*);
				reinterpret_cast<AddObjectivePointToList_t>(0x109f0580)(GRI, nullptr, Objective);
				Logger::Log("bosshud",
					"RegisterSelf: streamed-in objective mid=%d prio=%d -> r_Objectives\n",
					Objective->m_nMapObjectId, Objective->nPriority);
			}
		}
	}

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
