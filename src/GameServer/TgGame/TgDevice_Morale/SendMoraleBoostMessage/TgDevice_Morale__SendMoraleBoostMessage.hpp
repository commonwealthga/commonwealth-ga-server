#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgDevice_Morale::SendMoraleBoostMessage — called by UC `DeviceFiring::BeginState`
// on the server when a player activates their morale device. Address candidate
// from the TgDevice_Morale vtable scan (next-to-last slot); fallback candidate
// is 0x10a19bf0. Swap if the alert doesn't fire.
class TgDevice_Morale__SendMoraleBoostMessage : public HookBase<
	void(__fastcall*)(ATgDevice_Morale*, void*),
	0x10a19c00,
	TgDevice_Morale__SendMoraleBoostMessage> {
public:
	static void __fastcall Call(ATgDevice_Morale* Device, void* edx);
	static inline void __fastcall CallOriginal(ATgDevice_Morale* Device, void* edx) {
		m_original(Device, edx);
	}
};
