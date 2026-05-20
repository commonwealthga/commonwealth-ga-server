#pragma once

#include "src/pch.hpp"
#include "src/Utils/Logger/Logger.hpp"

template<typename FuncSig, uintptr_t Address, typename Derived>
class HookBase {
public:
    using Func = FuncSig;

    static void Install() {
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
