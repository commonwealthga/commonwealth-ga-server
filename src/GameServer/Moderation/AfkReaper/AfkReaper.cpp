#include "src/GameServer/Moderation/AfkReaper/AfkReaper.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"

#include <utility>
#include <vector>

std::map<std::string, AfkReaper::ActivityState> AfkReaper::states_;
float AfkReaper::sweep_accum_       = 0.0f;
int   AfkReaper::threshold_seconds_ = -1;

void AfkReaper::Tick(float DeltaSeconds) {
	if (threshold_seconds_ < 0) {
		threshold_seconds_ = Config::GetAfkKickSeconds();
		Logger::Log("afk", "[AfkReaper] threshold=%ds (0 = disabled)\n", threshold_seconds_);
	}
	if (threshold_seconds_ == 0) return;

	sweep_accum_ += DeltaSeconds;
	if (sweep_accum_ < 15.0f) return;
	const float elapsed = sweep_accum_;
	sweep_accum_ = 0.0f;

	std::vector<std::pair<UNetConnection*, std::string>> to_kick;
	for (auto& kv : GClientConnectionsData) {
		const std::string& guid = kv.second.SessionGuid;
		ATgPawn* pawn = (ATgPawn*)kv.second.Pawn;
		if (guid.empty() || !pawn) continue;  // pre-spawn connection

		ActivityState& st = states_[guid];
		// Exact compare is intentional: a standing pawn's Location/Rotation
		// are bit-identical between frames; any input moves the camera yaw.
		// Crafting counts as activity — a crafting player is stationary by
		// design (same exemption as TgPlayerController.CanPlayerAFK).
		const bool active =
			pawn->r_bIsCrafting               ||
			st.pawn_id        != pawn->r_nPawnId ||
			st.location.X     != pawn->Location.X ||
			st.location.Y     != pawn->Location.Y ||
			st.location.Z     != pawn->Location.Z ||
			st.rotation.Pitch != pawn->Rotation.Pitch ||
			st.rotation.Yaw   != pawn->Rotation.Yaw ||
			st.rotation.Roll  != pawn->Rotation.Roll;
		if (active) {
			st.pawn_id      = pawn->r_nPawnId;
			st.location     = pawn->Location;
			st.rotation     = pawn->Rotation;
			st.idle_seconds = 0.0f;
			continue;
		}

		st.idle_seconds += elapsed;
		if (st.idle_seconds >= (float)threshold_seconds_) {
			to_kick.emplace_back((UNetConnection*)kv.first, guid);
		}
	}

	// Drop tracking for sessions whose connection is gone.
	for (auto it = states_.begin(); it != states_.end();) {
		bool present = false;
		for (const auto& kv : GClientConnectionsData) {
			if (kv.second.SessionGuid == it->first) { present = true; break; }
		}
		it = present ? std::next(it) : states_.erase(it);
	}

	for (const auto& kick : to_kick) {
		Logger::Log("afk", "[AfkReaper] reaping guid=%s: no input for %ds\n",
			kick.second.c_str(), threshold_seconds_);

		// Tell the control server so it can drop the TCP session too; then
		// flip the connection to USOCK_Closed — the engine's TickDispatch
		// reap runs CleanUp this frame, which sends PLAYER_LEFT and destroys
		// the PlayerController/pawn via the engine's own teardown. Do NOT
		// drive NetConnection__Cleanup directly: the connection is live.
		nlohmann::json ev;
		ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]      = "afk_kick";
		ev["instance_id"]  = IpcClient::GetInstanceId();
		ev["session_guid"] = kick.second;
		IpcClient::Send(ev.dump());

		MarkConnectionClosed(kick.first);
		states_.erase(kick.second);
	}
}
