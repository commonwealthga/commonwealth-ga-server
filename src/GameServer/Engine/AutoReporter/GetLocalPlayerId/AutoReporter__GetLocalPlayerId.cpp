#include "src/GameServer/Engine/AutoReporter/GetLocalPlayerId/AutoReporter__GetLocalPlayerId.hpp"

int __cdecl AutoReporter__GetLocalPlayerId::Call() {
	// Dedicated server: no LocalPlayer. Original native faults on the lookup
	// chain; short-circuit to 0 here.
	return 0;
}
