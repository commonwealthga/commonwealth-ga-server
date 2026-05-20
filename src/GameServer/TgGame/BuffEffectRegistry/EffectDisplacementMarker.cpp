#include "src/GameServer/TgGame/BuffEffectRegistry/EffectDisplacementMarker.hpp"

#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <vector>

namespace EffectDisplacementMarker {

namespace {
using clock_t = std::chrono::steady_clock;

// Mark expiry window. The legitimate apply-driven displacement path is a
// fully-synchronous C++ chain through UC's GetNewEffectGroupByApp /
// GetStackingEffectGroup / GetRefreshedEffectGroup → returns to UC's
// ProcessEffect → adds clone to s_AppliedEffectGroups → SetEffectRep. End-to-
// end this is microseconds. Any stale mark from a STANDALONE remove (UC
// ProcessEffect(bRemove=true) for scope-out, jetpack-stop, fire-release; UC
// LifeOver timer expiry) sits in the registry until the next apply with the
// same egId — at minimum one frame later (~16ms at 60fps), in practice
// hundreds of ms (player has to release RMB and press it again).
//
// 2ms is comfortably above the legitimate path (µs) and well below one frame
// (~16ms). Cross-frame Consume cannot succeed.
//
// Symptom this expiry fixes: Colony Energy Rifle scope-in sound + visual
// doubling on the SECOND and subsequent scope-ins (first scope after spawn
// was clean because no prior mark existed). Scope-out's UC ProcessEffect
// (bRemove=true) → C++ RemoveEffectGroup left a mark that the next scope-in's
// SetEffectRep consumed, falsely treating a first-apply as a displacement
// and triggering the manual r_EventQueue push on top of Branch B's fresh
// slot — two transient TgEffectForms for the same fx_id → doubled audio.
constexpr auto kMaxAge = std::chrono::milliseconds(2);

struct Entry {
	int egId;
	clock_t::time_point time;
};

// Per-manager vector of timestamped egId marks. Single-threaded game-thread
// access — no locking. Vector (not set) because we need timestamps; entries
// per manager per frame are O(1) in practice (one displacement per apply).
std::unordered_map<ATgEffectManager*, std::vector<Entry>> g_marks;

void PruneExpired(std::vector<Entry>& vec, clock_t::time_point now) {
	vec.erase(std::remove_if(vec.begin(), vec.end(),
		[now](const Entry& e) { return now - e.time > kMaxAge; }),
		vec.end());
}
}

void Mark(ATgEffectManager* manager, int egId) {
	if (!manager) return;
	auto now = clock_t::now();
	auto& vec = g_marks[manager];
	PruneExpired(vec, now);
	vec.push_back({egId, now});
}

bool ConsumeWasJustDisplaced(ATgEffectManager* manager, int egId) {
	if (!manager) return false;
	auto it = g_marks.find(manager);
	if (it == g_marks.end()) return false;
	auto& vec = it->second;
	auto now = clock_t::now();
	PruneExpired(vec, now);

	// Consume the first matching (non-expired) entry for this egId.
	bool found = false;
	for (auto eit = vec.begin(); eit != vec.end(); ++eit) {
		if (eit->egId == egId) {
			vec.erase(eit);
			found = true;
			break;
		}
	}
	if (vec.empty()) g_marks.erase(it);
	return found;
}

void Forget(ATgEffectManager* manager) {
	if (!manager) return;
	g_marks.erase(manager);
}

}  // namespace EffectDisplacementMarker
