#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Think-Tank Array.h:560 client-crash guard (SERVER side).
//
// Every replicated UObject* reference serializes through
// UPackageMap::ObjectToIndex @ 0x11365260 as entry.ObjectBase + Obj->NetIndex,
// with no validation that the receiving client can actually create that
// export — the client's ULinkerLoad::CreateExport(localIndex) asserts
// Array.h:560 when the index is past its package's ExportMap (confirmed via
// client crash probe + this hook's trace: ThinkTank's shield-activate cue
// serialized with netIndex=46 into a 40-export package).
//
// The hook suppresses (serializes as NULL) any object whose NetIndex is past
// its own linker's ExportMap count — such an object cannot exist on the
// client's disk. Suppressions are logged on channel `packagemap`.
//
// Signature: int __thiscall UPackageMap::ObjectToIndex(UPackageMap*, UObject*)
class PackageMap__ObjectToIndex : public HookBase<
	int(__fastcall*)(void*, void*, UObject*),
	0x11365260,
	PackageMap__ObjectToIndex> {
public:
	static int __fastcall Call(void* PackageMap, void* edx, UObject* Obj);
	static inline int __fastcall CallOriginal(void* PackageMap, void* edx, UObject* Obj) {
		return m_original(PackageMap, edx, Obj);
	};
};
