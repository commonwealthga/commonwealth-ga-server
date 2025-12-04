#include "src/GameServer/IpDrv/NetConnection/CleanupActor/NetConnection__CleanupActor.hpp"

void __fastcall NetConnection__CleanupActor::Call(UNetConnection* Connection) {
	LogCallBegin();

	CallOriginal(Connection);

	// if (Connection->Actor != nullptr) {
	// 	if (Connection->Actor->Player != nullptr) {
	// 		Connection->Actor->Player = nullptr;
	// 	}
	// 	Connection->Actor->bDeleteMe = 1;
	// 	Connection->Actor = nullptr;
	// }

	LogCallEnd();
}

