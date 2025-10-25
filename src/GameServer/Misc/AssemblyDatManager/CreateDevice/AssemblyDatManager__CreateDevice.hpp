#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class AssemblyDatManager__CreateDevice : public HookBase<
	ATgDevice*(__fastcall*)(void*, void*, int, void*),
	0x109a69c0,
	AssemblyDatManager__CreateDevice> {
public:
	static ATgDevice* __fastcall Call(void* AssemblyDatManager, void* edx, int nDeviceId, void* InventoryObject);
	static inline ATgDevice* __fastcall CallOriginal(void* AssemblyDatManager, void* edx, int nDeviceId, void* InventoryObject) {
		return m_original(AssemblyDatManager, edx, nDeviceId, InventoryObject);
	};
};

