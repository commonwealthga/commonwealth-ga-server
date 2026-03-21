#include "src/GameServer/Engine/SeqActNullGuard/SeqActNullGuard.hpp"

void __fastcall SeqActNullGuard::Call(void* op, void* edx) {
	// GEngine->LocalPlayers (TArray at GEngine+0x32C) is always empty on a
	// dedicated server.  The original function dereferences LocalPlayers.Data
	// to get LocalPlayers[0] without guarding against an empty array → crash.
	// This op is client-only; return immediately.
}
