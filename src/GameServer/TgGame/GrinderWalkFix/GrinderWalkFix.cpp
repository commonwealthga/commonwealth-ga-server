#include "src/GameServer/TgGame/GrinderWalkFix/GrinderWalkFix.hpp"
#include "src/GameServer/Globals.hpp"

// Mobile Grinder = bot 1428, body_asm_id 2110. Bots replicate this into
// r_nBodyMeshAsmId at spawn (SpawnBotById.cpp:951). The headless game instance
// never loads skeletal meshes, so the asm id is the reliable server-visible
// identity, unique to the grinder.
static constexpr int kGrinderBodyAsmId = 2110;

// Root cause (reproduced on a healthy grinder): the grinder's robot anim tree
// (AT_Robot_Mining_B_Cargo) permanently breaks its posture-0 walk node once it
// passes through posture TG_POSTURE_CRITICALFAILURE (11) — after that it slides
// with a frozen skeleton. The stun effect drives this chassis into CRITICALFAILURE
// (TgEffectGroup.uc:431 SetPosture). Fix: remap that posture to TG_POSTURE_STUNNED
// (5) — the same stun posture other NPCs use, which the tree handles without
// poisoning the walk. r_ePosture is delta-replicated (V2 rep list, bNetDirty
// block), so this reaches the vanilla client. Scoped to the grinder + posture 11.
static constexpr unsigned char kCriticalFailurePosture = 11;  // TG_POSTURE_CRITICALFAILURE
static constexpr unsigned char kStunnedPosture         = 5;   // TG_POSTURE_STUNNED

bool GrinderWalkFix::IsMobileGrinder(ATgPawn* pawn) {
	return pawn && pawn->r_nBodyMeshAsmId == kGrinderBodyAsmId;
}

void GrinderWalkFix::Tick() {
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game || !Game->WorldInfo) return;

	for (APawn* p = Game->WorldInfo->PawnList; p != nullptr; p = p->NextPawn) {
		ATgPawn* tp = (ATgPawn*)p;
		if (!IsMobileGrinder(tp)) continue;

		if (tp->r_ePosture == kCriticalFailurePosture) {
			tp->r_ePosture      = kStunnedPosture;
			tp->bNetDirty       = 1;
			tp->bForceNetUpdate = 1;
		}
	}
}
