#include "src/GameServer/TgGame/TgPawn_Scanner/UpdateSensorSweep/TgPawn_Scanner__UpdateSensorSweep.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>

namespace {

// Vertical grace for the target's body: a beam clipping a standing pawn's
// head/feet still counts. Human collision half-height is 46.
constexpr float kTargetHalfHeight = 46.0f;

// Clockwise viewed from above = yaw-INCREASING in UE3 (left-handed, Z-up).
// Confirmed by playtest: with -1 the detection fired mirrored (beam visually
// ~opposite the player at rel=0.70 rev; real crossing was at 0.30 rev).
constexpr int kSpinDir = 1;

// Diagnostic heartbeat cadence while sweeping (alarm channel).
constexpr float kHeartbeatInterval = 2.0f;

// Per-scanner sweep state. The arc is SIMULATED from the sweep start
// (UC: SpinToTargetRotation(rot(0,65535,0), m_fScanSpeed) = one revolution
// over m_fScanSpeed seconds, anchored at the pawn's facing) — whether the
// server-side actor rotation also spins is unresolved; the heartbeat logs
// Rotation.Yaw so a capture settles it. Keyed by the object's FName
// (Index, Number) — unique per live instance.
struct SweepState {
	bool  bActive;
	float fStartTime;
	int   nStartYaw;
	int   nLastOffset;   // yaw units swept so far [0..65535]
	float fNextHeartbeat;
};
std::map<uint64_t, SweepState> s_sweeps;

uint64_t ScannerKey(ATgPawn_Scanner* s) {
	uint32_t number = 0;
	std::memcpy(&number, ((const char*)&s->Name) + 4, sizeof(number));
	return (static_cast<uint64_t>(static_cast<uint32_t>(s->Name.Index)) << 32) | number;
}

// INTACT natives. DidSensorBeamDetectEnemy = retail validity (enemy-team via
// vtbl+0x140, stealth-visibility, alive); EnemyDetectedTimer = retail alert:
// SetPhase(3) (vtbl+0x6B4) → UC OnPhaseChange fires DoAlarm; the behavior's
// "PHASE 3 - SOUND ALARM" action 620 then RadioAlarms on its cooldown.
using DidDetectFn     = int(__thiscall*)(ATgPawn_Scanner*, AActor*);
using EnemyDetectedFn = void(__fastcall*)(ATgPawn_Scanner*);
constexpr uintptr_t kDidSensorBeamDetectEnemy = 0x109e8570;
constexpr uintptr_t kEnemyDetectedTimer       = 0x109c9890;

}  // namespace

void __fastcall TgPawn_Scanner__UpdateSensorSweep::Call(ATgPawn_Scanner* Scanner, void* edx, float DeltaSeconds) {
	// Client half (beam FX; NetMode-gated no-op on a dedicated server).
	CallOriginal(Scanner, edx, DeltaSeconds);

	if (Scanner == nullptr) return;

	SweepState& st = s_sweeps[ScannerKey(Scanner)];

	if (!Scanner->m_bIsSensorSweeping || Scanner->s_bEnemyDetected ||
	    Scanner->r_nPhase != 1 || Scanner->Health <= 0) {
		st.bActive = false;
		return;
	}

	AWorldInfo* WI = Scanner->WorldInfo;
	if (WI == nullptr) return;
	const float now = WI->TimeSeconds;
	const float scanSpeed = Scanner->m_fScanSpeed > 0.0f ? Scanner->m_fScanSpeed : 10.0f;

	// The engine only dispatches this native WHILE a sweep is active, so we
	// never see a call with m_bIsSensorSweeping==false between sweeps and the
	// guard above can't reset st. A call with stale state (previous sweep
	// over by > scanSpeed+1, when UC's SensorSweepComplete timer fires) must
	// therefore be the START of a new sweep — re-anchor instead of wedging on
	// nLastOffset==65535.
	const bool stale = st.bActive && (now - st.fStartTime > scanSpeed + 1.5f);

	if (!st.bActive || stale || st.fStartTime > now) {
		st.bActive        = true;
		st.fStartTime     = now;
		st.nStartYaw      = Scanner->Rotation.Yaw;
		st.nLastOffset    = 0;
		st.fNextHeartbeat = now + kHeartbeatInterval;
		Logger::Log("alarm",
			"[%s] Scanner key=%llx sweep %s (facing yaw=%d, %.0fs/rev, range=%.0f angle=%.2f eyeZ=+%.0f)\n",
			Logger::GetTime(), (unsigned long long)ScannerKey(Scanner),
			stale ? "RE-started (stale state — caller is sweep-gated)" : "started",
			Scanner->Rotation.Yaw & 0xFFFF, scanSpeed,
			Scanner->m_fSensorRange, Scanner->m_fSensorAngle, Scanner->BaseEyeHeight);
		return;
	}

	const float elapsed = now - st.fStartTime;
	const int curOffset = elapsed >= scanSpeed
		? 65535
		: static_cast<int>(65536.0f * (elapsed / scanSpeed));
	if (curOffset <= st.nLastOffset) return;

	const float range = Scanner->m_fSensorRange > 0.0f ? Scanner->m_fSensorRange : 1200.0f;
	const float rangeSq = range * range;
	// Vertical fan envelope: the binary's per-beam offset is
	// range·tan(m_fSensorAngle·(0.5 − i/(n−1))) → half-angle =
	// m_fSensorAngle/2 RADIANS (default 0.5 → ±~14.3°).
	const float halfAngle = (Scanner->m_fSensorAngle > 0.0f ? Scanner->m_fSensorAngle : 0.5f) * 0.5f;
	const float tanHalf   = std::tan(halfAngle);

	// Beam origin: the emitter sits at head height above the pawn center.
	FVector eye = Scanner->Location;
	eye.Z += Scanner->BaseEyeHeight > 0.0f ? Scanner->BaseEyeHeight : 40.0f;

	const bool heartbeat = now >= st.fNextHeartbeat;
	if (heartbeat) {
		st.fNextHeartbeat = now + kHeartbeatInterval;
		Logger::Log("alarm",
			"[%s] Scanner key=%llx sweeping: offset=%d/65536 actorYaw=%d (start=%d — moves iff server spins)\n",
			Logger::GetTime(), (unsigned long long)ScannerKey(Scanner),
			curOffset, Scanner->Rotation.Yaw & 0xFFFF, st.nStartYaw & 0xFFFF);
	}

	for (const auto& kv : GClientConnectionsData) {
		ATgPawn_Character* P = kv.second.Pawn;
		if (P == nullptr || P->Health <= 0) continue;

		const float dx = P->Location.X - eye.X;
		const float dy = P->Location.Y - eye.Y;
		const float dz = P->Location.Z - eye.Z;
		const float distSq = dx * dx + dy * dy + dz * dz;
		const float horiz  = std::sqrt(dx * dx + dy * dy);
		const float envelope = horiz * tanHalf + kTargetHalfHeight;

		const int bearing = static_cast<int>(std::atan2(dy, dx) * (32768.0f / 3.14159265f));
		const int rel     = (kSpinDir * (bearing - st.nStartYaw)) & 0xFFFF;
		const bool crossed = rel > st.nLastOffset && rel <= curOffset;

		if (heartbeat) {
			Logger::Log("alarm",
				"  candidate %s: dist=%.0f (range %.0f) bearingRel=%d dz=%.0f (envelope %.0f) crossedThisFrame=%d\n",
				P->GetName(), std::sqrt(distSq), range, rel, dz, envelope, (int)crossed);
		}

		if (distSq > rangeSq) continue;
		if (!crossed) continue;

		if (std::fabs(dz) > envelope) {
			Logger::Log("alarm",
				"[%s] Scanner key=%llx beam crossed %s but OUTSIDE vertical envelope (dz=%.0f > %.0f)\n",
				Logger::GetTime(), (unsigned long long)ScannerKey(Scanner),
				P->GetName(), dz, envelope);
			continue;
		}

		// Retail validity: enemy team + stealth-visibility + alive.
		if (!((DidDetectFn)kDidSensorBeamDetectEnemy)(Scanner, (AActor*)P)) continue;

		// Beam LOS — must reach the pawn unobstructed.
		FVector hitLoc, hitNormal;
		FVector extent = {0.0f, 0.0f, 0.0f};
		FTraceHitInfo hitInfo = {};
		AActor* hit = Scanner->Trace(P->Location, eye, 1, extent, 0, &hitLoc, &hitNormal, &hitInfo);
		if (hit != (AActor*)P) {
			Logger::Log("alarm",
				"[%s] Scanner key=%llx beam crossed %s but LOS blocked (hit=%s)\n",
				Logger::GetTime(), (unsigned long long)ScannerKey(Scanner),
				P->GetName(), hit ? hit->GetName() : "<nothing>");
			continue;
		}

		Scanner->r_Target = (ATgPawn*)P;
		((EnemyDetectedFn)kEnemyDetectedTimer)(Scanner);

		Logger::Log("alarm",
			"[%s] Scanner key=%llx beam passed over %s (rel=%d arc=[%d..%d] dist=%.0f dz=%.0f) -> phase 3\n",
			Logger::GetTime(), (unsigned long long)ScannerKey(Scanner),
			P->GetName(), rel, st.nLastOffset, curOffset,
			std::sqrt(distSq), dz);
		st.bActive = false;
		return;
	}

	st.nLastOffset = curOffset;
}
