#pragma once

#include "src/Config/Config.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/pch.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

namespace BlackScreenDiagnostics {

// InstanceSpawner already scopes Logger::LogDir by instance ID. A dedicated
// subfolder plus one fixed-schema file per connection keeps each mission and
// each player's join lifecycle directly comparable without cross-instance mix.

struct EventDetails {
	AActor* relatedActor = nullptr;
	int eventValue = -1;
	int fadeEnabled = -1;
	float fadeAlphaFrom = 0.0f;
	float fadeAlphaTo = 0.0f;
	float fadeTime = 0.0f;
	int cinematic = -1;
	int affectsHud = -1;
};

inline bool IsEnabled() {
	static const bool enabled = []() {
		const std::vector<std::string> channels = Config::GetEnabledChannels();
		return std::find(channels.begin(), channels.end(), "blackscreen") != channels.end();
	}();
	return enabled;
}

inline void Log(const char* phase,
	            const char* event,
	            UNetConnection* connection,
	            ATgPlayerController* controller,
	            const EventDetails& details = {}) {
	if (!IsEnabled()) return;

	if (!connection && controller && controller->Player) {
		connection = (UNetConnection*)controller->Player;
	}

	static unsigned long long sequence = 0;
	static std::mutex fileMutex;
	std::lock_guard<std::mutex> lock(fileMutex);

	const unsigned long long eventSequence = ++sequence;
	const unsigned long long tickMs = (unsigned long long)GetTickCount64();

	const int32_t connectionKey = (int32_t)connection;
	const ClientConnectionData* connectionData = nullptr;
	const PlayerInfo* playerInfo = nullptr;
	auto connectionIt = GClientConnectionsData.find(connectionKey);
	if (connectionIt != GClientConnectionsData.end()) {
		connectionData = &connectionIt->second;
		playerInfo = &connectionData->PlayerInfo;
	}

	const long long characterId = playerInfo ? (long long)playerInfo->selected_character_id : 0;
	const unsigned long long registerGeneration =
		(connectionData && connectionData->pPlayerInfo)
			? (unsigned long long)connectionData->pPlayerInfo->register_generation
			: (playerInfo ? (unsigned long long)playerInfo->register_generation : 0);
	std::string playerName = playerInfo ? playerInfo->player_name : "";
	for (char& ch : playerName) {
		if (ch == '\t' || ch == '\r' || ch == '\n') ch = ' ';
	}
	if (!playerName.empty() &&
	    (playerName[0] == '=' || playerName[0] == '+' ||
	     playerName[0] == '-' || playerName[0] == '@')) {
		playerName.insert(playerName.begin(), '\'');
	}

	std::string stateName;
	if (controller) {
		stateName = controller->GetStateName().GetName();
	}

	APlayerReplicationInfo* pri = controller ? controller->PlayerReplicationInfo : nullptr;

	const std::string directory = Logger::LogDir + "\\blackscreen";
	if (!CreateDirectoryA(directory.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
		return;
	}

	char filename[256];
	if (connection) {
		snprintf(filename, sizeof(filename), "connection-%08X-generation-%llu.tsv",
			(unsigned int)(uintptr_t)connection, registerGeneration);
	} else {
		snprintf(filename, sizeof(filename), "unattributed.tsv");
	}

	char path[768];
	snprintf(path, sizeof(path), "%s\\%s", directory.c_str(), filename);
	FILE* fp = fopen(path, "a+");
	if (!fp) return;

	fseek(fp, 0, SEEK_END);
	if (ftell(fp) == 0) {
		fprintf(fp, "# schema=1\tinstance_id=%lld\tmap=%s\tmap_params=%s\n",
			(long long)Config::GetInstanceId(), Config::GetMapNameChar().c_str(),
			Config::GetMapParamsChar().c_str());
		fprintf(fp,
			"sequence\ttick_ms\tphase\tevent\tcharacter_id\tregister_generation\tplayer_name\t"
			"connection\tcontroller\tstate\tpawn\tacknowledged_pawn\tview_target\trelated_actor\t"
			"controller_ready\tpri_ready\twaiting_player\tonly_spectator\tis_spectator\tout_of_lives\t"
			"event_value\tfade_enabled\tfade_alpha_from\tfade_alpha_to\tfade_time\tcinematic\taffects_hud\n");
	}

	fprintf(fp,
		"%llu\t%llu\t%s\t%s\t%lld\t%llu\t%s\t%p\t%p\t%s\t%p\t%p\t%p\t%p\t"
		"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.3f\t%.3f\t%.3f\t%d\t%d\n",
		eventSequence,
		tickMs,
		phase ? phase : "",
		event ? event : "",
		characterId,
		registerGeneration,
		playerName.c_str(),
		(void*)connection,
		(void*)controller,
		stateName.c_str(),
		controller ? (void*)controller->Pawn : nullptr,
		controller ? (void*)controller->AcknowledgedPawn : nullptr,
		controller ? (void*)controller->ViewTarget : nullptr,
		(void*)details.relatedActor,
		controller ? (int)controller->c_bReadyToPlay : -1,
		pri ? (int)pri->bReadyToPlay : -1,
		pri ? (int)pri->bWaitingPlayer : -1,
		pri ? (int)pri->bOnlySpectator : -1,
		pri ? (int)pri->bIsSpectator : -1,
		pri ? (int)pri->bOutOfLives : -1,
		details.eventValue,
		details.fadeEnabled,
		details.fadeAlphaFrom,
		details.fadeAlphaTo,
		details.fadeTime,
		details.cinematic,
		details.affectsHud);

	fclose(fp);
}

} // namespace BlackScreenDiagnostics
