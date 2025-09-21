#pragma once

#include "src/pch.hpp"

template<typename FuncSig, uintptr_t Address, typename Derived>
class HookBase {
public:
    using Func = FuncSig;

    static void Install() {
        if (m_original) {
            ::DetourAttach(&(PVOID&)m_original, (PVOID)&Derived::Call);
        }
    }

    static Func m_original;
};

template<typename F, uintptr_t A, typename D>
typename HookBase<F,A,D>::Func HookBase<F,A,D>::m_original = reinterpret_cast<F>(A);

