#include "src/GameServer/TgGame/TgPawn_Character/ServerOnSetPlayerMesh/TgPawn_Character__ServerOnSetPlayerMesh.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/Tutorial/TutorialSuitUp.hpp"

void __fastcall TgPawn_Character__ServerOnSetPlayerMesh::Call(ATgPawn_Character* Pawn, void* edx, int MeshAsmId) {
	LogCallBegin();
	CallOriginal(Pawn, edx, MeshAsmId);
	// Skylark suit-up - Kismet "Default Suit" hands over the class armour + kit.
	TutorialSuitUp::OnSetPlayerMesh((ATgPawn*)Pawn, MeshAsmId);
	LogCallEnd();
}
