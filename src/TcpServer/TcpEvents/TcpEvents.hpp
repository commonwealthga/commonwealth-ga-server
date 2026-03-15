#pragma once

#include "src/pch.hpp"
#include <string>

struct TcpEvent {
	int Type;
	ATgPawn* Pawn;
	std::string session_guid;
	std::string player_name;
	std::vector<std::string> Names;
	std::map<std::string, int> IntValues;
};

struct BeaconPickupEvent {
	int nPawnId;
	int nDeviceId;
	int nInventoryId;
	int nEquipSlotValueId;
};

struct BeaconRemoveEvent {
	int nPawnId;
	int nInventoryId;
};

// Keyed by session_guid.
extern std::map<std::string, TcpEvent> GTcpEvents;

// Pending mid-game inventory-add packets, keyed by session_guid.
extern std::map<std::string, std::vector<BeaconPickupEvent>> GBeaconPickupEvents;

// Pending mid-game inventory-remove packets, keyed by session_guid.
extern std::map<std::string, std::vector<BeaconRemoveEvent>> GBeaconRemoveEvents;

enum class QuestAction { Request, Abandon };

struct QuestEvent {
	int nQuestId;
	QuestAction action;
};

// Pending quest update responses, keyed by session_guid.
extern std::map<std::string, std::vector<QuestEvent>> GQuestEvents;

// Reverse map: pawn pointer → session_guid (populated on spawn).
extern std::map<ATgPawn*, std::string> GPawnSessions;

