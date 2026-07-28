#pragma once

// Count of currently-connected spectators on THIS instance (one game process
// = one instance, so this is inherently per-instance already). Incremented
// once per spectator connection in the TgGamePostLogin ProcessEvent case
// (UObject__ProcessEvent.cpp), decremented on disconnect
// (NetConnection__Cleanup.cpp). Never spawns a pawn either way -- this is
// purely a gate for SpectatorOverlayFeed: when it's 0, nobody is watching
// this instance, so there's no reason to gather/send per-pawn health+effect
// snapshots for it at all. Also means the always-on home map -- which a
// spectator can never be routed to (enforced control-server-side in
// DeliverSpectateJoin) -- never leaves 0, so it never pushes either, with no
// separate is_home_map check needed here.
extern int GActiveSpectatorCount;
