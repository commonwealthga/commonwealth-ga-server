#include "src/GameServer/TgGame/TgGame/CalcDefenderReviveTime/TgGame__CalcDefenderReviveTime.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Wave interval (seconds) for the defending side. Stub native in the binary —
// no CallOriginal. Per-side value with m_nSecsToAutoRelease fallback.
float __fastcall TgGame__CalcDefenderReviveTime::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	int secs = Game->m_nSecsToAutoReleaseDefenders > 0
		? Game->m_nSecsToAutoReleaseDefenders
		: Game->m_nSecsToAutoRelease;
	LogCallEnd();
	return (float)secs;
}
