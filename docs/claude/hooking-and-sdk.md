# Hooking & SDK guide

Patterns, gotchas, and rules of thumb for adding new hooks. Read alongside `CLAUDE.md` which holds the non-negotiable rules.

## HookBase pattern

Every native intercept lives in its own file pair under `src/GameServer/<Subsystem>/<Class>/<Native>/<Native>.{hpp,cpp}`.

```cpp
// FooBar__SomeNative.hpp
#pragma once
#include "src/pch.hpp"

class FooBar__SomeNative : public HookBase<
    decltype(&FooBar__SomeNative::Call),  // FuncType
    0x10aXXXXX,                           // address from Ghidra
    FooBar__SomeNative                    // CRTP self-type
> {
public:
    static int __fastcall Call(AFooBar* self, void* edx, int someArg);
};
```

```cpp
// FooBar__SomeNative.cpp
#include "FooBar__SomeNative.hpp"

int __fastcall FooBar__SomeNative::Call(AFooBar* self, void* edx, int someArg) {
    // your impl, optionally:
    return CallOriginal(self, edx, someArg);
}
```

Don't forget to add the `.cpp` to `Makefile`.

## ParentHook::Call vs. CallOriginal — the stub trap

**The single most-burned rule.** Re-derived below because it's load-bearing for the entire effect/buff/property rewrite.

- **If the hooked native is INTACT** in the binary: `CallOriginal(self, edx, ...)` works as expected — it dispatches to the original implementation, which itself handles vtable chaining to overrides.
- **If the hooked native is a STUB** (no-op `ret 0`): `CallOriginal` is also a no-op. It does NOT chain to parent class overrides via the vtable. The override on the parent class will be silently skipped.

When you hook a stub on a derived class (e.g. `ATgBotFactory`-derived) and the parent class (`ATgActorFactory`) has the real implementation, you must dispatch to the parent's hook explicitly:

```cpp
ParentHook::Call((AParentType*)derived, edx, ...);
```

Decision is per-hook. Check whether the binary's native at the hooked address actually does work, or whether it's just `ret 0` / `xor eax,eax; ret`. If it's stripped, go via the parent hook.

Historical example: 2026-05-24 BotFactory refactor used `CallOriginal` instead of `ParentHook::Call`. Every map spawned empty because `TgActorFactory::SpawnNextBot` (the real impl, on the parent class) never ran. The derived stub's `CallOriginal` is the stub itself.

## MSVC struct-return convention

Hooks on natives returning `FVector` / `FRotator` / any non-trivial struct: the MSVC calling convention takes an implicit pointer-to-out-buffer as the first arg AFTER `this`. From a hook's perspective, you must:

1. Declare the return type as `OutType*` (the out-buffer pointer).
2. `return outLoc;` at EVERY exit path.

If you declare `void` return, EAX is left with junk, the caller dereferences it, and you get a null-deref crash (or worse, write-to-arbitrary-address).

Burned 2026-05-14 on the Shrike trace hook.

## FName dispatch & ProcessEvent

Native → UC `ProcessEvent` calls encode the target UFunction via the global FName cache. When you see a `ProcessEvent` site at some address, the pattern is:

```
mov     eax, dword ptr [DAT_119aXXXX]   ; FName lookup, points into GNames
push    eax
... (push other args) ...
call    UObject::ProcessEvent
```

Resolve the `DAT_` symbol in Ghidra to find which UFunction is being dispatched. If you don't have access to Ghidra or the FName→string table, ask the human for the dispatch target.

ProcessEvent intercepts run BEFORE UC bytecode — pre-zeroing state in a hook can break `ApplyToProperty`-style UC code that computes `m_fRaw - m_fCurrent`. Let UC zero its own state after the bytecode runs.

UC-to-UC calls in the SAME UC package can bypass `ProcessEvent` (the VM uses `EX_FinalFunction` for intra-package virtual calls). This is why some intercept attempts silently fail — the call never goes through `ProcessEvent` at all. `CheckEffectBuffModifier` and `TakeDamage` are both UC-to-UC dispatched and not interceptable via `ProcessEvent`. Solve those by patching the underlying data the UC code reads (e.g. pre-scale `TgEffectDamage`'s `m_fBase` in `CloneEffectGroup`).

## SDK pitfalls

### `IsA()` is unreliable

`IsA(SomeSDK::StaticClass())` returns the wrong answer on this binary because SDK-generated `GObjObjects` indices don't match the binary's runtime indices.

Use `ObjectClassCache::ClassNameContains(obj, "TgPawn")` or `GetClassName(obj)` for substring/exact-match dispatch.

### `StaticClass()` indices are misaligned

For the same reason as above, never use `SomeSDK::StaticClass()` to get a `UClass*`. Use:

```cpp
UClass* cls = ClassPreloader::GetClass("Class TgGame.TgPawn");
```

### `GetFullName()` shares a static buffer

`obj->GetFullName()` returns `const char*` pointing into a process-wide static buffer that the NEXT `GetFullName()` call (from ANY object) clobbers. Two `GetFullName()` calls in the same log line silently produce the same string for both args.

**Always copy to `std::string` immediately:**

```cpp
const std::string name(raw ? raw : "<null>");
```

Or — better — use `ObjectClassCache::GetClassName(obj)` which returns a cached `const std::string&` keyed by `UClass*`.

### FString allocator hazard

The SDK `FString` ctor/destructor allocates via the SDK's allocator, but engine code expects `GAllocator`. Mixing → double-free crashes.

Rules:
- Do NOT add an `operator=` or copy ctor to `FString`. A class-wide fix was attempted 2026-05-22 and reverted same day — the SDK `memcpy`s parm structs around and immediately double-frees.
- At handoff sites, allocate `fs.Data` via `GAllocator::Malloc`, write the field manually, and null `Data` before scope exit so the SDK destructor doesn't free what `GAllocator` owns.
- Crash signature: write-at-0 at `0x1090b4dc`, EAX=0 — that's the FString destructor stomping freed memory.

### TArray<T>: `Clear()` not `Empty()`

The SDK `TArray<T>` exposes:
- `Clear()` — frees `.Data` via `GAllocator`
- `Add(elem)` — Realloc via `GAllocator`
- `Num()` — element count
- `operator()(i)` — index

There is no `Empty()`. Never call `libc free` / `realloc` on `.Data` — allocator mixing.

Use the `TARRAY_INIT` / `TARRAY_ADD` macros from `src/Utils/Macros.hpp` for hot paths. They do direct malloc with no game function calls. Game `TArray` grow functions have unknown calling conventions and crash.

### SDK bitfield params bug

SDK `execX_Parms` structs that declare multiple `unsigned long x:1` bitfields at spaced offsets pack wrong. The generator misaligns them. When you encounter "param value reads as garbage" inside a `ProcessEvent` intercept, manually lay out the parm struct as `dword`s and read individual bits.

### `eventXxx()` wrappers resolve to the BASE class

The SDK generates per-method `eventFoo()` wrappers that resolve at compile time to `TgEffect::eventFoo` even when called on a `TgEffectBuff*`. They do NOT virtual-dispatch.

This caused buff `Remove` to run base `TgEffect.Remove` (no-op) instead of `TgEffectBuff.Remove` (reverse aggregator) — buffs accumulated permanently.

When polymorphism matters, dispatch by `ObjectClassCache::GetClassName(obj)` and call the appropriate concrete-class wrapper.

### FName Number uninit

Latent SDK bug fixed 2026-04-16: FName constructors didn't zero the `Number` dword. Broke `SetTimer` lookup (revive timers). If you see FName mismatches that look like junk-in-upper-bits, suspect this. The fix is in repo.

## Vtable layout in Ghidra

Class vtables are stored as contiguous `int*` arrays at fixed binary addresses. To find a class's vtable:

1. Locate the class name string literal in Ghidra (e.g. `"TgPawn"`).
2. Walk xrefs from the literal to the class constructor (typically the only caller).
3. The constructor writes `*(int*)this = <vtable_addr>;` near the top.

Known vtables are listed at the bottom of the per-class technical docs at repo root, and in the project's local Ghidra rename inputs.

To find a specific method:

1. Find the SDK-generated `intA<Class>exec<Func>` string.
2. Xref it → the wrapper function that pushes the parms and calls the native.
3. The native address is the vtable slot the wrapper reads.

The `BatchRename.py` / `FindNativeImpls.py` workflow (locally available, ask the human if you don't have it) automates this.

## Working without Ghidra

You may not have Ghidra access. That's fine for most tasks — the heavy lifting is done. Patterns to use:

- Read existing similar hooks for shape (`src/GameServer/**/<NearbyNative>.cpp`).
- The repo's root-level `.md` files (`bots.md`, `cgameclient.md`, etc.) document many addresses and vtable layouts already.
- When you need a vtable offset or a function address that isn't documented, ask the human. They'll look it up.
- Never guess addresses. A wrong address detours over arbitrary code and corrupts the process.
