#pragma once

#include "src/pch.hpp"

class Logger {
public:
	// ---- Init API (call once per channel at DLL init) ----------------------
	//
	// `EnableChannel` marks a channel as file-logged (writes to
	// `<LogDir>/<channel>.txt`). `EnableCrashChannel` marks it as crash-only
	// (recorded into an in-memory ring buffer, flushed by CrashHandler — zero
	// disk I/O during normal operation; use for high-volume diagnostics).
	//
	// A channel can be in both lists. Channels enabled after a call site has
	// already cached its state pointer take effect immediately — the cached
	// pointer references the same state object that EnableChannel mutates.
	static void EnableChannel(const std::string& name);
	static void EnableCrashChannel(const std::string& name);

	// ---- Hot-path API ------------------------------------------------------
	//
	// All three resolve the channel through a pointer-keyed cache: same
	// string-literal address → O(1) hash hit, no string compares or std::string
	// heap allocations. First sight of a given `const char*` value triggers a
	// one-shot string lookup that canonicalizes into the per-name state map
	// and caches the pointer. Channel names are expected to be string
	// literals; passing a transient pointer is safe but pays the slow path
	// every call (the cache key would never repeat).
	//
	// `IsChannelEnabled` returns true if either file OR crash recording is
	// active for the channel. Use it to gate expensive log-arg construction
	// — C++ evaluates variadic args before the callee can check the gate,
	// so calls like `Log(ch, "%s", BuildBigString())` still run BuildBigString
	// even when the channel is off. Cheap-arg sites can just call Log
	// directly; it has the same gate internally.
	//
	// `IndentChannel` adjusts the per-channel indent depth used by file/ring
	// output to render the call-tree visualization. Stored on the state
	// object — no `std::map<std::string,int>` lookup per increment.
	static bool IsChannelEnabled(const char* channel);
	static void Log(const char* channel, const char* format, ...);
	static void IndentChannel(const char* channel, int delta);

	// ---- Diagnostic / lifecycle -------------------------------------------
	static void DumpMemory(const char* channel, void* address, int size, int negativeSize = 0);
	static void DumpCrashBuffer(void* fileHandle);

	// Directory written to by per-channel file logging. Files are named
	// "<LogDir>\<channel>.txt". Default "C:" preserves the historical
	// "C:\<channel>.txt" location inside the Wine prefix's drive_c. Set via
	// Config::GetLogDir() at DLL init; do NOT include a trailing slash.
	static std::string LogDir;

	// Ensure the LogDir exists on disk (CreateDirectoryA, idempotent). Must
	// be called once at DLL init after `Logger::LogDir = Config::GetLogDir()`
	// and before any logging or ClearEnabledChannelFiles. fopen("a") does NOT
	// auto-create parent directories, so if the spawner-supplied
	// `<base>\<instance_id>` subdir doesn't exist yet, every channel write
	// would silently fail. Only handles the leaf; parents (e.g. "C:") must
	// already exist.
	static void EnsureLogDirExists();

	// Truncate every <LogDir>/<channel>.txt for each currently-file-enabled
	// channel. Intended to be called once at DLL init when the `-clearlogs=1`
	// switch is set, so repeated test runs don't pile new entries on top of
	// stale ones. Crash-only channels are left alone (they only ever land in
	// the in-memory ring, never on disk during normal ops).
	static void ClearEnabledChannelFiles();

	static inline const char* GetTime() {
		auto now = std::chrono::system_clock::now();
		std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

		std::tm tm;
#if defined(_WIN32) || defined(_WIN64)
		localtime_s(&tm, &now_time_t);
#else
		localtime_r(&now_time_t, &tm);
#endif

		static thread_local char buf[32];
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
		return buf;
	}
};
