#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn_Character__ServerPurchaseItem : public HookBase<
	void(__fastcall*)(ATgPawn_Character*, void*, int, int, int, int, int),
	0x109c0390,
	TgPawn_Character__ServerPurchaseItem> {
public:
	static void __fastcall Call(ATgPawn_Character* Pawn, void* edx, int nLootTableId, int nLootTableItemId, int nItemCount, int nAttemptedCurrencyType, int nAttemptedCostPerItem);
	static inline void __fastcall CallOriginal(ATgPawn_Character* Pawn, void* edx, int nLootTableId, int nLootTableItemId, int nItemCount, int nAttemptedCurrencyType, int nAttemptedCostPerItem) {
		m_original(Pawn, edx, nLootTableId, nLootTableItemId, nItemCount, nAttemptedCurrencyType, nAttemptedCostPerItem);
	};
};
