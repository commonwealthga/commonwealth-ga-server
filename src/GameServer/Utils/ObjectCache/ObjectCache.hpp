#pragma once

#include "src/pch.hpp"

// Lazy progressive cache over UObject::GObjObjects().
//
// On a cache miss, resumes scanning from where it last stopped, caching every
// object it passes. This means early lookups (classes, properties) only scan
// as far as needed, and later lookups (map actors) pick up where the earlier
// ones left off. The full array is scanned at most once.
//
// If GObjObjects grows (new actors spawned after initial scan), subsequent
// lookups will scan the new portion automatically.
class ObjectCache {
private:
	// fullName -> UObject*  (exact match)
	static std::unordered_map<std::string, UObject*> ByFullName;

	// className -> vector of instances  (for FindAllByClass)
	static std::unordered_map<std::string, std::vector<UObject*>> ByClassName;

	// How far we've scanned so far
	static int LastIndex;

	// Scan forward from LastIndex, caching every object.
	// If targetFullName is non-null, stops as soon as it's found.
	// If targetFullName is null, scans to the current end of the array.
	static void ScanForward(const char* targetFullName);

public:
	// Exact lookup by GetFullName(). Lazy -- only scans as far as needed.
	static UObject* Find(const char* fullName);

	// All non-Default__ instances whose Class->GetFullName() matches exactly.
	// Forces a full scan on first call (since we can't know if more exist).
	// Subsequent calls only scan new objects (if the array grew).
	static const std::vector<UObject*>& FindAllByClass(const char* classFullName);

	// All non-Default__ instances whose Class->GetFullName() contains the substring.
	// Same scan behavior as FindAllByClass.
	// Results are returned in a static vector that is rebuilt each call.
	static std::vector<UObject*> FindAllByClassSubstr(const char* classSubstr);
};
