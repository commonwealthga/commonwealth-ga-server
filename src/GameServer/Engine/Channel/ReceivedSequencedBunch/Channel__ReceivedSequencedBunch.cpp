#include "src/GameServer/Engine/Channel/ReceivedSequencedBunch/Channel__ReceivedSequencedBunch.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"


int __fastcall Channel__ReceivedSequencedBunch::Call(void *Channel, void *edx, void *Bunch) {
	void* Connection = *(void**)((char*)Channel + 0x3c);

	if (*(char*)((char*)Bunch + 0xa9) != '\0') {
		// close-notify

		int32_t index = (int32_t)Connection;
		auto it = GClientConnectionsData.find(index);
		if (it == GClientConnectionsData.end()) {
			// connection not found
			return CallOriginal(Channel, edx, Bunch);
		}

		it->second.bClosed = true;
	}

	return CallOriginal(Channel, edx, Bunch);
}

