#include "src/GameServer/TgGame/TgPawn_Character/SendMarshal/TgPawn_Character__SendMarshal.hpp"
#include "src/GameServer/IpDrv/ClientConnection/SendMarshal/ClientConnection__SendMarshal.hpp"
#include "src/GameServer/IpDrv/NetConnection/FlushNet/NetConnection__FlushNet.hpp"
#include <cstring>

void __fastcall TgPawn_Character__SendMarshal::Call(
	ATgPawn_Character* Pawn, void* /*edx*/, void* Marshal)
{
	if (!Pawn || !Marshal || !Pawn->s_nCharacterId) return;

	APlayerController* PC = (APlayerController*)Pawn->Controller;
	UNetConnection* Connection = (UNetConnection*)PC->Player;
	if (!Connection || (uintptr_t)Connection < 0x10000) return;

	ClientConnection__SendMarshal::CallOriginal(Connection, nullptr, Marshal);
	NetConnection__FlushNet::CallOriginal(Connection);
}
