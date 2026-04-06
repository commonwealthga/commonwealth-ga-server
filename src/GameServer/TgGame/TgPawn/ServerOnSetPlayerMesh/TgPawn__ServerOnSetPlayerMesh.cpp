#include "src/GameServer/TgGame/TgPawn/ServerOnSetPlayerMesh/TgPawn__ServerOnSetPlayerMesh.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__ServerOnSetPlayerMesh::Call(ATgPawn* Pawn, void* edx, int MeshAsmId) {
	LogCallBegin();
	CallOriginal(Pawn, edx, MeshAsmId);
	LogCallEnd();
}
