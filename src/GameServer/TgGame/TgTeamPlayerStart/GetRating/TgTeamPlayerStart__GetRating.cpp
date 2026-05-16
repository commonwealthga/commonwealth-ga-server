#include "src/GameServer/TgGame/TgTeamPlayerStart/GetRating/TgTeamPlayerStart__GetRating.hpp"

float __fastcall TgTeamPlayerStart__GetRating::Call(ATgTeamPlayerStart* PlayerStart, void* edx, AController* Player) {
	if (PlayerStart == nullptr) return 0.0f;
	// Mirrors TgStartPoint.GetRating UC body:
	//   if (!bEnabled) return 0;
	//   Rating = m_fCurrentRating + (bPrimaryStart ? 100 : 0);
	if (!PlayerStart->bEnabled) return 0.0f;
	float rating = PlayerStart->m_fCurrentRating;
	if (PlayerStart->bPrimaryStart) rating += 100.0f;
	return rating;
}
