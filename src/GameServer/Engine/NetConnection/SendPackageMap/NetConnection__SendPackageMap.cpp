#include "src/GameServer/Engine/NetConnection/SendPackageMap/NetConnection__SendPackageMap.hpp"
#include "src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall NetConnection__SendPackageMap::Call(void* Connection) {
	// Fix forced-export GUIDs before the package map is sent to the client.
	void* PackageMap = *(void**)((char*)Connection + 0xBC);

	char* List = *(char**)((char*)PackageMap + 0x3C);
	int ListCount = *(int*)((char*)PackageMap + 0x40);
	Logger::Log("packagemap", "[SendPackageMap HOOK] Connection=%p PackageMap=%p List=%p Count=%d\n",
		Connection, PackageMap, List, ListCount);

	if (Config::GetFixPackageGuids()) {
		MarshalChannel__NotifyControlMessage::FixForcedExportGuids(PackageMap);
	} else {
		Logger::Log("packagemap", "[SendPackageMap] GUID fix disabled by config\n");
	}

	// Now send the (patched) package map
	CallOriginal(Connection);
}
