#include "src/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.hpp"

void __fastcall ClientConnection__SendMarshal::Call(UNetConnection* Connection, void* edx, void* Marshal) {
	void* MarshalVtable = *(void**)Marshal;

	if (!Marshal || !Connection || !MarshalVtable) {
		return;
	}

	CallOriginal(Connection, edx, Marshal);
}

