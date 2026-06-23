#include "src/GameServer/Engine/World/BeginPlay/World__BeginPlay.hpp"
#include "src/GameServer/Engine/KismetDump/KismetDump.hpp"
#include "src/GameServer/Engine/KismetWebDump/KismetWebDump.hpp"
#include "src/GameServer/Engine/MapDataDumper/MapDataDumper.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Config/Config.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"

void __fastcall World__BeginPlay::Call(void *World, void *edx, void *Url, int bResetTime) {
	LogCallBegin();

	Globals::Get().GWorld = World;

	// Reload the override registry before any actor BeginPlay / LoadObjectConfig
	// hooks fire — variant groups re-roll each call, giving per-map-load variety.
	MapObjectConfig::Init();

	World__BeginPlay::CallOriginal(World, edx, Url, bResetTime);

	DumpKismetSequence();

	if (Config::GetDumpMapData()) {
		MapDataDumper::DumpAll();
	}

	if (Config::GetDumpKismet()) {
		// Arm a delayed dump instead of dumping now: BeginPlay runs before the
		// async streaming of sublevels (e.g. *_Sound holds the announcer-VO
		// kismet) finishes, so an immediate dump misses them. GameEngine::Tick
		// fires the actual dump once the settle window elapses.
		KismetWebDump::ArmDelayedDump();
	}

	// Signal control server that this instance is ready for players.
	{
		int64_t inst_id = Config::GetInstanceId();
		nlohmann::json ready;
		ready["type"]        = IpcProtocol::MSG_INSTANCE_READY;
		ready["instance_id"] = inst_id;
		ready["max_players"] = 100;  // Hardcoded for Phase 9; read from GameInfo in future
		IpcClient::Send(ready.dump());
		Logger::Log("ipc", "[World__BeginPlay] Sent INSTANCE_READY: instance_id=%lld\n", inst_id);
	}

	LogCallEnd();
}

