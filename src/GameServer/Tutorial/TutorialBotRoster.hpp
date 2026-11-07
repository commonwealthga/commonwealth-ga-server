#pragma once

#include "src/pch.hpp"

#include <string>

// Bot roster diagnostic + dev bot kill, for checking a
// mission's enemies without playing it through.
// Tick: only while the "tutbots" log channel is on (off on live). Once a
// second it walks the AI controllers and logs each new bot (pawn id, bot id,
// behavior id, factory, alarm mark, hp, spot), each death (seconds alive,
// distance moved) and every 10 s a roster line per bot (state, hp, distance
// moved since the last roster, has enemy/target) plus each player's hp.
namespace TutorialBotRoster {

void Tick(float DeltaSeconds);

// Dev shortcut (dev-kill target "bots"): kill living AI pawns through the
// normal death path with the first player as killer. botId / factoryId 0 =
// any. Returns how many were killed.
int KillBots(int botId, int factoryId);

// Dev shortcut (dev-kill target "touch"): activate the nth Kismet touch event
// whose output chain reaches TgSeqAct_<op>, with the first player as instigator.
bool FireTouch(const std::string& op, int nth);

}  // namespace TutorialBotRoster
