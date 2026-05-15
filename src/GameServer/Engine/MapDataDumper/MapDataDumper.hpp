#pragma once

// Walks UObject::GObjObjects() and writes a human-readable dump of selected
// map-placed actors (player starts, bot starts, factories, mission
// objectives, beacon factories) to the "mapdump" log channel.
//
// Designed to be driven by the `-dumpmapdata=1` switch from World::BeginPlay
// after the persistent level has been loaded and BeginPlay'd, so every
// edit-time field on the listed classes is populated.
//
// To add a new class or extend an existing one, add a Dump* helper at the
// top of MapDataDumper.cpp and register it in the kDumpers table at the
// bottom of the same file. Field printing is plain Logger::Log lines so
// adding a property is one more line in the helper.
class MapDataDumper {
public:
	static void DumpAll();
};
