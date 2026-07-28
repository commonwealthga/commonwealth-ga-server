#pragma once

#include <string>

namespace TgPlayerActions::ToggleBrokenSuitsCmd {

// -togglebrokensuits [1|0]: per-user "show broken suits" preference
// (ga_user_preferences, key BrokenSuitSwap::kPrefKey). mode: 1 = show
// originals, 0 = show replacements, -1 = toggle current value. Persists via
// UserPreferences and takes effect on the next replication pass (all player
// pawns/PRIs are force-marked net-dirty here so it lands immediately).
// `all` = -toggleallsuits instead: same mechanics on kPrefKeyAll ("show ALL
// suits"; 0 = OTHER players render suitless/bare-headed for this viewer —
// the viewer's own character is exempt).
//
// MUST be called on the game thread (sole caller path is IpcClient::DrainInbound
// from Actor::Tick).
void Execute(const std::string& session_guid, int mode, bool all);

} // namespace TgPlayerActions::ToggleBrokenSuitsCmd
