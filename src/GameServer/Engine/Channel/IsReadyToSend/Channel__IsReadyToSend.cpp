#include "src/GameServer/Engine/Channel/IsReadyToSend/Channel__IsReadyToSend.hpp"

// UMarshalChannel vtable. See decompiled/TgGame/_combat_message_pipeline.md
// § "UMarshalChannel vtable (0x116e1560)".
static constexpr uintptr_t UMARSHALCHANNEL_VTABLE = 0x116e1560;

// chan+0x58 = NumOutRec. See decompiled/Engine/UChannel/UChannel__IsReadyToSend/.
static inline int GetNumOutRec(void* chan) {
	return *(int*)((char*)chan + 0x58);
}

int __fastcall Channel__IsReadyToSend::Call(void* Channel) {
	if (!Channel) return 0;

	if (*(uintptr_t*)Channel == UMARSHALCHANNEL_VTABLE) {
		return GetNumOutRec(Channel) <= 126 ? 1 : 0;
	}

	return CallOriginal(Channel);
}
