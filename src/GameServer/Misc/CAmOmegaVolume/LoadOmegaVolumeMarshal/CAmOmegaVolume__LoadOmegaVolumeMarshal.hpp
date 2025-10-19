#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CAmOmegaVolume__LoadOmegaVolumeMarshal : public HookBase<
	void(__fastcall*)(void*, void*, void*),
	0x109468d0,
	CAmOmegaVolume__LoadOmegaVolumeMarshal> {
public:
	static std::map<uint32_t, int> m_OmegaVolumePointers;
	static void __fastcall Call(void* CAmOmegaVolumeRow, void* edx, void* Marshal);
	static inline void __fastcall CallOriginal(void* CAmOmegaVolumeRow, void* edx, void* Marshal) {
		m_original(CAmOmegaVolumeRow, edx, Marshal);
	};
};

