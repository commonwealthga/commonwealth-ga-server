#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"

namespace ObjectClassCache {

namespace {
	std::unordered_map<UClass*, std::string> g_classNames;
	const std::string g_nullName = "<null>";
}  // namespace

const std::string& GetClassName(UClass* cls) {
	if (!cls) return g_nullName;

	auto it = g_classNames.find(cls);
	if (it != g_classNames.end()) return it->second;

	// Miss: pay the GetFullName() walk once, copy into the cache, return the
	// stored copy by const ref. The raw `const char*` from GetFullName() must
	// NOT escape this function — by the time the caller looks at it, the
	// shared static buffer may have been overwritten by some other
	// GetFullName() call.
	const char* raw = cls->GetFullName();
	auto [insIt, inserted] = g_classNames.emplace(cls, raw ? raw : "<null>");
	return insIt->second;
}

const std::string& GetClassName(UObject* obj) {
	if (!obj) return g_nullName;
	return GetClassName(obj->Class);
}

bool ClassNameContains(UClass* cls, const char* needle) {
	if (!needle) return false;
	return GetClassName(cls).find(needle) != std::string::npos;
}

bool ClassNameContains(UObject* obj, const char* needle) {
	if (!obj || !needle) return false;
	return GetClassName(obj->Class).find(needle) != std::string::npos;
}

}  // namespace ObjectClassCache
