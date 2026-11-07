#include "src/GameServer/Utils/FramePacing/FramePacing.hpp"

// pch.hpp, not <windows.h> directly — it pulls winsock2 in first.
#include "src/pch.hpp"

#include <wchar.h>

#include "src/Utils/Logger/Logger.hpp"

namespace FramePacing {

namespace {

// Delay that replaces a game-thread Sleep(0). The engine's busy-wait covers
// roughly the last 2 ms of each frame, so the frame ends at most this much late.
constexpr LONGLONG kYieldDelayUs = 250;

typedef LONG(NTAPI* NtDelayExecution_t)(BOOLEAN, PLARGE_INTEGER);

VOID(WINAPI* g_TrueSleep)(DWORD) = Sleep;
NtDelayExecution_t g_NtDelayExecution = nullptr;
volatile DWORD g_GameThreadId = 0;

VOID WINAPI HookSleep(DWORD Milliseconds) {
	if (Milliseconds == 0 && g_GameThreadId != 0 && GetCurrentThreadId() == g_GameThreadId) {
		LARGE_INTEGER Delay;
		Delay.QuadPart = -kYieldDelayUs * 10;  // relative, 100 ns units
		g_NtDelayExecution(FALSE, &Delay);
		return;
	}
	g_TrueSleep(Milliseconds);
}

}  // namespace

void Install() {
	if (wcsstr(GetCommandLineW(), L"-noframepacing")) {
		Logger::Log("perf", "[FramePacing] off (-noframepacing)\n");
		return;
	}
	HMODULE Ntdll = GetModuleHandleA("ntdll.dll");
	// Wine only: there a short NtDelayExecution is a precise local wait. On
	// Windows (player-hosted missions) it rounds up to the timer resolution
	// (up to ~15.6 ms) and would make frames late.
	if (!GetProcAddress(Ntdll, "wine_get_version")) {
		return;
	}
	g_NtDelayExecution = reinterpret_cast<NtDelayExecution_t>(GetProcAddress(Ntdll, "NtDelayExecution"));
	if (!g_NtDelayExecution) {
		Logger::Log("perf", "[FramePacing] NtDelayExecution not found, off\n");
		return;
	}
	::DetourAttach(&(PVOID&)g_TrueSleep, (PVOID)&HookSleep);
}

void NoteGameThread() {
	if (g_GameThreadId == 0) {
		g_GameThreadId = GetCurrentThreadId();
	}
}

}  // namespace FramePacing
