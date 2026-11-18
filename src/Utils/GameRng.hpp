#pragma once

#include <cstdint>
#include <random>
#include <string>

#include "src/Shared/MatchSeed.hpp"

// Gameplay RNG — every roll the server makes that a player can perceive (spawn
// points, spawn timing, bot types, wave pacing/order, objective picks, random
// map-object variants).
//
// Two hard requirements drive the shape of this file:
//
//  1. NEVER use CRT rand(). The DLL links the same msvcrt as the game binary and
//     the binary calls srand() itself during map load, so rand() hands out a
//     reproducible sequence that restarts at the same point on every map load —
//     same spawn nav point, same jitter, same rollout, every single game.
//
//  2. Rolls must be ADDRESSABLE, not just sequential. A single global stream
//     makes an outcome depend on how many draws happened before it, which
//     depends on player behaviour and wall-clock timing — so a recorded match
//     seed could never reproduce a raid whose spawners tick mid-fight. Every
//     decision therefore derives its own engine from
//     (matchSeed, purpose, key, index), where:
//        purpose = what kind of decision (stable enum, part of the seed scheme)
//        key     = which entity owns it (bot factory mapObjectId, wave spawner
//                  name hash, variant group, …)
//        index   = a per-key ordinal that increments in a stable order (spawn
//                  counter, pulse counter, round number, activation counter)
//     A factory's 7th spawn, or a wave node's 3rd pulse of round 2, is then the
//     same roll in every match with the same seed — regardless of when it
//     happened or what else drew in between.
//
// Anything still calling the unkeyed helpers (Index/Int/Float01/FloatSpan) is
// random and match-seeded but NOT replay-stable; they exist for subsystems
// whose draw order is inherently event-driven (SuperAgent ambush pacing, bark
// cues). Convert a call site to Stream when its outcome should reproduce.
namespace GameRng {

// What a roll is for. Values are part of the seed scheme — never renumber or
// reuse one; changing them invalidates previously recorded seeds.
enum class Purpose : uint32_t {
	SpawnPlan       = 1,   // spawn-table group row / roster size, per activation
	SpawnEntryType  = 2,   // per-spawn bot pick inside a group
	SpawnLocation   = 3,   // nav-point pick + XY jitter, per spawn
	WavePace        = 4,   // defense wave spawner interval, per pulse
	WaveOrder       = 5,   // defense wave spawner factory order, per cycle
	Variants        = 6,   // map_object_config weighted variant picks (load time)
	Objective       = 7,   // PointRotation next-objective pick, per round
	AlarmFactory    = 8,   // boss-alarm adds factory pick, per fire
	Legacy          = 100, // unkeyed fallbacks (NOT replay-stable)
};

// ---- match seed -----------------------------------------------------------

namespace detail {

// Single instance per process: function-local statics inside inline functions
// are shared across translation units (an anonymous namespace would not be).
inline uint64_t& SeedSlot() {
	static uint64_t seed = 0;
	return seed;
}
inline bool& SeedFromCmdLineSlot() {
	static bool fromCmdLine = false;
	return fromCmdLine;
}
inline uint64_t RandomSeed() {
	// std::random_device is non-deterministic on Windows (RtlGenRandom).
	uint64_t v = ((uint64_t)std::random_device{}() << 32) ^ (uint64_t)std::random_device{}();
	return v ? v : 1;
}

// Lazily installed on first use so a build that never receives -seed= still
// gets a fresh rollout instead of a constant.
inline uint64_t EnsureSeeded() {
	uint64_t& seed = SeedSlot();
	if (seed == 0) seed = RandomSeed();
	return seed;
}

}  // namespace detail

// Called from the DLL bootstrap with Config::GetMatchSeed() (0 = none given).
// A non-zero seed makes the run replayable under the current seed scheme; a
// zero falls back to a fresh random seed so nothing changes behaviourally.
inline void SetMatchSeed(uint64_t seed) {
	detail::SeedSlot() = (seed != 0) ? seed : detail::RandomSeed();
	detail::SeedFromCmdLineSlot() = (seed != 0);
}

inline uint64_t MatchSeed() { return detail::EnsureSeeded(); }
inline bool MatchSeedFromCommandLine() { return detail::SeedFromCmdLineSlot(); }
inline int SeedScheme() { return kMatchSeedScheme; }

// ---- deterministic derivation --------------------------------------------

// splitmix64 finaliser — the mixing step behind every derivation.
inline uint64_t Mix(uint64_t x) {
	x += 0x9E3779B97F4A7C15ull;
	x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
	x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
	return x ^ (x >> 31);
}

inline uint64_t HashName(const char* name) {
	uint64_t h = 0xCBF29CE484222325ull;             // FNV-1a 64
	if (name) {
		for (const unsigned char* p = (const unsigned char*)name; *p; ++p) {
			h ^= (uint64_t)*p;
			h *= 0x100000001B3ull;
		}
	}
	return h;
}
inline uint64_t HashString(const std::string& s) { return HashName(s.c_str()); }

// The address of one decision → a 64-bit value derived from the match seed.
inline uint64_t Derive(Purpose purpose, uint64_t key, uint64_t index) {
	uint64_t h = MatchSeed();
	h ^= Mix((uint64_t)purpose * 0xD1B54A32D192ED03ull);
	h ^= Mix(key            * 0x9E3779B97F4A7C15ull);
	h ^= Mix(index          * 0xC2B2AE3D27D4EB4Full);
	return Mix(h);
}

// Engine for one addressable decision. Seed it, then take as many draws as the
// decision needs; the sequence is stable for the lifetime of this scheme.
class Stream {
public:
	Stream(Purpose purpose, uint64_t key, uint64_t index = 0)
		: gen_(static_cast<std::mt19937::result_type>(Derive(purpose, key, index))) {}

	int Index(int n) {
		if (n <= 0) return 0;
		return std::uniform_int_distribution<int>(0, n - 1)(gen_);
	}
	int Int(int lo, int hi) {   // inclusive
		if (hi < lo) { const int t = lo; lo = hi; hi = t; }
		return std::uniform_int_distribution<int>(lo, hi)(gen_);
	}
	float Float01() {
		return std::uniform_real_distribution<float>(0.0f, 1.0f)(gen_);
	}
	float FloatSpan(float span) {
		if (span <= 0.0f) return 0.0f;
		return std::uniform_real_distribution<float>(0.0f, span)(gen_);
	}

private:
	std::mt19937 gen_;
};

// ---- unkeyed fallbacks (random + match-seeded, NOT replay-stable) ---------
//
// One stateful engine seeded from the match seed. Fine for event-driven
// subsystems (ambush pacing, bark cues); anything that must reproduce should
// use Stream(purpose, key, index) instead.

inline std::mt19937& LegacyEngine() {
	static std::mt19937 gen(static_cast<std::mt19937::result_type>(Derive(Purpose::Legacy, 0, 0)));
	return gen;
}
inline int Index(int n) {
	if (n <= 0) return 0;
	return std::uniform_int_distribution<int>(0, n - 1)(LegacyEngine());
}
inline int Int(int lo, int hi) {   // inclusive
	if (hi < lo) { const int t = lo; lo = hi; hi = t; }
	return std::uniform_int_distribution<int>(lo, hi)(LegacyEngine());
}
inline float Float01() {
	return std::uniform_real_distribution<float>(0.0f, 1.0f)(LegacyEngine());
}
inline float FloatSpan(float span) {
	if (span <= 0.0f) return 0.0f;
	return std::uniform_real_distribution<float>(0.0f, span)(LegacyEngine());
}

}  // namespace GameRng
