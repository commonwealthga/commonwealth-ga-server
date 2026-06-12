#include "src/GameServer/TgGame/TgGame/CalcAttackerReviveTime/TgGame__CalcAttackerReviveTime.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Wave interval (seconds) for the attacking side. Stub native in the binary —
// no CallOriginal. Per-side value with m_nSecsToAutoRelease fallback.
float __fastcall TgGame__CalcAttackerReviveTime::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	int secs = Game->m_nSecsToAutoReleaseAttackers > 0
		? Game->m_nSecsToAutoReleaseAttackers
		: Game->m_nSecsToAutoRelease;
	LogCallEnd();
	return (float)secs;
}
