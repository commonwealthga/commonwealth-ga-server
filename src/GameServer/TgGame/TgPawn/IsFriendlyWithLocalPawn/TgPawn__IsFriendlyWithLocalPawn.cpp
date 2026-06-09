#include "src/GameServer/TgGame/TgPawn/IsFriendlyWithLocalPawn/TgPawn__IsFriendlyWithLocalPawn.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <unordered_map>

// Re-derive the branch label (purely to TAG the native's real return) and
// dump the pointers the native reads. Only for stealthed pawns (the cloak MIC
// path), throttled per pawn. Does not change behavior.
unsigned int __fastcall TgPawn__IsFriendlyWithLocalPawn::Call(ATgPawn* Pawn, void* edx) {
	const unsigned int orig = CallOriginal(Pawn, edx);

	if (Pawn && Pawn->r_bIsStealthed) {
		static std::unordered_map<int, int> s_cnt;
		int& cnt = s_cnt[Pawn->r_nPawnId];
		if ((cnt++ % 15) == 0) {
			ATgPlayerController* localPC = Pawn->c_LocalPC;
			void* thisPRI   = (void*)Pawn->PlayerReplicationInfo;
			void* localPawn = localPC ? (void*)localPC->Pawn : nullptr;
			void* localPRI  = localPC ? (void*)localPC->PlayerReplicationInfo : nullptr;

			int branch;
			if (localPC == nullptr)                             branch = 1;
			else if (thisPRI != nullptr && localPRI != nullptr) branch = 2;
			else                                                branch = 3;

			Logger::Log("stealth",
				"[isfriendly] pawn=%d return=%s branch=%d c_LocalPC=0x%p thisPRI=0x%p "
				"localPawn=0x%p localPRI=0x%p initEnemy=%d\n",
				Pawn->r_nPawnId, orig ? "FRIEND" : "enemy", branch,
				(void*)localPC, thisPRI, localPawn, localPRI,
				(int)Pawn->r_bInitialIsEnemy);
		}
	}

	return orig;
}
