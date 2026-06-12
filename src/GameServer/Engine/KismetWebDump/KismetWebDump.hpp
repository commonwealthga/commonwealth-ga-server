#pragma once

// Dumps every kismet sequence in the loaded map (nodes, links, variable
// values, editor positions/comments, runtime trigger state) to
// <LogDir>/kismet_<MapName>.json for the read-only visual viewer at
// tools/kismet-viewer/index.html.
//
// Driven by the `-dumpkismet=1` switch from World::BeginPlay, same pattern
// as MapDataDumper.
class KismetWebDump {
public:
	static void DumpAll();
};
