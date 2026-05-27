#include "src/GameServer/TgGame/_surface_rotation/SurfaceRotation.hpp"

#include <cmath>

namespace {

constexpr float kPi          = 3.14159265358979323846f;
constexpr float kRadToUEUnit = 32768.0f / kPi;  // UE3 FRotator units per radian
constexpr float kEps         = 1e-4f;

struct V3 { float X, Y, Z; };

inline V3   v3(const FVector& v)                { return {v.X, v.Y, v.Z}; }
inline V3   sub(const V3& a, const V3& b)       { return {a.X-b.X, a.Y-b.Y, a.Z-b.Z}; }
inline V3   scale(const V3& a, float s)         { return {a.X*s, a.Y*s, a.Z*s}; }
inline float dot(const V3& a, const V3& b)      { return a.X*b.X + a.Y*b.Y + a.Z*b.Z; }
inline V3   cross(const V3& a, const V3& b)     {
	return { a.Y*b.Z - a.Z*b.Y,
	         a.Z*b.X - a.X*b.Z,
	         a.X*b.Y - a.Y*b.X };
}
inline float lenSq(const V3& a)                  { return dot(a, a); }
inline V3   normalize(const V3& a, const V3& fb) {
	const float l2 = lenSq(a);
	if (l2 < kEps * kEps) return fb;
	const float inv = 1.0f / std::sqrt(l2);
	return scale(a, inv);
}

// Round-to-nearest-int helper (UE3's appRound). Plain truncation is biased.
inline int rRound(float f) {
	return (f >= 0.0f) ? (int)(f + 0.5f) : -(int)(-f + 0.5f);
}

}  // namespace

namespace SurfaceRotation {

FRotator FromSurfaceNormal(const FVector& surfaceNormal,
                            const FVector& playerFacing) {
	// ---- Step 1: Z = normalize(surfaceNormal). Degenerate input → world up.
	const V3 worldUp      = {0.0f, 0.0f, 1.0f};
	const V3 worldForward = {1.0f, 0.0f, 0.0f};
	const V3 worldRight   = {0.0f, 1.0f, 0.0f};

	const V3 Z = normalize(v3(surfaceNormal), worldUp);

	// ---- Step 2-3: X = playerFacing projected onto surface plane.
	V3 facing = v3(playerFacing);
	V3 X_proj = sub(facing, scale(Z, dot(facing, Z)));
	if (lenSq(X_proj) < kEps * kEps) {
		// Player is looking straight along the normal — pick world forward
		// projected against Z; if THAT degenerates (normal ≈ ±X) use right.
		X_proj = sub(worldForward, scale(Z, dot(worldForward, Z)));
		if (lenSq(X_proj) < kEps * kEps) {
			X_proj = sub(worldRight, scale(Z, dot(worldRight, Z)));
		}
	}
	const V3 X = normalize(X_proj, worldForward);

	// ---- Step 4: Y = Z × X (right-hand basis).
	const V3 Y = cross(Z, X);

	// ---- Step 5: Convert basis (X, Y, Z) → FRotator.
	//
	// Verbatim port of FMatrix::Rotator (UE3 UnMath.cpp:577):
	//   Pitch = atan2(X.Z, sqrt(X.X² + X.Y²)) * 32768/PI
	//   Yaw   = atan2(X.Y, X.X)               * 32768/PI
	//   SYAxis (Y-axis of FRotationMatrix(Pitch, Yaw, 0)) = (-sin(Yaw), cos(Yaw), 0)
	//     — derived from FRotationTranslationMatrix's M[1] row with SR=0, CR=1
	//   Roll  = atan2(Z · SYAxis, Y · SYAxis) * 32768/PI
	const float xyLen = std::sqrt(X.X * X.X + X.Y * X.Y);
	const float pitchRad = std::atan2(X.Z, xyLen);
	const float yawRad   = std::atan2(X.Y, X.X);

	const float sy = std::sin(yawRad);
	const float cy = std::cos(yawRad);
	const V3   SYAxis = { -sy, cy, 0.0f };

	const float rollRad = std::atan2(dot(Z, SYAxis), dot(Y, SYAxis));

	return FRotator(
		rRound(pitchRad * kRadToUEUnit),
		rRound(yawRad   * kRadToUEUnit),
		rRound(rollRad  * kRadToUEUnit));
}

}  // namespace SurfaceRotation
