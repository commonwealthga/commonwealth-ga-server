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

// Keyed by session_guid.
extern std::map<std::string, TcpEvent> GTcpEvents;

