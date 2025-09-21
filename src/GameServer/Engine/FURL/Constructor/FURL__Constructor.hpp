#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class FURL__Constructor : public HookBase<
	void(__fastcall*)(void*, void*, void*, wchar_t*, int),
	0x10BB8100,
	FURL__Constructor> {
public:
	static void __fastcall Call(FURL* Url, void* edx, void* param_3, wchar_t* UrlString, int param_5);
	static inline void __fastcall CallOriginal(FURL* Url, void* edx, void* param_3, wchar_t* UrlString, int param_5) {
		return m_original(Url, edx, param_3, UrlString, param_5);
	}
};

