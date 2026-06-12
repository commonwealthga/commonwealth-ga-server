#include "src/GameServer/TgGame/TgGame/UnregisterForWaveRevive/TgGame__UnregisterForWaveRevive.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Removes the controller from both wave revive lists. Called from
// Dead.Revive, Dead.EndState (player + AI), and TgPlayerController dialogs.
void __fastcall TgGame__UnregisterForWaveRevive::Call(ATgGame* Game, void* edx, AController* Controller) {
	if (Controller == nullptr) {
		return;
	}

	TArray<AController*>* Lists[2] = { &Game->s_AttackerReviveList, &Game->s_DefenderReviveList };
	const char* SideNames[2] = { "attacker", "defender" };
	for (int s = 0; s < 2; s++) {
		TArray<AController*>& List = *Lists[s];
		int w = 0;
		int removed = 0;
		for (int i = 0; i < List.Num(); i++) {
			if (List.Data[i] == Controller) {
				removed++;
				continue;
			}
			List.Data[w++] = List.Data[i];
		}
		List.Count = w;
		if (removed > 0 && Logger::IsChannelEnabled("revive")) {
			const std::string ctrlName(Controller->GetName() ? Controller->GetName() : "<null>");
			Logger::Log("revive", "UnregisterForWaveRevive: removed %s from %s list (count=%d)\n",
				ctrlName.c_str(), SideNames[s], List.Count);
		}
	}
}
