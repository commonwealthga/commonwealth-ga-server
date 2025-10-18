#pragma once

#include "src/pch.hpp"

struct TcpEvent {
	int Type;
	ATgPawn* Pawn;
};

extern std::vector<TcpEvent> GTcpEvents;

