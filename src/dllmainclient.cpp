#include "src/pch.hpp"

#include "src/Utils/DebugWindow/DebugWindow.hpp"
#include "src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.hpp"
#include "src/GameServer/TgGame/TgDeployable/NotifyGroupChanged/TgDeployable__NotifyGroupChanged.hpp"
#include "src/GameServer/TgGame/TgRepInfo_Game/GetTaskForceFor/TgRepInfo_Game__GetTaskForceFor.hpp"
#include "src/GameServer/TgGame/TgDeployable/IsFriendlyWithLocalPawn/TgDeployable__IsFriendlyWithLocalPawn.hpp"
#include "src/GameServer/TgGame/TgPawn/IsFriendlyWithLocalPawn/TgPawn__IsFriendlyWithLocalPawn.hpp"
#include "src/GameServer/TgGame/TgHUD_Game/DrawActorOverlays/TgHUD_Game__DrawActorOverlays.hpp"
#include "src/GameServer/TgGame/TgPawn/TGPostRenderFor/TgPawn__TGPostRenderFor.hpp"

unsigned long ModuleThread( void* ) {
	::DetourTransactionBegin();
	::DetourUpdateThread(::GetCurrentThread());
	Logger::EnableChannel("stealth");
	Logger::EnableChannel("hook_calltree");
	Logger::EnableChannel("replicated_event");
	Logger::EnableChannel("heal_tick");
	Logger::EnableChannel("team_colors");
	Logger::EnableChannel("damage-info");

	UObject__ProcessEvent::Install();
	TgDeployable__NotifyGroupChanged::Install();
	// Client-side DIAGNOSTICS (not a real fix) — route deployable friendship
	// checks through the Instigator chain when the DRI path is unusable.
	// See each hook's hpp for why this isn't the solution; DRI replication
	// itself still needs to be repaired on the server side.
	TgRepInfo_Game__GetTaskForceFor::Install();        // DRI non-null but fields null
	TgDeployable__IsFriendlyWithLocalPawn::Install();  // r_DRI null entirely (medstation)

	// Spectator health-bar/buff-visibility investigation (2026-07-21).
	//
	// DrawActorOverlays (0x113aa6f0) and TGPostRenderFor (0x109e5f40) are
	// left DISABLED for now: both addresses are leftovers from an earlier,
	// unverified session (never re-confirmed against the current binary),
	// and re-enabling them alongside IsFriendlyWithLocalPawn produced an
	// immediate STATUS_ILLEGAL_INSTRUCTION (0xc000001d) crash in DINPUT8.dll
	// on load — the classic signature of DetourAttach patching a stale/wrong
	// address's prologue. DrawActorOverlays in particular fires every frame
	// during normal HUD rendering, which fits a crash this immediate far
	// better than IsFriendlyWithLocalPawn (only called when viewing a pawn).
	//
	// IsFriendlyWithLocalPawn's address was freshly re-verified in Ghidra
	// this session (decompiled cleanly, logic matches expectations) — but
	// hooking it ALONE still produced the same STATUS_ILLEGAL_INSTRUCTION
	// crash, with a confirmed-matching exe version (ruled out via file
	// properties, 2026-07-21). Address is correct; the problem is specific
	// to Detours patching this particular function's prologue (likely an
	// instruction-length misjudgement in the trampoline). Disabled pending
	// a raw disassembly check of the first ~16 bytes at 0x109e5ac0.
	// TgHUD_Game__DrawActorOverlays::Install();
	// TgPawn__TGPostRenderFor::Install();
	// TgPawn__IsFriendlyWithLocalPawn::Install();

	::DetourTransactionCommit();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
			CreateThread( 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ModuleThread), 0, 0, 0 );

			DebugWindow::WindowTitle = "CLIENT";
			CreateThread( 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(DebugWindow::ModuleThread), 0, 0, 0 );
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
