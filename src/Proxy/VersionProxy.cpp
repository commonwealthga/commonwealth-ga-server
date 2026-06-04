#include <windows.h>
#include <winver.h>

namespace {

HMODULE GetSystemVersionDll() {
	static HMODULE module = []() -> HMODULE {
		char systemDir[MAX_PATH] = {};
		const UINT len = GetSystemDirectoryA(systemDir, MAX_PATH);
		if (len == 0 || len >= MAX_PATH) return nullptr;

		char path[MAX_PATH] = {};
		const int written = wsprintfA(path, "%s\\version.dll", systemDir);
		if (written <= 0 || written >= MAX_PATH) return nullptr;

		return LoadLibraryA(path);
	}();
	return module;
}

FARPROC ResolveVersionExport(const char* name) {
	HMODULE module = GetSystemVersionDll();
	return module ? GetProcAddress(module, name) : nullptr;
}

template <typename Fn>
Fn ResolveVersionFn(const char* name) {
	return reinterpret_cast<Fn>(ResolveVersionExport(name));
}

} // namespace

extern "C" BOOL WINAPI GetFileVersionInfoA(LPCSTR fileName, DWORD handle, DWORD len, LPVOID data) {
	using Fn = BOOL (WINAPI*)(LPCSTR, DWORD, DWORD, LPVOID);
	Fn fn = ResolveVersionFn<Fn>("GetFileVersionInfoA");
	return fn ? fn(fileName, handle, len, data) : FALSE;
}

extern "C" BOOL WINAPI GetFileVersionInfoW(LPCWSTR fileName, DWORD handle, DWORD len, LPVOID data) {
	using Fn = BOOL (WINAPI*)(LPCWSTR, DWORD, DWORD, LPVOID);
	Fn fn = ResolveVersionFn<Fn>("GetFileVersionInfoW");
	return fn ? fn(fileName, handle, len, data) : FALSE;
}

extern "C" BOOL WINAPI GetFileVersionInfoExA(DWORD flags, LPCSTR fileName, DWORD handle, DWORD len, LPVOID data) {
	using Fn = BOOL (WINAPI*)(DWORD, LPCSTR, DWORD, DWORD, LPVOID);
	Fn fn = ResolveVersionFn<Fn>("GetFileVersionInfoExA");
	return fn ? fn(flags, fileName, handle, len, data) : FALSE;
}

extern "C" BOOL WINAPI GetFileVersionInfoExW(DWORD flags, LPCWSTR fileName, DWORD handle, DWORD len, LPVOID data) {
	using Fn = BOOL (WINAPI*)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);
	Fn fn = ResolveVersionFn<Fn>("GetFileVersionInfoExW");
	return fn ? fn(flags, fileName, handle, len, data) : FALSE;
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR fileName, LPDWORD handle) {
	using Fn = DWORD (WINAPI*)(LPCSTR, LPDWORD);
	Fn fn = ResolveVersionFn<Fn>("GetFileVersionInfoSizeA");
	return fn ? fn(fileName, handle) : 0;
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeW(LPCWSTR fileName, LPDWORD handle) {
	using Fn = DWORD (WINAPI*)(LPCWSTR, LPDWORD);
	Fn fn = ResolveVersionFn<Fn>("GetFileVersionInfoSizeW");
	return fn ? fn(fileName, handle) : 0;
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeExA(DWORD flags, LPCSTR fileName, LPDWORD handle) {
	using Fn = DWORD (WINAPI*)(DWORD, LPCSTR, LPDWORD);
	Fn fn = ResolveVersionFn<Fn>("GetFileVersionInfoSizeExA");
	return fn ? fn(flags, fileName, handle) : 0;
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeExW(DWORD flags, LPCWSTR fileName, LPDWORD handle) {
	using Fn = DWORD (WINAPI*)(DWORD, LPCWSTR, LPDWORD);
	Fn fn = ResolveVersionFn<Fn>("GetFileVersionInfoSizeExW");
	return fn ? fn(flags, fileName, handle) : 0;
}

extern "C" BOOL WINAPI GetFileVersionInfoByHandle(DWORD handle, DWORD offset, DWORD len, LPVOID data) {
	using Fn = BOOL (WINAPI*)(DWORD, DWORD, DWORD, LPVOID);
	Fn fn = ResolveVersionFn<Fn>("GetFileVersionInfoByHandle");
	return fn ? fn(handle, offset, len, data) : FALSE;
}

extern "C" DWORD WINAPI VerFindFileA(
	DWORD flags,
	LPSTR fileName,
	LPSTR winDir,
	LPSTR appDir,
	LPSTR curDir,
	PUINT curDirLen,
	LPSTR destDir,
	PUINT destDirLen) {
	using Fn = DWORD (WINAPI*)(DWORD, LPSTR, LPSTR, LPSTR, LPSTR, PUINT, LPSTR, PUINT);
	Fn fn = ResolveVersionFn<Fn>("VerFindFileA");
	return fn ? fn(flags, fileName, winDir, appDir, curDir, curDirLen, destDir, destDirLen) : 0;
}

extern "C" DWORD WINAPI VerFindFileW(
	DWORD flags,
	LPWSTR fileName,
	LPWSTR winDir,
	LPWSTR appDir,
	LPWSTR curDir,
	PUINT curDirLen,
	LPWSTR destDir,
	PUINT destDirLen) {
	using Fn = DWORD (WINAPI*)(DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT, LPWSTR, PUINT);
	Fn fn = ResolveVersionFn<Fn>("VerFindFileW");
	return fn ? fn(flags, fileName, winDir, appDir, curDir, curDirLen, destDir, destDirLen) : 0;
}

extern "C" DWORD WINAPI VerInstallFileA(
	DWORD flags,
	LPSTR srcFileName,
	LPSTR destFileName,
	LPSTR srcDir,
	LPSTR destDir,
	LPSTR curDir,
	LPSTR tmpFile,
	PUINT tmpFileLen) {
	using Fn = DWORD (WINAPI*)(DWORD, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, PUINT);
	Fn fn = ResolveVersionFn<Fn>("VerInstallFileA");
	return fn ? fn(flags, srcFileName, destFileName, srcDir, destDir, curDir, tmpFile, tmpFileLen) : 0;
}

extern "C" DWORD WINAPI VerInstallFileW(
	DWORD flags,
	LPWSTR srcFileName,
	LPWSTR destFileName,
	LPWSTR srcDir,
	LPWSTR destDir,
	LPWSTR curDir,
	LPWSTR tmpFile,
	PUINT tmpFileLen) {
	using Fn = DWORD (WINAPI*)(DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT);
	Fn fn = ResolveVersionFn<Fn>("VerInstallFileW");
	return fn ? fn(flags, srcFileName, destFileName, srcDir, destDir, curDir, tmpFile, tmpFileLen) : 0;
}

extern "C" DWORD WINAPI VerLanguageNameA(DWORD type, LPSTR buffer, DWORD len) {
	using Fn = DWORD (WINAPI*)(DWORD, LPSTR, DWORD);
	Fn fn = ResolveVersionFn<Fn>("VerLanguageNameA");
	return fn ? fn(type, buffer, len) : 0;
}

extern "C" DWORD WINAPI VerLanguageNameW(DWORD type, LPWSTR buffer, DWORD len) {
	using Fn = DWORD (WINAPI*)(DWORD, LPWSTR, DWORD);
	Fn fn = ResolveVersionFn<Fn>("VerLanguageNameW");
	return fn ? fn(type, buffer, len) : 0;
}

extern "C" BOOL WINAPI VerQueryValueA(LPCVOID block, LPCSTR subBlock, LPVOID* buffer, PUINT len) {
	using Fn = BOOL (WINAPI*)(LPCVOID, LPCSTR, LPVOID*, PUINT);
	Fn fn = ResolveVersionFn<Fn>("VerQueryValueA");
	return fn ? fn(block, subBlock, buffer, len) : FALSE;
}

extern "C" BOOL WINAPI VerQueryValueW(LPCVOID block, LPCWSTR subBlock, LPVOID* buffer, PUINT len) {
	using Fn = BOOL (WINAPI*)(LPCVOID, LPCWSTR, LPVOID*, PUINT);
	Fn fn = ResolveVersionFn<Fn>("VerQueryValueW");
	return fn ? fn(block, subBlock, buffer, len) : FALSE;
}
