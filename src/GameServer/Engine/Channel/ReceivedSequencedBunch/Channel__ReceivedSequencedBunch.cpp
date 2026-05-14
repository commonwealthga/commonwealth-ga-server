#include "src/GameServer/Engine/Channel/ReceivedSequencedBunch/Channel__ReceivedSequencedBunch.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"


int __fastcall Channel__ReceivedSequencedBunch::Call(void *Channel, void *edx, void *Bunch) {
	void* Connection = *(void**)((char*)Channel + 0x3c);  // UChannel.Connection
	int   ChIndex    = *(int*)  ((char*)Channel + 0x44);  // UChannel.ChIndex
	bool  bClose     = (*(char*)((char*)Bunch + 0xa9) != '\0');  // FInBunch.bClose

	// Run the engine path first so ConditionalCleanUp on the channel can
	// happen normally — we just observe the close-notify here.
	int result = CallOriginal(Channel, edx, Bunch);

	if (bClose && Connection) {
		auto it = GClientConnectionsData.find((int32_t)Connection);
		if (it != GClientConnectionsData.end()) {
			it->second.bClosed = true;
		}

		// Channel 0 close = the client tearing the session down. The engine's
		// stock path waits for UNetConnection::Tick to notice Channels[0]==NULL
		// and flip State to USOCK_Closed, which can take up to ConnectionTimeout
		// (~30s) and leaves a stale connection sitting around long enough to
		// interfere with reconnect. Flip the state directly so the next
		// UNetDriver::TickDispatch reap runs UNetConnection::CleanUp this frame.
		// Don't call CleanUp inline — ReceivedRawPacket is on the stack above
		// us and tearing the connection down under itself is unsafe.
		if (ChIndex == 0) {
			*(uint32_t*)((char*)Connection + 0x74) = 1; // USOCK_Closed
		}
	}

	return result;
}
