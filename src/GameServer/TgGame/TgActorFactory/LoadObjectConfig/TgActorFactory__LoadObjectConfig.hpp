#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Native `Function TgGame.TgActorFactory.LoadObjectConfig` at 0x10a8c240 — the
// base class entrypoint that fires for every TgActorFactory subclass (including
// TgBeaconFactory). The subclass-specific LoadObjectConfig hooks (TgBotFactory
// at 0x10a8c2d0, etc.) sit alongside this one; this base hook covers the cases
// where the subclass has no dedicated implementation.
//
// We use it to layer map_object_config overrides onto TgBeaconFactory's
// m_nPriority (offset 0x01F8), which the editor-baked priority would otherwise
// lock in.
class TgActorFactory__LoadObjectConfig : public HookBase<
	void(__fastcall*)(ATgActorFactory*, void*),
	0x10a8c240,
	TgActorFactory__LoadObjectConfig> {
public:
	static void __fastcall Call(ATgActorFactory* Factory, void* edx);
	static inline void __fastcall CallOriginal(ATgActorFactory* Factory, void* edx) {
		m_original(Factory, edx);
	}
};
