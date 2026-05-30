#pragma once

#include "src/Utils/HookBase.hpp"

// Hook the UE3 auto-crash-reporter's "fetch local player ID" helper at
// 0x1096d460. On a dedicated server there's no LocalPlayer, and the binary's
// own implementation crashes (reads `[iVar1+0x40]` on iVar1=0 when the lookup
// path early-exits — vtable[0x40] of a null PlayerController). That secondary
// crash was eating the primary crash dump in our VEH filter: the original
// fault would fire UE3's __try/__except in WinMain → auto-reporter →
// FUN_1096d460 → secondary AV that the VEH wrote out instead of the primary.
//
// Replacing the body with `return 0` removes the secondary crash and lets
// the primary crash dump reach disk. The auto-reporter just stringifies the
// returned int into the dump file — "0" is a fine value for "no local player".
class AutoReporter__GetLocalPlayerId : public HookBase<
	int(__cdecl*)(),
	0x1096d460,
	AutoReporter__GetLocalPlayerId> {
public:
	static int __cdecl Call();
};
