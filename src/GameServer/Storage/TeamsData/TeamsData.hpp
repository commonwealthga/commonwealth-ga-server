#pragma once

#include "src/pch.hpp"

struct TeamsData {
	ATgRepInfo_TaskForce* Attackers;
	ATgRepInfo_TaskForce* Defenders;
	
	ATgDeploy_Beacon* BeaconAttackers = nullptr;
	ATgDeploy_Beacon* BeaconDefenders = nullptr;

	ATgDeploy_BeaconEntrance* BeaconEntranceAttackers = nullptr;
	ATgDeploy_BeaconEntrance* BeaconEntranceDefenders = nullptr;
};

extern TeamsData GTeamsData;

