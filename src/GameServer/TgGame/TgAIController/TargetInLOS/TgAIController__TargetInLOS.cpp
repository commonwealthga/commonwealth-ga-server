#include "src/GameServer/TgGame/TgAIController/TargetInLOS/TgAIController__TargetInLOS.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Throttle: only log every Nth call per AI to avoid flooding (AI runs LOS
// check every fire-tick during continuous fire — that's many per second).
static std::map<int, int> s_callCount;

int __fastcall TgAIController__TargetInLOS::Call(ATgAIController* aic, void* edx) {
	// Sanity log on FIRST entry (any AI, any test) just to confirm the hook
	// installs and the function actually fires during the turret-firing test.
	static bool bFirstEntry = true;
	if (bFirstEntry) {
		bFirstEntry = false;
		Logger::Log("turret_los",
			"[TargetInLOS] *** HOOK FIRED *** first invocation, aic=%p\n",
			(void*)aic);
	}

	int result = CallOriginal(aic, edx);

	if (!aic) return result;

	ATgPawn* pawn = aic->Pawn ? (ATgPawn*)aic->Pawn : nullptr;
	if (!pawn) return result;
	bool isBot = pawn->r_bIsBot;

	int& n = s_callCount[(int)aic];
	n++;
	// Log every call for now (no throttle, no bot filter) — trim later once
	// we've confirmed the hook fires for the turret scenario.
	(void)isBot;

	// Native TgAIController::GetTargetPawn at 0x10a80d00 — call directly to
	// avoid the SDK ProcessEvent round-trip in a hot path.
	typedef ATgPawn*(__fastcall* GetTargetPawnFn)(ATgAIController*, void*);
	static GetTargetPawnFn nativeGetTargetPawn = (GetTargetPawnFn)0x10a80d00;
	ATgPawn* target = nativeGetTargetPawn(aic, nullptr);

	// GetFullName returns into a shared static buffer — copy both names
	// into std::string before format-printing or the second call clobbers
	// the first's pointer.
	const char* pawnRaw = pawn->GetFullName();
	std::string pawnName = pawnRaw ? pawnRaw : "<null>";
	const char* targetRaw = target ? target->GetFullName() : nullptr;
	std::string targetName = targetRaw ? targetRaw : "(null)";

	Logger::Log("turret_los",
		"[TargetInLOS] aic=%p pawn=%s pawnLoc=(%.0f,%.0f,%.0f) "
		"target=%s targetLoc=(%.0f,%.0f,%.0f) result=%d (1=visible 0=blocked) call#=%d\n",
		(void*)aic,
		pawnName.c_str(), pawn->Location.X, pawn->Location.Y, pawn->Location.Z,
		targetName.c_str(),
		target ? target->Location.X : 0.0f,
		target ? target->Location.Y : 0.0f,
		target ? target->Location.Z : 0.0f,
		result, n);

	return result;
}
