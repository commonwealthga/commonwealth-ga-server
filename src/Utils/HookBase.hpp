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

	static inline void LogCall() {
		Logger::Log(GetLogChannel(), "├─ %s::Call();\n", GetFunctionName());
	}

	static inline void LogCallBegin() {
		Logger::Log(GetLogChannel(), "├─ %s::Call\n", GetFunctionName());
		Logger::ChannelIndents[GetLogChannel()]++;
	}

	static inline void LogCallEnd() {
		Logger::ChannelIndents[GetLogChannel()]--;
		// Logger::Log(GetLogChannel(), "}\n");
	}

	static inline void LogCallBegin(const char* FunctionName) {
		Logger::Log(GetLogChannel(), "├─ %s::%s\n", GetFunctionName(), FunctionName);
		Logger::ChannelIndents[GetLogChannel()]++;
	}

	static inline void LogCallOriginal() {
		Logger::Log(GetLogChannel(), "├─ %s::CallOriginal();\n", GetFunctionName());
	}

	static inline void LogCallOriginalBegin() {
		Logger::Log(GetLogChannel(), "├─ %s::CallOriginal\n", GetFunctionName());
		Logger::ChannelIndents[GetLogChannel()]++;
	}

	static inline void LogCallOriginalEnd() {
		Logger::ChannelIndents[GetLogChannel()]--;
		// Logger::Log(GetLogChannel(), "}\n");
	}

    static Func m_original;
};

template<typename F, uintptr_t A, typename D>
typename HookBase<F,A,D>::Func HookBase<F,A,D>::m_original = reinterpret_cast<F>(A);

