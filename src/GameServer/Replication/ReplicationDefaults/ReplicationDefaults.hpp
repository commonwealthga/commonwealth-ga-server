#pragma once

// One-shot CDO patcher for replication-related actor fields.
//
// In UE3, AActor::Spawn copies the CDO's property memory into the new instance
// during InitProperties. A static patch of CDO fields therefore applies to
// every subsequent spawn at zero per-spawn cost — no Spawn hook needed, no
// GObjects walk, no per-instance fixup sprinkled across the codebase.
//
// Call ::Apply() once at engine init, AFTER LoadStartupPackages (CDOs must
// exist) and BEFORE the original GameEngine init (no world has spawned yet).
// The right hook point is GameEngine__Init::Call, between the two.
class ReplicationDefaults {
public:
	static void Apply();
};
