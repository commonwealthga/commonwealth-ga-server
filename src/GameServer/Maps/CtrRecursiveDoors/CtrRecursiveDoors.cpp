#include "src/GameServer/Maps/CtrRecursiveDoors/CtrRecursiveDoors.hpp"

#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/pch.hpp"

#include <cmath>
#include <vector>

namespace {
constexpr const char* CH = "recursive";

// The two leaves of the attacker spawn door (far cluster, x=770). Confirmed in
// playtest: clearing (770,7036) opened that half server-side and let players
// through, while (770,7040) stayed shut. The level's InterpActors all share one
// object name, so we match them by exact level coordinate.
struct Leaf { float x, y, z; };
constexpr Leaf kLeaves[] = {
	{ 770.0f, 7036.0f, -3076.0f },
	{ 770.0f, 7040.0f, -3076.0f },
};
constexpr int   kLeafCount = (int)(sizeof(kLeaves) / sizeof(kLeaves[0]));
constexpr float MATCH_TOL  = 10.0f;

// Resolved once by CacheSpawnDoors(); rooted by ObjectCache so the pointers stay
// valid (the door InterpActors are map-baked and live the whole match).
std::vector<AActor*> g_cachedLeaves;

inline float DistTo(const FVector& a, const Leaf& l) {
	const float dx = a.X - l.x, dy = a.Y - l.y, dz = a.Z - l.z;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}
}  // namespace

void CtrRecursiveDoors::CacheSpawnDoors() {
	g_cachedLeaves.clear();

	// Sanctioned cached lookup (scans once, caches by class) — no raw GObjObjects
	// iteration, and nothing here repeats during the match.
	const std::vector<UObject*>& interps = ObjectCache::FindAllByClass("Class Engine.InterpActor");

	for (const Leaf& leaf : kLeaves) {
		AActor* best = nullptr;
		float bestD = MATCH_TOL;
		for (UObject* o : interps) {
			if (!o) continue;
			const float d = DistTo(((AActor*)o)->Location, leaf);
			if (d < bestD) { bestD = d; best = (AActor*)o; }
		}
		if (best) {
			g_cachedLeaves.push_back(best);
			Logger::Log(CH, "CacheSpawnDoors: cached leaf (%.0f,%.0f,%.0f) d=%.1f\n",
				best->Location.X, best->Location.Y, best->Location.Z, bestD);
		} else {
			Logger::Log(CH, "CacheSpawnDoors: no InterpActor at (%.0f,%.0f,%.0f) among %d\n",
				leaf.x, leaf.y, leaf.z, (int)interps.size());
		}
	}

	Logger::Log(CH, "CacheSpawnDoors: cached %d/%d leaves from %d InterpActors\n",
		(int)g_cachedLeaves.size(), kLeafCount, (int)interps.size());
}

void CtrRecursiveDoors::OpenAttackerSpawnDoor() {
	if (g_cachedLeaves.empty()) {
		Logger::Log(CH, "OpenAttackerSpawnDoor: no cached leaves (CacheSpawnDoors not run?)\n");
		return;
	}

	int opened = 0;
	for (AActor* a : g_cachedLeaves) {
		if (!a) continue;

		// Make the (replicated) hide + collision change reach clients — these
		// door InterpActors normally aren't network-relevant.
		a->bStatic = 0;
		a->RemoteRole = 1;  // ROLE_SimulatedProxy
		a->bAlwaysRelevant = 1;

		a->SetHidden(1);           // bHidden        — CPF_Net
		a->SetCollision(0, 0, 0);  // bCollide/Block — CPF_Net

		a->bNetDirty = 1;
		a->bForceNetUpdate = 1;
		opened++;

		Logger::Log(CH, "OpenAttackerSpawnDoor: cleared leaf (%.0f,%.0f,%.0f)\n",
			a->Location.X, a->Location.Y, a->Location.Z);
	}

	Logger::Log(CH, "OpenAttackerSpawnDoor: cleared %d leaves\n", opened);
}
