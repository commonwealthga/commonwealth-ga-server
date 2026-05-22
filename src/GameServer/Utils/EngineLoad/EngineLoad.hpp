#pragma once

class UObject;

namespace EngineLoad {

// Force-load a class by full object path (e.g. "TgGame.TgEffectBuff") via the
// engine's StaticLoadClass (0x112e55d0). UE3 creates package exports lazily —
// a script-only class that nothing on the server references is never
// instantiated, so it never enters GObjObjects even though its export exists
// (with RF_LoadForServer). This drives StaticLoadClass, which resolves the
// already-loaded package's linker and calls CreateExport for that export.
//
// Returns the resolved UClass* (nullptr on failure). Logs to the "load"
// channel. Safe to call repeatedly — returns the existing class once loaded.
UObject* LoadClassByName(const char* fullPath);

// Like LoadClassByName, but also marks the class RF_RootSet so GC never
// reclaims it. Use for classes the server must keep resident even before
// anything references them (a force-loaded, otherwise-unreferenced class can
// be collected — GObjFirstGCIndex=0 leaves no disregard-for-GC set). Idempotent.
UObject* PreloadClass(const char* fullPath);

} // namespace EngineLoad
