#pragma once

#include "src/Utils/HookBase.hpp"

class World__NotifyAcceptedConnection : public HookBase<
	void(*)(),
	0x10BEBFD0,
	World__NotifyAcceptedConnection> {
public:
	static void Call();
	static inline void CallOriginal(void* Notify, UNetConnection* Connection) {
		asm volatile(
			"push %[conn] \n\t"
			"mov  %[notify], %%ecx \n\t"   // FIXED: move notify into ecx
			"call *%[func] \n\t"
			:
			: [func] "r" (World__NotifyAcceptedConnection::m_original),
			[notify] "r" (Notify),
			[conn] "r" (Connection)
			: "ecx", "memory"
		);
	}
};

