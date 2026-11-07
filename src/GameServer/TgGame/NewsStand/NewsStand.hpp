#pragma once

#include "src/pch.hpp"

// Global News stand in Dome City. The original stand
// (TgNewsStand, centre of the dome) played a positional news broadcast through
// Hi-Rez's Vivox voice service (gad.vivox.com, gone); nothing else in the
// client asks the server for news. Instead, when a player walks up to a stand
// the instance sends a "news_stand" GAME_EVENT and the control server shows
// the current news feed (news.json) in that player's chat. Edge-triggered,
// at most once per player per kRepeatSec. Disable with -nonewsstand=1.
class NewsStand {
public:
	static void Tick(float DeltaSeconds);
};
