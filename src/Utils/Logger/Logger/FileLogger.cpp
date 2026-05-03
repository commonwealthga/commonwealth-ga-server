#include "src/Utils/Logger/Logger.hpp"

#include <atomic>

std::map<std::string, int> Logger::ChannelIndents;
std::vector<std::string> Logger::EnabledChannels;
std::vector<std::string> Logger::EnabledCrashChannels;
std::string Logger::LogDir = "C:";
static CRITICAL_SECTION g_log_cs;
static bool g_log_cs_init = false;

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

static bool IsCrashChannel(const char* channel)
{
	// Snapshot size first in case another thread is pushing into the vector.
	const size_t n = Logger::EnabledCrashChannels.size();
	for (size_t i = 0; i < n; ++i) {
		if (Logger::EnabledCrashChannels[i] == channel) return true;
	}
	return false;
}

static void CrashRingAppend(const char* channel, const char* format, va_list args)
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

	// Mirror the file-logger's "│  " indent per ChannelIndents level so the
	// ring buffer preserves the call-tree visualization. Uses find() (not
	// operator[]) to avoid insert-on-miss; concurrent map writes are already
	// accepted risk in the existing logger.
	int indent = 0;
	{
		auto it = Logger::ChannelIndents.find(channel);
		if (it != Logger::ChannelIndents.end()) indent = it->second;
	}
	for (int i = 0; i < indent; ++i) {
		const int w = snprintf(slot + pos, kCrashRingEntrySize - (size_t)pos, "│  ");
		if (w < 0 || (size_t)(pos + w) >= kCrashRingEntrySize - 1) break;
		pos += w;
	}

	vsnprintf(slot + pos, kCrashRingEntrySize - (size_t)pos, format, args);
	slot[kCrashRingEntrySize - 1] = '\0';
}

void Logger::Log(const char* Channel, const char* Format, ...) {
	// Fast path for crash-only channels: no mutex, no file I/O.
	if (IsCrashChannel(Channel)) {
		va_list args;
		va_start(args, Format);
		CrashRingAppend(Channel, Format, args);
		va_end(args);
	}

	if (std::find(EnabledChannels.begin(), EnabledChannels.end(), Channel) == EnabledChannels.end()) {
		return;
	}

	if (!g_log_cs_init) {
		InitializeCriticalSection(&g_log_cs);
		g_log_cs_init = true;
	}

	EnterCriticalSection(&g_log_cs);

	char filename[512];
	snprintf(filename, sizeof(filename), "%s\\%s.txt", LogDir.c_str(), Channel);

    FILE* fp = fopen(filename, "a");
    if (fp == NULL) {
		LeaveCriticalSection(&g_log_cs);
        return;
    }

	int indent = ChannelIndents[Channel];
	for (int i = 0; i < indent; i++) {
		fprintf(fp, "│  ");
	}

    va_list args;
    va_start(args, Format);

    vfprintf(fp, Format, args);      // Write formatted content

    va_end(args);
    fclose(fp);

	LeaveCriticalSection(&g_log_cs);
}

void Logger::DumpMemory(const char* Channel, void* Address, int Size, int NegativeSize) {
	// Honor either output mode: if the channel is enabled for file OR crash
	// recording, produce output. (Previously only file mode was checked.)
	const bool toFile  = std::find(EnabledChannels.begin(), EnabledChannels.end(), Channel) != EnabledChannels.end();
	const bool toRing  = IsCrashChannel(Channel);
	if (!toFile && !toRing) {
		return;
	}

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
