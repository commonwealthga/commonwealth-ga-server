#include "src/GameServer/TgGame/TgPawn/TickClientStealthDiag/TgPawn__TickClientStealthDiag.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <unordered_map>

void __fastcall TgPawn__TickClientStealthDiag::Call(ATgPawn* Pawn, void* edx, float DeltaTime) {
	CallOriginal(Pawn, edx, DeltaTime);

	if (Pawn && (Pawn->r_bIsStealthed || Pawn->m_fMakeVisibleCurrent != 0.0f)) {
		static std::unordered_map<int, int> s_n;
		int& n = s_n[Pawn->r_nPawnId];
		if ((n++ % 30) == 0) {  // ~1 line/sec/pawn
			Logger::Log("stealth",
				"[clienttick] pawn=%d m=%.4f incr=%.4f fadeRate=%.3f tickState=%d stealthed=%d alert=%d dt=%.4f\n",
				Pawn->r_nPawnId, Pawn->m_fMakeVisibleCurrent, Pawn->r_fMakeVisibleIncreased,
				Pawn->r_fMakeVisibleFadeRate, Pawn->c_nTickCheckingState,
				(int)Pawn->r_bIsStealthed, Pawn->r_nSensorAlertLevel, DeltaTime);
		}
	}
}
