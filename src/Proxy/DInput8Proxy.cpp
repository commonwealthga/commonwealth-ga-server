#include <windows.h>
#include <dinput.h>

namespace {

HMODULE GetSystemDInput8() {
	static HMODULE module = []() -> HMODULE {
		char systemDir[MAX_PATH] = {};
		const UINT len = GetSystemDirectoryA(systemDir, MAX_PATH);
		if (len == 0 || len >= MAX_PATH) return nullptr;

		char path[MAX_PATH] = {};
		const int written = wsprintfA(path, "%s\\dinput8.dll", systemDir);
		if (written <= 0 || written >= MAX_PATH) return nullptr;

		return LoadLibraryA(path);
	}();
	return module;
}

FARPROC ResolveSystemExport(const char* name) {
	HMODULE module = GetSystemDInput8();
	return module ? GetProcAddress(module, name) : nullptr;
}

} // namespace

extern "C" HRESULT WINAPI DirectInput8Create(
	HINSTANCE hinst,
	DWORD dwVersion,
	REFIID riidltf,
	LPVOID* ppvOut,
	LPUNKNOWN punkOuter) {
	using Fn = HRESULT (WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
	Fn fn = reinterpret_cast<Fn>(ResolveSystemExport("DirectInput8Create"));
	return fn ? fn(hinst, dwVersion, riidltf, ppvOut, punkOuter) : E_FAIL;
}

extern "C" HRESULT WINAPI DllCanUnloadNow() {
	using Fn = HRESULT (WINAPI*)();
	Fn fn = reinterpret_cast<Fn>(ResolveSystemExport("DllCanUnloadNow"));
	return fn ? fn() : S_FALSE;
}

extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
	using Fn = HRESULT (WINAPI*)(REFCLSID, REFIID, LPVOID*);
	Fn fn = reinterpret_cast<Fn>(ResolveSystemExport("DllGetClassObject"));
	return fn ? fn(rclsid, riid, ppv) : CLASS_E_CLASSNOTAVAILABLE;
}

extern "C" HRESULT WINAPI DllRegisterServer() {
	using Fn = HRESULT (WINAPI*)();
	Fn fn = reinterpret_cast<Fn>(ResolveSystemExport("DllRegisterServer"));
	return fn ? fn() : E_FAIL;
}

extern "C" HRESULT WINAPI DllUnregisterServer() {
	using Fn = HRESULT (WINAPI*)();
	Fn fn = reinterpret_cast<Fn>(ResolveSystemExport("DllUnregisterServer"));
	return fn ? fn() : E_FAIL;
}

extern "C" const DIDATAFORMAT* WINAPI GetdfDIJoystick() {
	using Fn = const DIDATAFORMAT* (WINAPI*)();
	Fn fn = reinterpret_cast<Fn>(ResolveSystemExport("GetdfDIJoystick"));
	return fn ? fn() : nullptr;
}
