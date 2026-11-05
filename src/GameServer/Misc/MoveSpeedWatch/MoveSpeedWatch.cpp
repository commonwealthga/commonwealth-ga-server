#include "src/GameServer/Misc/MoveSpeedWatch/MoveSpeedWatch.hpp"

#include <cmath>
#include <cstring>
#include <unordered_map>

#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MoveSpeedWatch {

namespace {

static constexpr const char* CH = "move_speed";

// Ratio vs the pawn's current cap (walk GroundSpeed / jet AirSpeed).
// 1.09 = 9% over that cap. Hysteresis clears the streak only once the
// sample drops back under 5% so jitter around the trip point doesn't
// reset a real run.
static constexpr float kOverRatio = 1.09f;
static constexpr float kClearRatio = 1.05f;
// ~6 packets at 30–60 Hz ≈ 0.1–0.2 s of sustained overspeed. Knockback is
// usually 1–3 packets; a speed hack holds the streak.
static constexpr int kOverStreak = 6;
static constexpr float kMinDt = 0.020f;
static constexpr float kMaxDt = 0.45f;
// Skip beacon / spawn / teleport jumps (uu). Legal jet cruise is ~595 uu/s.
static constexpr float kMaxStepUu = 800.0f;
static constexpr float kSummaryPeriodSec = 30.0f;
static constexpr float kOverLogCooldownSec = 2.0f;

struct State {
	APawn* pawn = nullptr;
	float lastTs = 0.0f;
	float lastX = 0.0f;
	float lastY = 0.0f;
	float lastZ = 0.0f;
	bool haveLast = false;

	int samples = 0;
	int overSamples = 0;
	int triggers = 0;       // times a streak reached kOverStreak
	int streak = 0;
	bool streakCounted = false;
	float maxRatio = 0.0f;
	float sumDt = 0.0f;     // sampled move time since last SUMMARY
	float lastOverLogTs = -1000.0f;
};

static std::unordered_map<APlayerController*, State> g_state;

static void NarrowName(const wchar_t* w, char* out, int cap) {
	if (!out || cap <= 0) return;
	if (!w) { out[0] = '\0'; return; }
	int i = 0;
	for (; w[i] && i < cap - 1; ++i) {
		out[i] = (w[i] > 0 && w[i] < 128) ? (char)w[i] : '?';
	}
	out[i] = '\0';
}

static void FillIdentity(APlayerController* pc, APawn* pawn,
                         int* charId, char* name, int nameCap) {
	*charId = -1;
	NarrowName(nullptr, name, nameCap);
	if (pc && pc->PlayerReplicationInfo && pc->PlayerReplicationInfo->PlayerName.Data) {
		NarrowName(pc->PlayerReplicationInfo->PlayerName.Data, name, nameCap);
	}
	if (pawn && ObjectClassCache::ClassNameContains(pawn, "TgPawn_Character")) {
		*charId = ((ATgPawn_Character*)pawn)->s_nCharacterId;
	}
}

static const char* ModeName(bool jetting, unsigned char phys) {
	if (jetting) return "jet";
	if (phys == 2) return "fall";
	if (phys == 1) return "walk";
	return "other";
}

static void LogSummary(APlayerController* pc, State& s, const char* why) {
	if (s.samples <= 0) return;
	int charId = -1;
	char name[64];
	FillIdentity(pc, s.pawn, &charId, name, sizeof(name));
	const float overPct = 100.0f * (float)s.overSamples / (float)s.samples;
	Logger::Log(CH,
		"[SUMMARY] why=%s name='%s' char=%d pc=%p samples=%d over=%d overPct=%.1f "
		"triggers=%d maxRatio=%.2f\n",
		why, name, charId, (void*)pc, s.samples, s.overSamples, overPct,
		s.triggers, s.maxRatio);
}

}  // namespace

void Observe(APlayerController* pc, float timestamp) {
	if (!Logger::IsChannelEnabled(CH) || !pc) return;

	APawn* pawn = pc->Pawn;
	State& s = g_state[pc];

	if (!pawn) {
		s.haveLast = false;
		s.pawn = nullptr;
		s.streak = 0;
		s.streakCounted = false;
		return;
	}

	if (s.pawn != pawn) {
		s.pawn = pawn;
		s.haveLast = false;
		s.streak = 0;
		s.streakCounted = false;
	}

	const float x = pawn->Location.X;
	const float y = pawn->Location.Y;
	const float z = pawn->Location.Z;

	if (!s.haveLast) {
		s.lastTs = timestamp;
		s.lastX = x;
		s.lastY = y;
		s.lastZ = z;
		s.haveLast = true;
		return;
	}

	const float dt = timestamp - s.lastTs;
	s.lastTs = timestamp;
	const float dx = x - s.lastX;
	const float dy = y - s.lastY;
	const float dz = z - s.lastZ;
	s.lastX = x;
	s.lastY = y;
	s.lastZ = z;

	if (dt < kMinDt || dt > kMaxDt || dt <= 0.0f) {
		s.streak = 0;
		s.streakCounted = false;
		return;
	}

	const float distH = std::sqrt(dx * dx + dy * dy);
	const float dist3 = std::sqrt(dx * dx + dy * dy + dz * dz);
	if (dist3 > kMaxStepUu) {
		s.streak = 0;
		s.streakCounted = false;
		return;
	}

	const char* pcState = pc->GetStateName().GetName();
	const bool jetting =
		(pawn->Physics == 4) ||
		(pcState && std::strstr(pcState, "Jet") != nullptr);

	const float cap = jetting
		? (pawn->AirSpeed > 1.0f ? pawn->AirSpeed : 595.0f)
		: (pawn->GroundSpeed > 1.0f ? pawn->GroundSpeed : 289.0f);
	const float dist = jetting ? dist3 : distH;
	const float observed = dist / dt;
	const float ratio = observed / cap;

	s.samples++;
	s.sumDt += dt;
	if (ratio > s.maxRatio) s.maxRatio = ratio;
	if (ratio >= kOverRatio) s.overSamples++;

	if (ratio >= kOverRatio) {
		s.streak++;
		if (s.streak >= kOverStreak && !s.streakCounted) {
			s.triggers++;
			s.streakCounted = true;
			if ((timestamp - s.lastOverLogTs) >= kOverLogCooldownSec) {
				s.lastOverLogTs = timestamp;
				int charId = -1;
				char name[64];
				FillIdentity(pc, pawn, &charId, name, sizeof(name));
				Logger::Log(CH,
					"[OVER] name='%s' char=%d pc=%p mode=%s phys=%d streak=%d "
					"ratio=%.2f obs=%.0f cap=%.0f dt=%.3f gs=%.0f air=%.0f "
					"loc=(%.0f,%.0f,%.0f)\n",
					name, charId, (void*)pc, ModeName(jetting, pawn->Physics),
					(int)pawn->Physics, s.streak, ratio, observed, cap, dt,
					pawn->GroundSpeed, pawn->AirSpeed, x, y, z);
			}
		}
	} else if (ratio < kClearRatio) {
		s.streak = 0;
		s.streakCounted = false;
	}

	if (s.sumDt >= kSummaryPeriodSec) {
		LogSummary(pc, s, "period");
		s.sumDt = 0.0f;
		// Keep samples/over/triggers/maxRatio for the whole session so
		// period lines are comparable; only the timer resets.
	}
}

void OnControllerDestroyed(APlayerController* pc) {
	if (!pc) return;
	auto it = g_state.find(pc);
	if (it == g_state.end()) return;
	if (Logger::IsChannelEnabled(CH)) {
		LogSummary(pc, it->second, "disconnect");
	}
	g_state.erase(it);
}

}  // namespace MoveSpeedWatch
