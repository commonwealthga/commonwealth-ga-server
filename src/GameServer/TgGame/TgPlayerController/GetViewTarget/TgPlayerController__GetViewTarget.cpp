#include "src/GameServer/TgGame/TgPlayerController/GetViewTarget/TgPlayerController__GetViewTarget.hpp"
#include "src/Utils/Logger/Logger.hpp"

int __fastcall TgPlayerController__GetViewTarget::Call(void* Controller) {

	int result = CallOriginal(Controller);

	if (!result) {
		return result;
	}

	UObject* obj = (UObject*)result;

	// Defensive: if the binary's GetViewTarget fallback ever lands on PC=self
	// while a healthy possessed pawn exists, prefer the pawn. The actual
	// VT-flip-to-self bug is fixed at its source via the
	// `Function TgGame.TgGame.PostLogin` ProcessEvent hook (clears
	// bOnlySpectator for non-spectator joins before RestartPlayer runs); this
	// remains as belt-and-suspenders.
	if (strcmp(obj->Class->GetFullName(), "Class TgGame.TgPlayerController") == 0) {
		ATgPlayerController* PlayerController = (ATgPlayerController*)obj;

		if (PlayerController->Pawn && PlayerController->Pawn->Health > 0 && PlayerController->Pawn->Controller == Controller) {
			return (int)PlayerController->Pawn;
		}
	}

	return result;
}

