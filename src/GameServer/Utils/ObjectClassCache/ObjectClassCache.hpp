#pragma once

#include "src/pch.hpp"

#include <string>
#include <unordered_map>

// Per-class name cache.
//
// The engine's `UObject::GetFullName()` returns a `const char*` into a shared
// static buffer that gets clobbered on the next `GetFullName()` call by ANY
// object (see memory note `reference_getfullname_pointer_reuse.md` and the
// project's hard rule "Copy GetFullName() to std::string IMMEDIATELY"). Two
// back-to-back `GetFullName()` calls in a single log line silently produce
// the same string for both args; comparisons against a `const char*` stash
// silently start matching against later-overwritten content.
//
// Most callers actually want the CLASS name (`obj->Class->GetFullName()`) and
// use it to test substring identity (e.g. `strstr(name, "TgPawn")`). Since
// every instance of a class shares the same `UClass*`, and class pointers
// don't move for the lifetime of the process, we can resolve the name ONCE
// per class and hand out a stable `const std::string&` thereafter. Cached
// lookups are a single `unordered_map<UClass*, std::string>::find` — no
// `GetFullName()` invocation, no shared-buffer hazard, no heap allocation.
//
// Single-threaded assumption (same as the rest of the DLL — UE3 game thread).
// `UClass*` keys are stable; entries are never erased. Process-lifetime cache.
//
// Usage:
//   const std::string& cn = ObjectClassCache::GetClassName(obj);
//   if (cn.find("TgPawn") != std::string::npos) { ... }
//   // or, when you already have the class:
//   const std::string& cn = ObjectClassCache::GetClassName(obj->Class);
namespace ObjectClassCache {

// Cached `cls->GetFullName()`. Returns a static "<null>" sentinel for nullptr
// input so callers can safely chain `.find(...)` without a separate null check.
const std::string& GetClassName(UClass* cls);

// Convenience: `GetClassName(obj->Class)` with a null-check on `obj` too.
const std::string& GetClassName(UObject* obj);

// Substring check against the cached class name. Equivalent to
// `GetClassName(obj).find(needle) != npos` but inlineable and saves a call
// site keystroke. `needle` is matched as a plain C-string substring.
bool ClassNameContains(UObject* obj, const char* needle);
bool ClassNameContains(UClass* cls, const char* needle);

}  // namespace ObjectClassCache
