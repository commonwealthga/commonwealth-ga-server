#include "src/GameServer/TgGame/TgPawn/TickMakeVisibleCalculation/TgPawn__TickMakeVisibleCalculation.hpp"

// Stub placeholder for a server-side native that the shipping binary stripped
// (the original body is `RET 0x4`). Hook is installed to reserve the slot —
// the original's correct body is still TBD. Earlier attempt (forcing
// `m_fMakeVisibleCurrent` to 1.0 per tick while stealthed) was misguided:
// that field is a temporary reveal pulse that `TgPawn::Tick` (FUN_109cb910)
// intentionally decays toward 0 each frame, so fighting the decay just creates
// rapid oscillation. The visual stealth trigger lives elsewhere — likely a
// mesh/material swap driven by `r_bIsStealthed` replication, via a native
// OnRep path we haven't located yet.
void __fastcall TgPawn__TickMakeVisibleCalculation::Call(ATgPawn* /*Pawn*/, void* /*edx*/, float /*DeltaTime*/) {
}
