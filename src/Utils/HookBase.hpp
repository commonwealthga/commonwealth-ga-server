#pragma once

#include "src/pch.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstddef>
// Stack bytes a function's parameters occupy (each rounded to 4). The check
// in Install() compares the hook's declared original signature with Call():
// a mismatch makes CallOriginal push a different amount than the original
// pops, which only some compilers happen to survive.
template<typename... A> struct HookArgSum {
	static constexpr std::size_t value = (std::size_t{0} + ... + ((sizeof(A) + 3) & ~std::size_t{3}));
};
template<typename T> struct HookArgBytes;
template<typename R, typename... A> struct HookArgBytes<R(__cdecl*)(A...)>    : HookArgSum<A...> {};
template<typename R, typename... A> struct HookArgBytes<R(__stdcall*)(A...)>  : HookArgSum<A...> {};
template<typename R, typename... A> struct HookArgBytes<R(__fastcall*)(A...)> : HookArgSum<A...> {};
template<typename R, typename... A> struct HookArgBytes<R(__thiscall*)(A...)> : HookArgSum<A...> {};

template<typename FuncSig, uintptr_t Address, typename Derived>
class HookBase {
public:
    using Func = FuncSig;

    static void Install() {
        static_assert(HookArgBytes<FuncSig>::value == HookArgBytes<decltype(&Derived::Call)>::value,
                      "hook signature mismatch: FuncSig and Derived::Call take different argument sizes");
        if (m_original) {
            ::DetourAttach(&(PVOID&)m_original, (PVOID)&Derived::Call);
        }
    }

	static const char* GetFunctionName() {
		return typeid(Derived).name();
	}

	static const char* GetLogChannel() {
		return "hook_calltree";
	}

	// All LogCall* helpers gate on `IsChannelEnabled` first so disabled
	// channels (the production case) pay a single pointer-cache lookup
	// and bail. When enabled, the Log + IndentChannel calls each do their
	// own (cheap) cache lookup. Keeping the gate here means hot-path
	// hooks don't have to remember to add it themselves.

	static inline void LogCall() {
		if (!Logger::IsChannelEnabled(GetLogChannel())) return;
		Logger::Log(GetLogChannel(), "├─ %s::Call();\n", GetFunctionName());
	}

	static inline void LogCallBegin() {
		if (!Logger::IsChannelEnabled(GetLogChannel())) return;
		Logger::Log(GetLogChannel(), "├─ %s::Call\n", GetFunctionName());
		Logger::IndentChannel(GetLogChannel(), +1);
	}

	static inline void LogCallEnd() {
		if (!Logger::IsChannelEnabled(GetLogChannel())) return;
		Logger::IndentChannel(GetLogChannel(), -1);
	}

	static inline void LogCallBegin(const char* FunctionName) {
		if (!Logger::IsChannelEnabled(GetLogChannel())) return;
		Logger::Log(GetLogChannel(), "├─ %s::%s\n", GetFunctionName(), FunctionName);
		Logger::IndentChannel(GetLogChannel(), +1);
	}

	static inline void LogCallOriginal() {
		if (!Logger::IsChannelEnabled(GetLogChannel())) return;
		Logger::Log(GetLogChannel(), "├─ %s::CallOriginal();\n", GetFunctionName());
	}

	static inline void LogCallOriginalBegin() {
		if (!Logger::IsChannelEnabled(GetLogChannel())) return;
		Logger::Log(GetLogChannel(), "├─ %s::CallOriginal\n", GetFunctionName());
		Logger::IndentChannel(GetLogChannel(), +1);
	}

	static inline void LogCallOriginalEnd() {
		if (!Logger::IsChannelEnabled(GetLogChannel())) return;
		Logger::IndentChannel(GetLogChannel(), -1);
	}

    static Func m_original;
};

template<typename F, uintptr_t A, typename D>
typename HookBase<F,A,D>::Func HookBase<F,A,D>::m_original = reinterpret_cast<F>(A);
