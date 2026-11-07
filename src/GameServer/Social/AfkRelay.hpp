#pragma once
// The client's /afk (TgPlayerController.GoAFK, local exec) and the
// "back from AFK" path both end in the server RPC TgPlayerController.ServerToggleAFK(bool bEnabled), which
// only sets TgPawn.r_bIsAFK on the game server. AFK state for /who and the "%s - AFK" whisper auto-reply
// lives on the control server, so forward every toggle there as GAME_EVENT afk_state (header-only;
// included by UObject__ProcessEvent.cpp, called before CallOriginal).
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"

namespace AfkRelay {
	inline void Report(UObject* Controller, void* Params) {
		if (!Controller || !Params) return;
		if (!Logger::IsChannelEnabled("social")) Logger::EnableChannel("social");   // rare RPC; file social.txt
		const bool enabled = (*(uint32_t*)Params & 1u) != 0;   // UBOOL bEnabled
		APawn* pawn = ((AController*)Controller)->Pawn;
		for (auto& kv : GClientConnectionsData) {
			if (kv.second.bClosed || kv.second.SessionGuid.empty()) continue;
			if (pawn == nullptr || (APawn*)kv.second.Pawn != pawn) continue;
			const auto* info = kv.second.pPlayerInfo ? kv.second.pPlayerInfo : &kv.second.PlayerInfo;
			nlohmann::json ev;
			ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
			ev["subtype"]      = "afk_state";
			ev["instance_id"]  = IpcClient::GetInstanceId();
			ev["session_guid"] = kv.second.SessionGuid;
			ev["character_id"] = info->selected_character_id;
			ev["afk"]          = enabled;
			IpcClient::Send(ev.dump());
			Logger::Log("social", "[%s] [AfkRelay] ServerToggleAFK(%d) guid=%s -> CS\n",
				Logger::GetTime(), (int)enabled, kv.second.SessionGuid.c_str());
			return;
		}
		Logger::Log("social", "[%s] [AfkRelay] ServerToggleAFK(%d) - no player session for pawn %p\n",
			Logger::GetTime(), (int)enabled, (void*)pawn);
	}
}
