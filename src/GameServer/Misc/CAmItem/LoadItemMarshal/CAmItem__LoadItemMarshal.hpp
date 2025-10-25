#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CAmItem__LoadItemMarshal : public HookBase<
	void(__fastcall*)(void*, void*, void*),
	0x1094f1b0,
	CAmItem__LoadItemMarshal> {
public:
	static bool bPopulateDatabaseItems;
	static std::map<uint32_t, int> m_ItemPointers;
	static void __fastcall Call(void* CAmItemRow, void* edx, void* Marshal);
	static inline void __fastcall CallOriginal(void* CAmItemRow, void* edx, void* Marshal) {
		m_original(CAmItemRow, edx, Marshal);
	};
};

