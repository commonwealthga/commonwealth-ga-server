#include "src/GameServer/GameModes/Acquisition/AssertTrace.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <string>

namespace {
std::string ScanStack(uintptr_t* sp) {
	HMODULE self = nullptr;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&AssertTrace::Call, &self);
	const uintptr_t dllLo = (uintptr_t)self, dllHi = dllLo + 0x1000000;
	std::string out; int n = 0;
	for (int i = 0; i < 2048 && n < 60; i++) {
		const uintptr_t v = sp[i]; char buf[32];
		if (v >= 0x10901000 && v < 0x11681000) { snprintf(buf, sizeof buf, " %08x", (unsigned)v); out += buf; n++; }
		else if (v >= dllLo && v < dllHi) { snprintf(buf, sizeof buf, " dll+%x", (unsigned)(v - dllLo)); out += buf; n++; }
	}
	return out;
}
int s_avLogged = 0;
LONG CALLBACK FirstChance(EXCEPTION_POINTERS* info) {
	if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || s_avLogged >= 3) return EXCEPTION_CONTINUE_SEARCH;
	if (!Logger::IsChannelEnabled("assert")) return EXCEPTION_CONTINUE_SEARCH;  // off unless the channel is on
	s_avLogged++;
	const char* rep = "-";
	void* a = AssertTrace::s_lastRepActor;
	AssertTrace::s_lastRepActor = nullptr;  // never re-enter on a bad pointer
	char buf[64]; snprintf(buf, sizeof buf, "%p", a); rep = buf;
	Logger::Log("assert", "[AVTrace] eip=%08lx addr=%08lx lastRepActor=%s stack:%s\n",
		(unsigned long)info->ContextRecord->Eip, (unsigned long)info->ExceptionRecord->ExceptionInformation[1], rep,
		ScanStack((uintptr_t*)info->ContextRecord->Esp).c_str());
	if (a) Logger::Log("assert", "[AVTrace] lastRepActor name=%s\n", ((UObject*)a)->GetFullName());
	return EXCEPTION_CONTINUE_SEARCH;
}
}  // namespace

void AssertTrace::InstallFirstChance() { AddVectoredExceptionHandler(1, FirstChance); }

void __cdecl AssertTrace::Call(void* a, void* b, int line, void* d) {
	if (!Logger::IsChannelEnabled("assert")) { m_original(a, b, line, d); return; }
	HMODULE self = nullptr;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&AssertTrace::Call, &self);
	const uintptr_t dllLo = (uintptr_t)self, dllHi = dllLo + 0x1000000;
	uintptr_t* sp = (uintptr_t*)&a;
	std::string out;
	int n = 0;
	for (int i = 0; i < 2048 && n < 60; i++) {
		const uintptr_t v = sp[i];
		char buf[32];
		if (v >= 0x10901000 && v < 0x11681000) { snprintf(buf, sizeof buf, " %08x", (unsigned)v); out += buf; n++; }
		else if (v >= dllLo && v < dllHi) { snprintf(buf, sizeof buf, " dll+%x", (unsigned)(v - dllLo)); out += buf; n++; }
	}
	const char* rep = "-";
	if (s_lastRepActor) rep = ((UObject*)s_lastRepActor)->GetFullName();
	Logger::Log("assert", "[AssertTrace] line=%d lastRepActor=%s stack:%s\n", line, rep, out.c_str());
	m_original(a, b, line, d);
}
