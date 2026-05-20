#include "src/Utils/Logger/Logger.hpp"

#include <atomic>
#include <unordered_map>

std::string Logger::LogDir = "C:";

// -----------------------------------------------------------------------------
// Per-channel state.
//
// The previous implementation kept three public containers — two
// `vector<string>` (enabled / crash-enabled) and one `map<string,int>` (indent)
// — and every `Log` call did:
//   - two linear scans over the vectors with `std::string == const char*`
//     comparisons (full strcmp at each element), and
//   - an implicit `std::string` heap-allocation for the indent map lookup
//     (`ChannelIndents[const char*]` constructs a temp std::string per key)
// per call. Even when the channel was disabled. Multiply by every hooked
// function call's `LogCallBegin/LogCallEnd` and you get hundreds of
// allocations + thousands of strcmps per game tick.
//
// The new design:
//   1. Each channel is a single `ChannelState` (file/crash flags + indent
//      counter) stored by string in `g_states`. Stable address — never erased,
//      so call-site state pointers are valid for the lifetime of the process.
//   2. A second map keyed by `const char*` pointer caches the resolved
//      ChannelState* per call-site literal. Same channel name from the same
//      translation unit always points to the same literal address (compiler
//      de-dupes string literals), so cached lookups are a single pointer-keyed
//      unordered_map hit.
//   3. First sight of a new `const char*` value (e.g. dynamic build of a
//      channel name) falls through to a string-keyed lookup that resolves to
//      the canonical state, then caches the pointer mapping for next time.
//
// All cache mutation is serialized by `g_log_cs` (already in use for file
// I/O). Reads from a ChannelState* are not synchronized — single-writer of
// flags at init time + monotonic `indent` counter make tearing benign.
// -----------------------------------------------------------------------------

namespace {

struct ChannelState {
	bool fileEnabled = false;
	bool crashEnabled = false;
	int indent = 0;
};

// Canonical-by-name. unordered_map node addresses are stable across rehashes,
// so &g_states[key] is a permanent handle for the lifetime of the process.
std::unordered_map<std::string, ChannelState> g_states;

// Pointer-keyed shortcut. Populated lazily on the first call seen for each
// const char* value. The pointer is the key, ChannelState* is the value.
std::unordered_map<const char*, ChannelState*> g_ptrCache;

CRITICAL_SECTION g_log_cs;
bool g_log_cs_init = false;

void EnsureCsInit() {
	if (!g_log_cs_init) {
		InitializeCriticalSection(&g_log_cs);
		g_log_cs_init = true;
	}
}

// Resolve the state pointer for a given channel name. Holds g_log_cs only
// during the cache mutation; the returned pointer is safe to read without the
// lock (state node addresses don't move; flag/indent fields are scalar).
ChannelState* GetState(const char* channel) {
	EnsureCsInit();
	EnterCriticalSection(&g_log_cs);

	auto pIt = g_ptrCache.find(channel);
	if (pIt != g_ptrCache.end()) {
		ChannelState* s = pIt->second;
		LeaveCriticalSection(&g_log_cs);
		return s;
	}

	// First time we've seen this const char* value. Canonicalize via the
	// string-keyed map (insert default-constructed state if missing), then
	// cache the pointer so future lookups skip the string compare entirely.
	auto& state = g_states[channel];  // creates default if missing
	g_ptrCache[channel] = &state;

	LeaveCriticalSection(&g_log_cs);
	return &state;
}

}  // namespace

// -----------------------------------------------------------------------------
// Per-channel in-memory ring buffers for crash-only channels.
//
// Each EnabledCrashChannels entry gets its own fixed-size ring, allocated
// lazily on first write. Per-channel rings prevent a high-frequency channel
// (like hook_calltree) from pushing out entries of low-frequency targeted
// channels (roster_walker, deploy_validate, etc.). Each ring holds the last
// N entries for that specific channel.
//
// Writes are lock-free via atomic fetch_add on the channel's own counter.
// A wraparound race can garble one entry — acceptable post-mortem.
// Ring allocation (first write for a channel) is serialized by a CS.
// -----------------------------------------------------------------------------

static constexpr size_t kMaxCrashChannels   = 16;
static constexpr size_t kCrashRingEntries   = 100;
static constexpr size_t kCrashRingEntrySize = 1024;

struct CrashRing {
	char                  name[64];
	std::atomic<uint32_t> nextIndex;
	char                  buffer[kCrashRingEntries * kCrashRingEntrySize];
};

static CrashRing        g_crashRings[kMaxCrashChannels];
static std::atomic<int> g_crashRingCount{0};
static CRITICAL_SECTION g_crashRingCs;
static bool             g_crashRingCsInit = false;

static CrashRing* FindCrashRing(const char* channel)
{
	const int count = g_crashRingCount.load(std::memory_order_acquire);
	for (int i = 0; i < count; ++i) {
		if (strcmp(g_crashRings[i].name, channel) == 0) {
			return &g_crashRings[i];
		}
	}
	return nullptr;
}

static CrashRing* GetOrCreateCrashRing(const char* channel)
{
	CrashRing* ring = FindCrashRing(channel);
	if (ring) return ring;

	if (!g_crashRingCsInit) {
		InitializeCriticalSection(&g_crashRingCs);
		g_crashRingCsInit = true;
	}

	EnterCriticalSection(&g_crashRingCs);
	// Re-check under lock: another thread may have created it meanwhile.
	ring = FindCrashRing(channel);
	if (ring) {
		LeaveCriticalSection(&g_crashRingCs);
		return ring;
	}

	const int idx = g_crashRingCount.load(std::memory_order_relaxed);
	if (idx >= (int)kMaxCrashChannels) {
		LeaveCriticalSection(&g_crashRingCs);
		return nullptr; // channel cap reached; silently drop
	}

	CrashRing* newRing = &g_crashRings[idx];
	strncpy(newRing->name, channel, sizeof(newRing->name) - 1);
	newRing->name[sizeof(newRing->name) - 1] = '\0';
	newRing->nextIndex.store(0, std::memory_order_relaxed);
	// Release store so other threads reading g_crashRingCount see the
	// initialized ring before they index into the array.
	g_crashRingCount.store(idx + 1, std::memory_order_release);

	LeaveCriticalSection(&g_crashRingCs);
	return newRing;
}

static void CrashRingAppend(const char* channel, int indent, const char* format, va_list args)
{
	CrashRing* ring = GetOrCreateCrashRing(channel);
	if (!ring) return;

	const uint32_t idx  = ring->nextIndex.fetch_add(1, std::memory_order_relaxed);
	char*          slot = &ring->buffer[(idx % kCrashRingEntries) * kCrashRingEntrySize];

	int pos = snprintf(slot, kCrashRingEntrySize, "[%s] [%s] ",
	                   Logger::GetTime(), channel);
	if (pos < 0 || (size_t)pos >= kCrashRingEntrySize - 1) {
		slot[kCrashRingEntrySize - 1] = '\0';
		return;
	}

	// Mirror the file-logger's "│  " indent so the ring buffer preserves the
	// call-tree visualization. Caller passes the indent it already read
	// off the channel state (one lookup amortized between ring + file paths).
	for (int i = 0; i < indent; ++i) {
		const int w = snprintf(slot + pos, kCrashRingEntrySize - (size_t)pos, "│  ");
		if (w < 0 || (size_t)(pos + w) >= kCrashRingEntrySize - 1) break;
		pos += w;
	}

	vsnprintf(slot + pos, kCrashRingEntrySize - (size_t)pos, format, args);
	slot[kCrashRingEntrySize - 1] = '\0';
}

// ---- Public API -------------------------------------------------------------

void Logger::EnableChannel(const std::string& name) {
	EnsureCsInit();
	EnterCriticalSection(&g_log_cs);
	g_states[name].fileEnabled = true;
	LeaveCriticalSection(&g_log_cs);
}

void Logger::EnableCrashChannel(const std::string& name) {
	EnsureCsInit();
	EnterCriticalSection(&g_log_cs);
	g_states[name].crashEnabled = true;
	LeaveCriticalSection(&g_log_cs);
}

bool Logger::IsChannelEnabled(const char* channel) {
	ChannelState* s = GetState(channel);
	return s->fileEnabled || s->crashEnabled;
}

void Logger::IndentChannel(const char* channel, int delta) {
	ChannelState* s = GetState(channel);
	s->indent += delta;
}

void Logger::Log(const char* channel, const char* format, ...) {
	ChannelState* s = GetState(channel);
	const bool toRing = s->crashEnabled;
	const bool toFile = s->fileEnabled;
	if (!toRing && !toFile) return;

	const int indent = s->indent;

	if (toRing) {
		va_list args;
		va_start(args, format);
		CrashRingAppend(channel, indent, format, args);
		va_end(args);
	}

	if (!toFile) return;

	EnterCriticalSection(&g_log_cs);

	char filename[512];
	snprintf(filename, sizeof(filename), "%s\\%s.txt", LogDir.c_str(), channel);

	FILE* fp = fopen(filename, "a");
	if (fp == NULL) {
		LeaveCriticalSection(&g_log_cs);
		return;
	}

	for (int i = 0; i < indent; i++) {
		fprintf(fp, "│  ");
	}

	va_list args;
	va_start(args, format);
	vfprintf(fp, format, args);
	va_end(args);

	fclose(fp);

	LeaveCriticalSection(&g_log_cs);
}

void Logger::EnsureLogDirExists() {
	// Idempotent: CreateDirectoryA returns false with GetLastError()=
	// ERROR_ALREADY_EXISTS when the dir is already there — fine, ignore.
	// Wine translates the Win32 path (e.g. "C:\12345") to its host fs
	// equivalent inside the active WINEPREFIX. Only creates the leaf;
	// the parent ("C:" by default) is guaranteed to exist already.
	CreateDirectoryA(LogDir.c_str(), nullptr);
}

void Logger::ClearEnabledChannelFiles() {
	EnsureCsInit();
	EnterCriticalSection(&g_log_cs);
	for (const auto& kv : g_states) {
		if (!kv.second.fileEnabled) continue;
		char filename[512];
		snprintf(filename, sizeof(filename), "%s\\%s.txt", LogDir.c_str(), kv.first.c_str());
		// "w" truncates; if the file doesn't exist yet (first run), this just
		// creates an empty one — harmless. Ignore errors silently — there's
		// nothing useful to do here at boot.
		FILE* fp = fopen(filename, "w");
		if (fp) fclose(fp);
	}
	LeaveCriticalSection(&g_log_cs);
}

void Logger::DumpMemory(const char* Channel, void* Address, int Size, int NegativeSize) {
	// Honor either output mode: if the channel is enabled for file OR crash
	// recording, produce output.
	if (!IsChannelEnabled(Channel)) return;

	char* base = (char*)Address;

	Log(Channel, "[%p] Memory Dump:\n", Address);

	if (NegativeSize > 0) {
		for (int i = -NegativeSize * 4; i < 0; i += 4) {
			Log(Channel, "  0x%04X:   %08X\n", i, *(int*)(base + i));
		}
	}

	for (int i = 0; i < Size; i += 4) {
		Log(Channel, "  0x%04X:   %08X\n", i, *(int*)(base + i));
	}
}

static void DumpOneRing(HANDLE f, CrashRing* ring)
{
	DWORD n = 0;

	const uint32_t next = ring->nextIndex.load(std::memory_order_relaxed);

	char header[160];
	int  hlen = snprintf(header, sizeof(header),
	                     "\n--- Crash ring [%s]: %u total writes ---\n",
	                     ring->name, (unsigned)next);
	if (hlen > 0) WriteFile(f, header, (DWORD)hlen, &n, nullptr);

	if (next == 0) return;

	const uint32_t total = (next < kCrashRingEntries) ? next : (uint32_t)kCrashRingEntries;
	const uint32_t start = (next - total) % kCrashRingEntries;

	for (uint32_t i = 0; i < total; ++i) {
		const uint32_t    slot  = (start + i) % kCrashRingEntries;
		const char* const entry = &ring->buffer[slot * kCrashRingEntrySize];
		if (entry[0] == '\0') continue;

		size_t len = 0;
		while (len < kCrashRingEntrySize && entry[len] != '\0') ++len;
		WriteFile(f, entry, (DWORD)len, &n, nullptr);
		if (len == 0 || entry[len - 1] != '\n') {
			WriteFile(f, "\n", 1, &n, nullptr);
		}
	}
}

void Logger::DumpCrashBuffer(void* fileHandle) {
	HANDLE f = (HANDLE)fileHandle;
	DWORD  n = 0;

	const int count = g_crashRingCount.load(std::memory_order_acquire);
	if (count == 0) {
		const char* empty = "\n--- Crash-channel rings: (no channels recorded) ---\n";
		WriteFile(f, empty, (DWORD)strlen(empty), &n, nullptr);
		return;
	}

	for (int ri = 0; ri < count; ++ri) {
		DumpOneRing(f, &g_crashRings[ri]);
	}
}
