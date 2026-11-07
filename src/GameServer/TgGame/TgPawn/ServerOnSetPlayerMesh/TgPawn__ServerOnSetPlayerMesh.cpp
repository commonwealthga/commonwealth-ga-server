#include "src/GameServer/TgGame/TgPawn/ServerOnSetPlayerMesh/TgPawn__ServerOnSetPlayerMesh.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/Tutorial/TutorialSuitUp.hpp"

void __fastcall TgPawn__ServerOnSetPlayerMesh::Call(ATgPawn* Pawn, void* edx, int MeshAsmId) {
	LogCallBegin();
	CallOriginal(Pawn, edx, MeshAsmId);
	// Skylark suit-up - Kismet "Default Suit" hands over the class armour + kit.
	TutorialSuitUp::OnSetPlayerMesh((ATgPawn*)Pawn, MeshAsmId);
	LogCallEnd();
}
