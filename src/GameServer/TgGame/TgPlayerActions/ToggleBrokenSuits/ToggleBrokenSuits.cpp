#include "src/GameServer/TgGame/TgPlayerActions/ToggleBrokenSuits/ToggleBrokenSuits.hpp"

#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Cosmetics/BrokenSuitSwap.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/UserPreferences/UserPreferences.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstring>
#include <string>

namespace TgPlayerActions::ToggleBrokenSuitsCmd {

namespace {

ClientConnectionData* FindBySessionGuid(const std::string& guid) {
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.SessionGuid == guid) {
			return &kv.second;
		}
	}
	return nullptr;
}

void AlertPlayer(ATgPawn_Character* pawn, const char* msg) {
	if (!pawn || !pawn->Controller) return;
	const char* raw = pawn->Controller->Class ? pawn->Controller->Class->GetFullName() : nullptr;
	const std::string ctrlClass(raw ? raw : "");
	if (ctrlClass.find("PlayerController") == std::string::npos) return;

	APlayerController* PC = (APlayerController*)pawn->Controller;
	if (!PC->Player) return;
	UNetConnection* Connection = (UNetConnection*)PC->Player;

	// ASCII zero-extend widen (message text is ASCII-only).
	const std::wstring wmsg(msg, msg + std::strlen(msg));
	SendAlert::SendText(Connection, wmsg.c_str(), 1, 0, 3.0f);
}

// The engine only re-sends r_CustomCharacterAssembly as a delta; after a pref
// flip the viewer's channels still hold the previously-presented values, so
// force every player pawn + PRI into the next net update to re-diff now.
void ForceAssemblyResend() {
	for (auto& kv : GClientConnectionsData) {
		ATgPawn_Character* pawn = kv.second.Pawn;
		if (!pawn) continue;
		pawn->bNetDirty       = 1;
		pawn->bForceNetUpdate = 1;
		auto* PRI = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
		if (PRI) {
			PRI->bNetDirty       = 1;
			PRI->bForceNetUpdate = 1;
		}
	}
}

} // namespace

void Execute(const std::string& session_guid, int mode) {
	ClientConnectionData* data = FindBySessionGuid(session_guid);
	if (!data) {
		Logger::Log("brokensuits",
			"[ChatCmd][DLL] togglebrokensuits guid=%s: no connection; dropping\n",
			session_guid.c_str());
		return;
	}
	const int64_t userId = data->PlayerInfo.user_id;
	if (userId <= 0) {
		Logger::Log("brokensuits",
			"[ChatCmd][DLL] togglebrokensuits guid=%s: no user_id; dropping\n",
			session_guid.c_str());
		return;
	}

	bool show;
	if (mode == 0 || mode == 1) {
		show = (mode == 1);
		UserPreferences::Set(userId, BrokenSuitSwap::kPrefKey, show ? "1" : "0");
	} else {
		show = UserPreferences::ToggleBool(userId, BrokenSuitSwap::kPrefKey,
		                                   BrokenSuitSwap::kPrefDefault);
	}
	ForceAssemblyResend();

	Logger::Log("brokensuits",
		"[ChatCmd][DLL] togglebrokensuits guid=%s user=%lld mode=%d -> show=%d\n",
		session_guid.c_str(), (long long)userId, mode, show ? 1 : 0);

	AlertPlayer(data->Pawn, show
		? "Broken suits ENABLED - you see original cosmetics"
		: "Broken suits DISABLED - stutter-prone cosmetics are replaced");
}

} // namespace TgPlayerActions::ToggleBrokenSuitsCmd
