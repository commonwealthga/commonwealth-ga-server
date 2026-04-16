#include "src/GameServer/TgGame/TgHUD_Game/DrawActorOverlays/TgHUD_Game__DrawActorOverlays.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Rate-limit so we don't spam 60/s. Only log every time ANY gate value changes.
static int g_lastFireCount = 0;
static unsigned g_lastFlags480 = 0xDEADBEEF;
static void* g_lastHud = nullptr;

void __fastcall TgHUD_Game__DrawActorOverlays::Call(void* HUD, void* edx) {
	if (HUD) {
		char* p = (char*)HUD;
		void* worldInfo = *(void**)(p + 0xD8);          // param_1[0x36]
		unsigned flags480 = *(unsigned*)(p + 0x480);    // param_1[0x120] - flag with bit 0x02 gate
		void* fieldX119 = *(void**)(p + 0x464);         // param_1[0x119]
		void* canvas   = *(void**)(p + 0x42C);          // param_1[0x10b]
		void** vtable  = *(void***)HUD;
		void* vfn498   = vtable ? vtable[0x498/4] : nullptr;

		// Log first time + any time a gate flips
		// Actually invoke vtable[0x498] and log return value. If non-zero, that's
		// what's gating DrawActorOverlays from running to completion.
		int gateRet = -1;
		if (vfn498) {
			gateRet = ((int(__fastcall*)(void*, void*))vfn498)(HUD, edx);
		}

		bool shouldLog = (g_lastHud != HUD) || (g_lastFlags480 != flags480) || (g_lastFireCount < 5);
		if (shouldLog) {
			g_lastHud = HUD;
			g_lastFlags480 = flags480;
			g_lastFireCount++;

			void* pawnOwner = *(void**)(p + 0x468);          // m_PawnOwner
		unsigned flags678 = *(unsigned*)(p + 0x678);     // m_bDrawPawnHUD bit0x01, m_bBlockPawnHUD bit0x02
		void* pcPawn       = fieldX119 ? *(void**)((char*)fieldX119 + 0x1CC) : nullptr;
		void* pcViewTarget = fieldX119 ? *(void**)((char*)fieldX119 + 0x388) : nullptr;
		void* pcCamera     = fieldX119 ? *(void**)((char*)fieldX119 + 0x358) : nullptr;
		// AHUD base flags at +0x01FC: bit 0x01 LostFocusPaused, bit 0x02 bShowHUD,
		// bit 0x04 bShowScores, bit 0x08 bShowDebugInfo.
		unsigned flags1FC = *(unsigned*)(p + 0x1FC);
		Logger::Log("damage-info",
				"DrawActorOverlays ENTRY hud=%p pc=%p pc->Pawn=%p pc->ViewTarget=%p pc->PlayerCamera=%p "
				"m_PawnOwner=%p m_bDrawPawnHUD=%d m_bBlockPawnHUD=%d "
				"bShowHUD=%d bShowScores=%d bShowDebugInfo=%d\n",
				HUD, fieldX119, pcPawn, pcViewTarget, pcCamera,
				pawnOwner, (flags678 >> 0) & 1, (flags678 >> 1) & 1,
				(flags1FC >> 1) & 1, (flags1FC >> 2) & 1, (flags1FC >> 3) & 1);

		// Diagnostic write removed — confirmed m_PawnOwner + m_bDrawPawnHUD are
		// the only client-side blockers. Real fix needs to be server-side.
		}
	}
	CallOriginal(HUD, edx);
}
