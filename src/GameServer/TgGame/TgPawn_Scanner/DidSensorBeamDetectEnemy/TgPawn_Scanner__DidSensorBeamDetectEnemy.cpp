#include "src/GameServer/TgGame/TgPawn_Scanner/DidSensorBeamDetectEnemy/TgPawn_Scanner__DidSensorBeamDetectEnemy.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <string>

int __fastcall TgPawn_Scanner__DidSensorBeamDetectEnemy::Call(
	ATgPawn_Scanner* Scanner, void* edx, AActor* target) {

	const int result = CallOriginal(Scanner, edx, target);

	// Gather identities for the log.
	auto nameOf = [](AActor* a) -> std::string {
		if (a == nullptr) return "<null>";
		const char* n = a->GetName();
		return n ? std::string(n) : std::string("<null>");
	};
	auto classOf = [](AActor* a) -> std::string {
		if (a == nullptr || a->Class == nullptr) return "<null>";
		const char* c = a->Class->GetFullName();
		return c ? std::string(c) : std::string("<null>");
	};

	int scanTeam = -1, scanTF = -1, scanPhase = -1;
	if (Scanner != nullptr) {
		scanPhase = Scanner->r_nPhase;
		ATgRepInfo_Player* sPri = (ATgRepInfo_Player*)Scanner->PlayerReplicationInfo;
		if (sPri != nullptr) { scanTeam = sPri->Team; scanTF = sPri->r_TaskForce; }
	}

	int tgtTeam = -1, tgtTF = -1;
	bool tgtIsPawn = false;
	if (target != nullptr) {
		const char* cn = target->Class ? target->Class->GetFullName() : nullptr;
		if (cn && std::string(cn).find("Pawn") != std::string::npos) tgtIsPawn = true;
		APawn* tp = (APawn*)target;
		ATgRepInfo_Player* tPri = (ATgRepInfo_Player*)tp->PlayerReplicationInfo;
		if (tPri != nullptr) { tgtTeam = tPri->Team; tgtTF = tPri->r_TaskForce; }
	}

	Logger::Log("alarm",
		"DidSensorBeamDetectEnemy result=%d  scanner=%s phase=%d team=%d tf=%d  "
		"target=%s (%s) tgtTeam=%d tgtTF=%d isPawn=%d\n",
		result,
		nameOf((AActor*)Scanner).c_str(), scanPhase, scanTeam, scanTF,
		nameOf(target).c_str(), classOf(target).c_str(), tgtTeam, tgtTF, (int)tgtIsPawn);

	return result;
}
