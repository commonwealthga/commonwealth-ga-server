#include "src/pch.hpp"
#include "src/Utils/CrashHandler/CrashHandler.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <winternl.h>

// Where to drop crash dumps. Set by CrashHandler::Init() from config.
// Default is a Wine-mapped path (Z: maps / on the host) that matches the
// historical hardcoded value, used if the caller passes an empty string.
static std::string g_crashDir = "Z:\\home\\zax\\games\\crashes";

// Re-entry guard: if our filter itself faults, don't recurse.
static volatile LONG s_inFilter = 0;

static void WriteStr(HANDLE f, const char* s)
{
    DWORD n = 0;
    WriteFile(f, s, (DWORD)strlen(s), &n, nullptr);
}

static void ResolveModule(void* addr, char* out, size_t outSz)
{
    HMODULE mod = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)addr, &mod) && mod) {
        char name[MAX_PATH] = {0};
        if (GetModuleFileNameA(mod, name, sizeof(name))) {
            const char* base = strrchr(name, '\\');
            base = base ? base + 1 : name;
            unsigned long off = (unsigned long)((uintptr_t)addr - (uintptr_t)mod);
            snprintf(out, outSz, "%s+0x%lx", base, off);
            return;
        }
    }
    snprintf(out, outSz, "?");
}

static bool SafeRead(const void* p, size_t n)
{
    // IsBadReadPtr is deprecated but adequate here: we already hold the filter
    // re-entry guard, so a recursive fault would be dropped.
    return !IsBadReadPtr(p, n);
}

static void WriteModuleList(HANDLE f)
{
    WriteStr(f, "\n--- Loaded modules ---\n");
    PEB* peb = nullptr;
#if defined(__i386__)
    {
        unsigned long pebAddr;
        __asm__ volatile ("movl %%fs:0x30, %0" : "=r"(pebAddr));
        peb = (PEB*)(uintptr_t)pebAddr;
    }
#endif
    if (!peb || !peb->Ldr) return;

    PEB_LDR_DATA* ldr = peb->Ldr;
    LIST_ENTRY* head = &ldr->InMemoryOrderModuleList;
    LIST_ENTRY* cur  = head->Flink;

    char line[MAX_PATH + 64];
    int count = 0;
    while (cur && cur != head && count < 512) {
        // InMemoryOrderLinks is the 2nd LIST_ENTRY in LDR_DATA_TABLE_ENTRY;
        // step back one LIST_ENTRY (8 bytes on i386) to reach the struct base.
        LDR_DATA_TABLE_ENTRY* e =
            (LDR_DATA_TABLE_ENTRY*)((char*)cur - sizeof(LIST_ENTRY));

        if (e->FullDllName.Buffer && e->FullDllName.Length) {
            char name[MAX_PATH] = {0};
            WideCharToMultiByte(CP_ACP, 0,
                                e->FullDllName.Buffer,
                                e->FullDllName.Length / 2,
                                name, sizeof(name) - 1, nullptr, nullptr);
            snprintf(line, sizeof(line), "  %08lx  %s\n",
                     (unsigned long)(uintptr_t)e->DllBase, name);
            WriteStr(f, line);
        }
        cur = cur->Flink;
        ++count;
    }
}

static LONG WINAPI CrashFilter(EXCEPTION_POINTERS* info)
{
    // First-chance filter: only dump for exception codes that indicate a real
    // crash. Skip C++ throws (0xE06D7363), DBG_* breakpoints, etc., since
    // otherwise the VEH fires for every caught exception.
    const DWORD code = info->ExceptionRecord->ExceptionCode;
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_STACK_OVERFLOW:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_PRIV_INSTRUCTION:
        case EXCEPTION_IN_PAGE_ERROR:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            break;
        default:
            return EXCEPTION_CONTINUE_SEARCH;
    }

    if (InterlockedExchange(&s_inFilter, 1)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    SYSTEMTIME st;
    GetLocalTime(&st);

    CreateDirectoryA(g_crashDir.c_str(), nullptr); // no-op if exists

    char path[MAX_PATH];
    snprintf(path, sizeof(path),
             "%s\\crash-%04d%02d%02d-%02d%02d%02d-pid%lu.log",
             g_crashDir.c_str(), st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond,
             (unsigned long)GetCurrentProcessId());

    HANDLE f = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ,
                           nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) {
        s_inFilter = 0;
        return EXCEPTION_CONTINUE_SEARCH;
    }

    char line[1024];

    EXCEPTION_RECORD* er = info->ExceptionRecord;
    CONTEXT*          cx = info->ContextRecord;

    snprintf(line, sizeof(line),
             "=== Unhandled exception ===\n"
             "Time:    %04d-%02d-%02d %02d:%02d:%02d\n"
             "PID:     %lu   TID: %lu\n"
             "Code:    0x%08lx   Flags: 0x%08lx\n"
             "Address: %p\n",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond,
             (unsigned long)GetCurrentProcessId(),
             (unsigned long)GetCurrentThreadId(),
             (unsigned long)er->ExceptionCode,
             (unsigned long)er->ExceptionFlags,
             er->ExceptionAddress);
    WriteStr(f, line);

    if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2) {
        const char* verb =
            er->ExceptionInformation[0] == 0 ? "read" :
            er->ExceptionInformation[0] == 1 ? "write" :
            er->ExceptionInformation[0] == 8 ? "execute" : "?";
        snprintf(line, sizeof(line),
                 "Access:  %s at 0x%08lx\n",
                 verb, (unsigned long)er->ExceptionInformation[1]);
        WriteStr(f, line);
    }

#if defined(__i386__)
    snprintf(line, sizeof(line),
             "\n--- Registers ---\n"
             "EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\n"
             "ESI=%08lx EDI=%08lx EBP=%08lx ESP=%08lx\n"
             "EIP=%08lx EFLAGS=%08lx\n"
             "CS=%04lx DS=%04lx ES=%04lx FS=%04lx GS=%04lx SS=%04lx\n",
             (unsigned long)cx->Eax, (unsigned long)cx->Ebx,
             (unsigned long)cx->Ecx, (unsigned long)cx->Edx,
             (unsigned long)cx->Esi, (unsigned long)cx->Edi,
             (unsigned long)cx->Ebp, (unsigned long)cx->Esp,
             (unsigned long)cx->Eip, (unsigned long)cx->EFlags,
             (unsigned long)cx->SegCs, (unsigned long)cx->SegDs,
             (unsigned long)cx->SegEs, (unsigned long)cx->SegFs,
             (unsigned long)cx->SegGs, (unsigned long)cx->SegSs);
    WriteStr(f, line);

    WriteStr(f, "\n--- Backtrace (EBP chain) ---\n");
    {
        char modbuf[MAX_PATH];
        ResolveModule((void*)cx->Eip, modbuf, sizeof(modbuf));
        snprintf(line, sizeof(line), "#00 0x%08lx %s\n",
                 (unsigned long)cx->Eip, modbuf);
        WriteStr(f, line);

        DWORD prevEbp = 0;
        DWORD* ebp = (DWORD*)cx->Ebp;
        for (int i = 1; i < 128; ++i) {
            if (!SafeRead(ebp, 8)) break;
            DWORD nextEbp = ebp[0];
            DWORD retAddr = ebp[1];
            if (!retAddr) break;
            ResolveModule((void*)retAddr, modbuf, sizeof(modbuf));
            snprintf(line, sizeof(line), "#%02d 0x%08lx %s\n",
                     i, (unsigned long)retAddr, modbuf);
            WriteStr(f, line);
            if (nextEbp <= (DWORD)(uintptr_t)ebp) break; // must grow upward
            if (nextEbp == prevEbp) break;
            prevEbp = (DWORD)(uintptr_t)ebp;
            ebp = (DWORD*)(uintptr_t)nextEbp;
        }
    }

    WriteStr(f, "\n--- Stack dump (256 dwords @ ESP) ---\n");
    {
        DWORD* sp = (DWORD*)cx->Esp;
        for (int i = 0; i < 256; i += 4) {
            if (!SafeRead(sp + i, 16)) break;
            snprintf(line, sizeof(line),
                     "  %08lx: %08lx %08lx %08lx %08lx\n",
                     (unsigned long)(cx->Esp + (DWORD)(i * 4)),
                     (unsigned long)sp[i + 0],
                     (unsigned long)sp[i + 1],
                     (unsigned long)sp[i + 2],
                     (unsigned long)sp[i + 3]);
            WriteStr(f, line);
        }
    }
#endif

    WriteModuleList(f);

    // Append the crash-channel ring buffer (e.g. hook_calltree recordings) so
    // we see what the server was doing just before the fault.
    Logger::DumpCrashBuffer(f);

    WriteStr(f, "\n=== End of dump ===\n");

    FlushFileBuffers(f);
    CloseHandle(f);

    // CONTINUE_SEARCH so Wine's default handler (and AeDebug, if configured)
    // still runs after us. If it swallows the process silently, our file is
    // already on disk.
    s_inFilter = 0;
    return EXCEPTION_CONTINUE_SEARCH;
}

void CrashHandler::Init(const std::string& crashDir)
{
    if (!crashDir.empty()) {
        g_crashDir = crashDir;
    }

    // Vectored handlers fire before any __try/__except and CANNOT be
    // overridden by later SetUnhandledExceptionFilter() calls. UE3 installs
    // its own top-level filter during WinMain, so SetUnhandledExceptionFilter
    // alone would lose us. First=1 puts us at the head of the VEH chain.
    AddVectoredExceptionHandler(1, CrashFilter);

    // Belt and suspenders: also register as top-level in case something
    // suppresses the VEH. Harmless if the game's own filter replaces it.
    SetUnhandledExceptionFilter(CrashFilter);
}
