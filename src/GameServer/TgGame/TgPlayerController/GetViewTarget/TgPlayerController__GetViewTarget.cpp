#include "src/GameServer/TgGame/TgPlayerController/GetViewTarget/TgPlayerController__GetViewTarget.hpp"
#include "src/Utils/Logger/Logger.hpp"

int __fastcall TgPlayerController__GetViewTarget::Call(void* Controller) {

	int result = CallOriginal(Controller);

	if (!result) {
		return result;
	}

	UObject* obj = (UObject*)result;

	// workaround - until first respawn the view target is controller, which prevents actor replication
	if (strcmp(obj->Class->GetFullName(), "Class TgGame.TgPlayerController") == 0) {
		ATgPlayerController* PlayerController = (ATgPlayerController*)obj;

		if (PlayerController->Pawn && PlayerController->Pawn->Health > 0 && PlayerController->Pawn->Controller == Controller) {
			return (int)PlayerController->Pawn;
		}
	}

	return result;
}

