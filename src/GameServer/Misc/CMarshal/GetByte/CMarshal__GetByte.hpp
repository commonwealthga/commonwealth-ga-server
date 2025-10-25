#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

struct DeviceIdSlotPair {
	int DeviceId;
	int SlotUsedValueId;
};

class CMarshal__GetByte : public HookBase<
	int(__fastcall*)(void*, void*, int, uint32_t*),
	0x10938090,
	CMarshal__GetByte> {
public:
	static bool bIsLoadingBots;
	static std::map<int, uint32_t> m_values;
	static std::map<int, std::vector<DeviceIdSlotPair>> m_botDevices;
	static int __fastcall Call(void* CMarshal, void* edx, int Field, uint32_t* Out);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, int Field, uint32_t* Out) {
		return m_original(CMarshal, edx, Field, Out);
	};
};

