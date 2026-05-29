#include "src/GameServer/TgGame/TgGame/GetDifficultyModifier/TgGame__GetDifficultyModifier.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Native is stripped — CallOriginal returns garbage (per the stub-hook rule),
// so any binary code path that ends up here was getting a broken modifier.
// Return the per-difficulty scalar instead. nPriority is logged but ignored
// until we observe an actual caller pattern that demands per-priority scaling
// (props 346/347 "Priority/Objective Id (PvE)" are defined but unused in DB,
// so the original per-priority contract is unknown).
float __fastcall TgGame__GetDifficultyModifier::Call(ATgGame* Game, void* edx, int nPriority) {
	LogCallBegin();
	const float result = Config::GetDifficultyScalar();
	Logger::Log("difficulty",
		"GetDifficultyModifier(nPriority=%d) -> %.2f (diffId=%d)\n",
		nPriority, result, Config::GetDifficultyValueId());
	LogCallEnd();
	return result;
}
