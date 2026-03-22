#pragma once
#include "src/pch.hpp"
#include <map>
#include <string>

// Reverse map: ATgPawn* -> session_guid. Populated in MarshalChannel__NotifyControlMessage
// on UDP JOIN; read by NonPersistAddDevice and NonPersistRemoveDevice for IPC event routing.
extern std::map<ATgPawn*, std::string> GPawnSessions;
