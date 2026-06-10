#include "src/GameServer/TgGame/TgPawn/IsFriendlyWithLocalPawn/TgPawn__IsFriendlyWithLocalPawn.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <unordered_map>

// CLIENT-SIDE DIAGNOSTIC. Logs the full decision chain of the native for
// stealthed pawns: branch taken, caller, and the PRI/TaskForce pointers that
// CheckIsEnemy (0x109ee170) compares. Change-driven dedup per pawn so the
// decisive SwapMeshToStealthed-time call is never missed.
unsigned int __fastcall TgPawn__IsFriendlyWithLocalPawn::Call(ATgPawn* Pawn, void* edx) {
	const unsigned int orig = CallOriginal(Pawn, edx);

	if (Pawn && Pawn->r_bIsStealthed) {
		void* caller = __builtin_return_address(0);

		ATgPlayerController* localPC = Pawn->c_LocalPC;
		ATgRepInfo_Player* thisPRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		ATgRepInfo_TaskForce* thisTF = thisPRI ? thisPRI->r_TaskForce : nullptr;

		APawn* localPawn = localPC ? localPC->Pawn : nullptr;
		ATgRepInfo_Player* localPCPRI =
			localPC ? (ATgRepInfo_Player*)localPC->PlayerReplicationInfo : nullptr;
		// CheckIsEnemy resolves the LOCAL side via localPawn->PRI->r_TaskForce
		// (GetTaskForceFor 0x109f1fa0), not localPC->PRI — log both.
		ATgRepInfo_Player* localPawnPRI =
			localPawn ? (ATgRepInfo_Player*)localPawn->PlayerReplicationInfo : nullptr;
		ATgRepInfo_TaskForce* localTF = localPawnPRI ? localPawnPRI->r_TaskForce : nullptr;

		const int thisTfNum  = thisTF  ? (int)thisTF->r_nTaskForce  : -1;
		const int localTfNum = localTF ? (int)localTF->r_nTaskForce : -1;

		int branch;
		if (localPC == nullptr)                               branch = 1;
		else if (thisPRI != nullptr && localPCPRI != nullptr) branch = 2;
		else                                                  branch = 3;

		// One line per state change; heartbeat every 120 identical calls.
		static std::unordered_map<int, int> s_lastState;
		static std::unordered_map<int, int> s_sameCount;
		const int state = (branch << 24) | ((int)orig << 16)
			| ((thisTfNum & 0xFF) << 8) | (localTfNum & 0xFF);
		int& last = s_lastState[Pawn->r_nPawnId];
		int& same = s_sameCount[Pawn->r_nPawnId];
		const bool changed = (last != state);
		if (changed) { last = state; same = 0; }
		if (changed || (++same % 120) == 0) {
			Logger::Log("stealth",
				"[isfriendly] pawn=%d ret=%s branch=%d caller=%p c_LocalPC=%p "
				"thisPRI=%p thisTF=%p thisTfNum=%d localPawn=%p localPCPRI=%p "
				"localPawnPRI=%p localTF=%p localTfNum=%d initEnemy=%d m=%.2f\n",
				Pawn->r_nPawnId, orig ? "FRIEND" : "enemy", branch, caller,
				(void*)localPC, (void*)thisPRI, (void*)thisTF, thisTfNum,
				(void*)localPawn, (void*)localPCPRI, (void*)localPawnPRI,
				(void*)localTF, localTfNum,
				(int)Pawn->r_bInitialIsEnemy, Pawn->m_fMakeVisibleCurrent);
		}
	}

	return orig;
}
