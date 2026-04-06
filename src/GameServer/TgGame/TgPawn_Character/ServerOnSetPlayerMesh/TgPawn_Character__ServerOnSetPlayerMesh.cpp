#include "src/GameServer/TgGame/TgPawn_Character/ServerOnSetPlayerMesh/TgPawn_Character__ServerOnSetPlayerMesh.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__ServerOnSetPlayerMesh::Call(ATgPawn_Character* Pawn, void* edx, int MeshAsmId) {
	LogCallBegin();
	CallOriginal(Pawn, edx, MeshAsmId);
	LogCallEnd();
}
