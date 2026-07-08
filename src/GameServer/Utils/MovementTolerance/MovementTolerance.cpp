#include "src/GameServer/Utils/MovementTolerance/MovementTolerance.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cmath>
#include <unordered_map>

namespace MovementTolerance {

namespace {

constexpr float kMaxPingMs      = 200.0f;
constexpr float kBaseThreshXY   = 32.0f;   // ~1 m horizontal
constexpr float kBaseThreshZ    = 64.0f;   // ~2 m vertical
constexpr float kFlyingZMult    = 2.5f;
constexpr float kHardCapXY      = 500.0f;
constexpr float kHardCapZ       = 600.0f;
constexpr unsigned char kPhysFlying = 4;

std::unordered_map<APlayerController*, FVector> s_lastClientLoc;

float DistSqXY(const FVector& a, const FVector& b) {
	const float dx = a.X - b.X;
	const float dy = a.Y - b.Y;
	return dx * dx + dy * dy;
}

float DistSqZ(const FVector& a, const FVector& b) {
	const float dz = a.Z - b.Z;
	return dz * dz;
}

void TrustClientPosition(APawn* pawn, const FVector& clientLoc) {
	pawn->Location = clientLoc;
	pawn->bNetDirty = 1;
}

void QueueAck(APlayerController* pc) {
	pc->PendingAdjustment.bAckGoodMove = 1;
	pc->PendingAdjustment.newPhysics = 0;
}

} // namespace

float GetPingMs(APlayerController* pc) {
	if (!pc || !pc->PlayerReplicationInfo) return 0.0f;
	const APlayerReplicationInfo* pri = pc->PlayerReplicationInfo;
	if (pri->ExactPing > 0.0f) {
		return std::min(pri->ExactPing * 1000.0f, kMaxPingMs);
	}
	if (pri->Ping > 0) {
		return std::min(static_cast<float>(pri->Ping), kMaxPingMs);
	}
	return 0.0f;
}

Thresholds ComputeThresholds(APawn* pawn, float pingMs) {
	Thresholds t;
	const float pingScale = 1.0f + (pingMs / kMaxPingMs);
	const bool flying = pawn && pawn->Physics == kPhysFlying;
	t.xy = kBaseThreshXY * pingScale;
	t.z  = kBaseThreshZ * pingScale * (flying ? kFlyingZMult : 1.0f);
	return t;
}

bool IsWithinThreshold(const FVector& a, const FVector& b, const Thresholds& t) {
	return DistSqXY(a, b) <= t.xy * t.xy && DistSqZ(a, b) <= t.z * t.z;
}

bool ExceedsHardCap(const FVector& a, const FVector& b) {
	return DistSqXY(a, b) > kHardCapXY * kHardCapXY || DistSqZ(a, b) > kHardCapZ * kHardCapZ;
}

void RecordClientMove(APlayerController* pc, const FVector& clientLoc) {
	if (!pc) return;
	s_lastClientLoc[pc] = clientLoc;
}

bool TrySoftAuthorizeAfterMove(APlayerController* pc, APawn* pawn, const FVector& clientLoc) {
	if (!pc || !pawn) return false;

	const float pingMs = GetPingMs(pc);
	const Thresholds thresholds = ComputeThresholds(pawn, pingMs);
	const FVector& serverLoc = pawn->Location;

	if (ExceedsHardCap(clientLoc, serverLoc)) {
		if (Logger::IsChannelEnabled("movement")) {
			Logger::Log("movement",
				"hard-cap reject pc=%p ping=%.0f client=(%.1f,%.1f,%.1f) server=(%.1f,%.1f,%.1f)\n",
				pc, pingMs,
				clientLoc.X, clientLoc.Y, clientLoc.Z,
				serverLoc.X, serverLoc.Y, serverLoc.Z);
		}
		return false;
	}

	if (!IsWithinThreshold(clientLoc, serverLoc, thresholds)) {
		return false;
	}

	TrustClientPosition(pawn, clientLoc);
	QueueAck(pc);

	if (Logger::IsChannelEnabled("movement")) {
		Logger::Log("movement",
			"soft-auth move pc=%p ping=%.0f phys=%d threshXY=%.1f threshZ=%.1f "
			"client=(%.1f,%.1f,%.1f) serverWas=(%.1f,%.1f,%.1f)\n",
			pc, pingMs, (int)pawn->Physics, thresholds.xy, thresholds.z,
			clientLoc.X, clientLoc.Y, clientLoc.Z,
			serverLoc.X, serverLoc.Y, serverLoc.Z);
	}
	return true;
}

bool TrySuppressPendingAdjustment(APlayerController* pc, APawn* pawn) {
	if (!pc || !pawn) return false;

	FClientAdjustment& adj = pc->PendingAdjustment;
	if (adj.bAckGoodMove) return false;

	auto it = s_lastClientLoc.find(pc);
	if (it == s_lastClientLoc.end()) return false;

	const FVector& clientLoc = it->second;
	const FVector& serverLoc = pawn->Location;
	const float pingMs = GetPingMs(pc);
	const Thresholds thresholds = ComputeThresholds(pawn, pingMs);

	// Compare client report against both the live server pawn and the
	// pending correction target — either within the soft zone is enough.
	const bool nearServer = IsWithinThreshold(clientLoc, serverLoc, thresholds);
	const bool nearAdj    = IsWithinThreshold(clientLoc, adj.NewLoc, thresholds);

	if (!nearServer && !nearAdj) return false;
	if (ExceedsHardCap(clientLoc, serverLoc)) return false;

	TrustClientPosition(pawn, clientLoc);
	QueueAck(pc);

	if (Logger::IsChannelEnabled("movement")) {
		Logger::Log("movement",
			"suppressed correction pc=%p ping=%.0f phys=%d threshXY=%.1f threshZ=%.1f "
			"client=(%.1f,%.1f,%.1f) server=(%.1f,%.1f,%.1f) adj=(%.1f,%.1f,%.1f)\n",
			pc, pingMs, (int)pawn->Physics, thresholds.xy, thresholds.z,
			clientLoc.X, clientLoc.Y, clientLoc.Z,
			serverLoc.X, serverLoc.Y, serverLoc.Z,
			adj.NewLoc.X, adj.NewLoc.Y, adj.NewLoc.Z);
	}
	return true;
}

} // namespace MovementTolerance
