// Logger channel: "vendor" (same as the consumable spend line)
#include "src/GameServer/Combat/FireRules/FireRules.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {
constexpr int kConsumePoint        = 13;    // engine equip point of slot value 501 "ES13 (Consumable)"
constexpr int kPersonalVandalBoost = 5813;  // Robotics morale boost (mode 4537)

void Tell(ATgPawn* Pawn, const wchar_t* text) {
	for (auto& kv : GClientConnectionsData) {
		if (kv.second.bClosed || kv.second.Pawn != Pawn) continue;
		SendAlert::SendTextAlert((UNetConnection*)(intptr_t)kv.first, text, /*priority*/ 2, /*type*/ 1, 3.0f);
		break;
	}
}
}  // namespace

bool FireRules::RefuseStartFire(ATgDevice* Device) {
	if (!Device || !Device->Instigator) return false;
	ATgPawn* Pawn = (ATgPawn*)Device->Instigator;
	if (!Pawn->WorldInfo || !Pawn->WorldInfo->GRI) return false;
	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Pawn->WorldInfo->GRI;
	const char* why = nullptr;
	const wchar_t* text = nullptr;
	if (GRI->r_bIsPVP && (int)Device->r_eEquippedAt == kConsumePoint) {
		why = "consumable in PvP"; text = L"Consumables cannot be used in PvP.";
	} else if (GRI->r_bIsOpenWorld && Device->r_nDeviceId == kPersonalVandalBoost) {
		why = "Personal Vandal Boost in an open zone"; text = L"Personal Vandal Boost is disabled in open zones.";
	}
	if (!why) return false;
	Logger::Log("vendor", "[FireRules] refused pawn=%d device=%d point=%d: %s\n",
		(int)Pawn->r_nPawnId, Device->r_nDeviceId, (int)Device->r_eEquippedAt, why);
	Tell(Pawn, text);
	return true;
}
