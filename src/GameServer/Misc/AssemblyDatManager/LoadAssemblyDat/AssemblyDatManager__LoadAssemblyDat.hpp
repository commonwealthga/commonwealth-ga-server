#pragma once

#include "src/Utils/HookBase.hpp"

class AssemblyDatManager__LoadAssemblyDat : public HookBase<
	int(__fastcall*)(void*),
	0x109A1280,
	AssemblyDatManager__LoadAssemblyDat> {
public:
    static int __fastcall Call(void* AssemblyDatManager);
	static inline int __fastcall CallOriginal(void* AssemblyDatManager) {
		return m_original(AssemblyDatManager);
	}
};

