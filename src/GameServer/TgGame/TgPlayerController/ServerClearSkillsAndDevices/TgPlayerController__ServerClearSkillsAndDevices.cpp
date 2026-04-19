#include "src/GameServer/TgGame/TgPlayerController/ServerClearSkillsAndDevices/TgPlayerController__ServerClearSkillsAndDevices.hpp"
#include "src/Utils/Logger/Logger.hpp"

// UTgPlayerController::ServerClearSkillsAndDevices — the `ResetMeSkills` cheat
// / admin console command routes here (TgPlayerController.uc:885). Original
// body is stripped; we only handle the skills half of the name since device
// reset already has its own flow (ServerClearProfiles → loadout reset) and
// nothing in UC expects this native to wipe inventory too.
//
// Scoped forwarding: walk to the controller's pawn and dispatch the skill
// respec there. The native lives on TgPlayerController so the server can fire
// it without a pawn reference (e.g. from chat); when a pawn exists we still
// want the effect-list clear and control-server wipe.
void __fastcall TgPlayerController__ServerClearSkillsAndDevices::Call(ATgPlayerController* Controller, void* edx) {
	LogCallBegin();

	if (Controller && Controller->Pawn) {
		ATgPawn_Character* pc = (ATgPawn_Character*)Controller->Pawn;
		pc->ServerRemoveAllCharSkills();
	} else {
		Logger::Log("skills", "[ServerClearSkillsAndDevices] controller without pawn — ignored\n");
	}

	LogCallEnd();
}
