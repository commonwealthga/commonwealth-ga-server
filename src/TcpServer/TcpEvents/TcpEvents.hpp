#pragma once

#include "src/pch.hpp"

struct TcpEvent {
	int Type;
	ATgPawn* Pawn;
	std::vector<std::string> Names;
};

extern std::vector<TcpEvent> GTcpEvents;

