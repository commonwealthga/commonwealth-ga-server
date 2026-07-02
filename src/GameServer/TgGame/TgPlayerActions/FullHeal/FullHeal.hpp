#pragma once

#include <string>

namespace TgPlayerActions::FullHealCmd {

// -fullheal: restore the requesting player's pawn to full health. Only active
// on the VR arena maps (Dome3_VR_Arena_P*) — practice / 1v1 use. Per-player
// cooldown to prevent spam mid-duel; remaining time is shown as an alert.
//
// MUST be called on the game thread (sole caller path is IpcClient::DrainInbound
// from Actor::Tick).
void Execute(const std::string& session_guid);

} // namespace TgPlayerActions::FullHealCmd
