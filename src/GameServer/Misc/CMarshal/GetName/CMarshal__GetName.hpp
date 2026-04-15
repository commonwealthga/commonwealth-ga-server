#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Game-side sig: int __cdecl/__fastcall (void* marshal, int field, undefined4 out)
// Internally wraps CMarshal__get_wchar_2 + converts wchar -> FString handle stored
// at `out`. The wchar capture is covered by the existing GetString2 hook; this hook
// exists primarily for symmetry + optional logging.
class CMarshal__GetName : public HookBase<
	int(__stdcall*)(void*, int, void*),
	0x1093d590,
	CMarshal__GetName> {
public:
	static bool bLogEnabled;
	static int __stdcall Call(void* CMarshal, int Field, void* Out);
	static inline int __stdcall CallOriginal(void* CMarshal, int Field, void* Out) {
		return m_original(CMarshal, Field, Out);
	};
};
