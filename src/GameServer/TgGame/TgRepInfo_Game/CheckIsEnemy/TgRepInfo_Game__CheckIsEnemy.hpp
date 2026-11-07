#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgRepInfo_Game::CheckIsEnemy (vtable slot 0x374, behind Actor.IsEnemy).
// Two natives share the slot: TgRepInfo_Game @ 0x109ee170 (task force compare)
// and TgRepInfo_GameOpenWorld @ 0x109ee250 (GetCoalitionFor compare). Both
// return false for two AI pawns that resolve to the same side, so pairs of
// bots that BotSight marks hostile are answered here; everything else goes
// to the original.
//   int __thiscall CheckIsEnemy(ATgRepInfo_Game* this, AActor* a, AActor* b)
class TgRepInfo_Game__CheckIsEnemy : public HookBase<
	int(__fastcall*)(ATgRepInfo_Game*, void*, AActor*, AActor*),
	0x109ee170,
	TgRepInfo_Game__CheckIsEnemy> {
public:
	static int __fastcall Call(ATgRepInfo_Game* pThis, void* edx, AActor* a, AActor* b);
	static inline int __fastcall CallOriginal(ATgRepInfo_Game* pThis, void* edx, AActor* a, AActor* b) {
		return m_original(pThis, edx, a, b);
	}
};

class TgRepInfo_GameOpenWorld__CheckIsEnemy : public HookBase<
	int(__fastcall*)(ATgRepInfo_Game*, void*, AActor*, AActor*),
	0x109ee250,
	TgRepInfo_GameOpenWorld__CheckIsEnemy> {
public:
	static int __fastcall Call(ATgRepInfo_Game* pThis, void* edx, AActor* a, AActor* b);
	static inline int __fastcall CallOriginal(ATgRepInfo_Game* pThis, void* edx, AActor* a, AActor* b) {
		return m_original(pThis, edx, a, b);
	}
};
