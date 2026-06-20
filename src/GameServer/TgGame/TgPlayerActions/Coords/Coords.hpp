#pragma once

#include <string>

namespace TgPlayerActions::CoordsCmd {

// -coords: report the requesting player's current pawn position (XYZ). Looks up
// the pawn via GClientConnectionsData[session_guid], logs the position on the
// "coords" channel (persistent record) and shows it as a center-screen alert to
// that player (read live while walking the map to mark objective spots).
//
// Map-prep tooling for the CTR Control-Point mode — see
// .planning/2026-06-20-ctr-control-point-coords-design.md.
//
// MUST be called on the game thread (sole caller path is IpcClient::DrainInbound
// from Actor::Tick).
void Execute(const std::string& session_guid);

} // namespace TgPlayerActions::CoordsCmd
