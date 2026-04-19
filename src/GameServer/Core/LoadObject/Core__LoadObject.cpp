#include "src/GameServer/Core/LoadObject/Core__LoadObject.hpp"
#include "src/GameServer/TgAssemblyMisc/LoadAssetRefs/TgAssemblyMisc__LoadAssetRefs.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstdio>
#include <cwchar>

// Narrow-render a wide string for logging. Truncates to fit the buffer.
static const char* WToA(wchar_t* w, char* buf, size_t bufSize) {
	if (!w) {
		snprintf(buf, bufSize, "<null>");
		return buf;
	}
	size_t n = wcslen(w);
	if (n >= bufSize) n = bufSize - 1;
	size_t i = 0;
	for (; i < n; ++i) {
		wchar_t c = w[i];
		buf[i] = (c > 0 && c < 128) ? (char)c : '?';
	}
	buf[i] = '\0';
	return buf;
}

void* __cdecl Core__LoadObject::Call(void* ObjectClass, wchar_t* InOuter, wchar_t* InName,
	wchar_t* InFilename, unsigned LoadFlags, int* Sandbox, int IsVerifyImport)
{
	void* result = CallOriginal(ObjectClass, InOuter, InName, InFilename, LoadFlags, Sandbox, IsVerifyImport);

	// Two conditions trigger a log: a null return always, or any return
	// while nested inside TgAssemblyMisc::LoadAssetRefs. The null case is
	// the immediate bug; the nested case gives us full per-asset tracing
	// for that specific code path so we can see which refs succeed and
	// which fail together.
	bool nested = (TgAssemblyMisc__LoadAssetRefs::s_nestLevel > 0);
	bool failed = (result == nullptr);

	if (nested || failed) {
		char nameBuf[256];
		char outerBuf[128];
		char fileBuf[128];
		const char* nameStr  = WToA(InName,     nameBuf,  sizeof(nameBuf));
		const char* outerStr = WToA(InOuter,    outerBuf, sizeof(outerBuf));
		const char* fileStr  = WToA(InFilename, fileBuf,  sizeof(fileBuf));

		const char* classFull = "<null>";
		if (ObjectClass) {
			UObject* cls = (UObject*)ObjectClass;
			const char* f = cls->GetFullName();
			if (f) classFull = f;
		}

		Logger::Log("asset_load",
			"[LoadObject] %s class='%s' name='%s' outer='%s' file='%s' flags=0x%x verify=%d nested=%d\n",
			failed ? "FAIL→null" : "ok",
			classFull, nameStr, outerStr, fileStr,
			LoadFlags, IsVerifyImport, (int)nested);
	}

	// Substitute a safe non-null sentinel when LoadObject fails while we're
	// nested inside TgAssemblyMisc::LoadAssetRefs (FUN_10a2bc70). That native
	// calls CrashProgram("newObj", "TgAssemblyMisc.cpp", 253) unconditionally
	// when LoadObject returns null — which kills the server every time a
	// deployable's form references a client-only particle/mesh asset (e.g.
	// "DEV_EFX_GraphicEFX.PART_DEV_EFX_Cured_tag" for medical stations).
	//
	// The loaded pointer is stored into the form's referencer array via
	// FUN_10c89a80 and never dereferenced for gameplay on the server — the
	// form's asset-ref list is a client-render / GC-keepalive mechanism.
	// Substituting `ObjectClass` (a UClass*, always non-null, always a real
	// UObject) keeps the referencer array well-formed without pretending a
	// particle system exists.
	if (nested && failed) {
		Logger::Log("asset_load",
			"[LoadObject] SUBSTITUTE class-ptr for nested null-load (avoids CrashProgram newObj)\n");
		return ObjectClass;
	}

	return result;
}
