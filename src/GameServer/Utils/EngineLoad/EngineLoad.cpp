#include "src/GameServer/Utils/EngineLoad/EngineLoad.hpp"

#include <string>

#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace EngineLoad {

namespace {

// UObject* StaticLoadClass(UClass* BaseClass, UObject* InOuter, const TCHAR* InName,
//                          const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox)
// .\Src\UnObj.cpp — the result must descend from BaseClass (LoadClassMismatch gate).
using StaticLoadClass_t = void* (__cdecl*)(void* BaseClass, const wchar_t* InOuter,
	const wchar_t* InName, const wchar_t* Filename, unsigned int LoadFlags, int* Sandbox);
constexpr uintptr_t kStaticLoadClassAddr = 0x112e55d0;

std::wstring Widen(const char* s) {
	std::wstring w;
	for (const char* p = s; p && *p; ++p) {
		w.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*p)));
	}
	return w;
}

} // namespace

UObject* LoadClassByName(const char* fullPath) {
	if (fullPath == nullptr || *fullPath == '\0') {
		return nullptr;
	}

	// Every class descends from Object, so the BaseClass gate always passes.
	void* baseClass = ObjectCache::Find("Class Core.Object");
	if (baseClass == nullptr) {
		Logger::Log("load",
			"[EngineLoad] LoadClassByName(\"%s\"): 'Class Core.Object' not resident; abort\n",
			fullPath);
		return nullptr;
	}

	const std::wstring name = Widen(fullPath);
	auto StaticLoadClass = reinterpret_cast<StaticLoadClass_t>(kStaticLoadClassAddr);
	void* cls = StaticLoadClass(baseClass, nullptr, name.c_str(), nullptr, 0, nullptr);

	Logger::Log("load", "[EngineLoad] StaticLoadClass(\"%s\") -> %p\n", fullPath, cls);
	return reinterpret_cast<UObject*>(cls);
}

UObject* PreloadClass(const char* fullPath) {
	UObject* cls = LoadClassByName(fullPath);
	if (cls != nullptr) {
		// RF_RootSet (0x4000) via the established ObjectFlags.A idiom — keep the
		// class (and, transitively, its CDO + UFunctions) alive across GC even
		// while nothing references it yet.
		cls->ObjectFlags.A |= 0x4000;
		Logger::Log("load", "[EngineLoad] PreloadClass(\"%s\") rooted -> %p\n", fullPath, (void*)cls);
	} else {
		Logger::Log("load", "[EngineLoad] PreloadClass(\"%s\") FAILED to load\n", fullPath);
	}
	return cls;
}

} // namespace EngineLoad
