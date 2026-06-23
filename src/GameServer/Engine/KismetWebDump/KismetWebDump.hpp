#pragma once

// Dumps every kismet sequence in the loaded map (nodes, links, variable
// values, editor positions/comments, runtime trigger state) to
// <LogDir>/kismet_<MapName>.json for the read-only visual viewer at
// tools/kismet-viewer/index.html.
//
// Driven by the `-dumpkismet=1` switch. Note: World::BeginPlay runs BEFORE the
// async streaming of sublevels (e.g. Raid_DomeCityDefense_Sound holds the
// announcer-VO kismet, _TER holds terrain) completes, so a BeginPlay-time dump
// captures only the persistent level. Instead BeginPlay ARMS a delayed dump and
// GameEngine::Tick fires it once after the streaming settle window — capturing
// every loaded sublevel sequence.
class KismetWebDump {
public:
	static void DumpAll();

	// Arm the delayed dump (call from World::BeginPlay when -dumpkismet is set).
	// Re-arms per map load so multi-map sessions each dump once.
	static void ArmDelayedDump();

	// Tick the arm timer (call every GameEngine::Tick with the frame delta).
	// Fires DumpAll() once when the settle window elapses, then disarms.
	static void TickDelayedDump(float deltaSeconds);
};
