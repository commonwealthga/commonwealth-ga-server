#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn_Character__SendCharacterSkillMarshal : public HookBase<
	void(__fastcall*)(ATgPawn_Character*, void*),
	0x109c0dc0,
	TgPawn_Character__SendCharacterSkillMarshal> {
public:
	static void __fastcall Call(ATgPawn_Character* Pawn, void* edx);
	static inline void __fastcall CallOriginal(ATgPawn_Character* Pawn, void* edx) {
		m_original(Pawn, edx);
	};
};
