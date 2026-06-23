#pragma once

// Server-side mission-VO director for Raid_DomeCityDefense_P. The shipped server
// fired Ava's per-round satellite-countdown banter via console events
// (ce Bancroft_HalfwayPoint / _30sRemaining / _10sRemaining); that native
// director is gone, and the events have no kismet feeder, so on our server they
// never fired. This reproduces it off the per-round timer (state 6) via
// MissionVO::CauseEvent.
//
// Round-4 divergence ("I can't fire the satellite" + dismantler approach) is
// handled by the map's OWN kismet counter (SeqCond_Increment) on the
// Bancroft_10sRemaining event, so we just fire each threshold once per round and
// let the kismet route the 4th 10sRemaining to the satellite-failed branch.
// Ticked from GameEngine::Tick. Channel "missionvo".
class MissionVODirector {
public:
	static void Tick(float dt);
};
