#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UndefinedFunction_10a8b090 — Activated() of an unidentified TgSeqAct that
// reads GEngine->LocalPlayers[0] (GEngine+0x32C / +0x330).  On a dedicated
// server LocalPlayers is empty, so the array Data pointer is null → crash at
// 0x10a8b107.  The op is client-only; skip it unconditionally on the server.
class SeqActNullGuard : public HookBase<
	void(__fastcall*)(void*, void*),
	0x10a8b090,
	SeqActNullGuard> {
public:
	static void __fastcall Call(void* op, void* edx);
	static inline void __fastcall CallOriginal(void* op, void* edx) {
		m_original(op, edx);
	};
};
