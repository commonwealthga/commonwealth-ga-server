#include "src/GameServer/TgGame/TgPawn/GetWeaponStartTraceLocationFromSocketOffsetInfo/TgPawn__GetWeaponStartTraceLocationFromSocketOffsetInfo.hpp"
#include "src/Database/SocketCycle/SocketCycle.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {

	// Boss Shrike (TgPawn_Boss_Destroyer, body_asm_id 794) shoulder offsets
	// in pawn-local coords (X=fwd, Y=right, Z=up). Derived from the asset's
	// BoxExtent + WSO_Emit socket offsets — see mesh-socket-obj-dump-guide.md
	// for the derivation. Y=±59.68 is tight (geometric); Z=+63 is upper-third
	// estimate, iterated visually.
	constexpr int   kShrikeBodyAsmId = 794;
	constexpr float kShrikeShoulderY = 59.68f;
	constexpr float kShrikeShoulderZ = 63.0f;

	// UE3 rotator units: 65536 = 360°.
	constexpr float kRotUnitToRad = 3.14159265358979323846f / 32768.0f;

	// Yaw-rotate a pawn-local vector into world space. UE3: X=forward,
	// Y=right, Z=up; yaw rotates around Z.
	static inline void YawRotate(const FVector& local, int yawUnits, FVector* out) {
		const float a = (float)yawUnits * kRotUnitToRad;
		const float c = cosf(a);
		const float s = sinf(a);
		out->X = local.X * c - local.Y * s;
		out->Y = local.X * s + local.Y * c;
		out->Z = local.Z;
	}

}  // namespace

FVector* __fastcall TgPawn__GetWeaponStartTraceLocationFromSocketOffsetInfo::Call(
	ATgPawn* Pawn, void* /*edx*/, FVector* outLoc, ATgDevice* Dev)
{
	// Default: pawn body Location (matches the stock fall-through behavior
	// when no socket info applies).
	if (Pawn) {
		outLoc->X = Pawn->Location.X;
		outLoc->Y = Pawn->Location.Y;
		outLoc->Z = Pawn->Location.Z;
	} else {
		outLoc->X = 0.0f;
		outLoc->Y = 0.0f;
		outLoc->Z = 0.0f;
		return outLoc;  // MSVC struct-return: caller derefs returned pointer
	}

	if (!Dev) return outLoc;
	if (Dev->m_nSocketMax < 2) return outLoc;    // single-socket weapon
	const int slot = Dev->m_nSocketIndex;
	if (slot != 1 && slot != 2) return outLoc;

	const int asmId = SocketCycle::GetBodyAsmId(Pawn);
	if (asmId != kShrikeBodyAsmId) return outLoc; // POC: Shrike only

	// Slot→side mapping (verified 2026-05-15 on Shrike, matches the same
	// inversion observed earlier in InitializeProjectile):
	//   slot 1 → bot's RIGHT shoulder → +Y
	//   slot 2 → bot's LEFT shoulder  → -Y
	// Despite the WSO_Origin_01/02 naming suggesting "01=Left, 02=Right",
	// the DB display_order in asm_data_set_asm_mesh_fxs lists them
	// right-first for Shrike. So slot 1 (= vec[0] in SocketCycle) = right.
	// Same convention works at this trace-layer hook AND at the InitProj
	// layer; if a future bot's projectiles come out swapped, this is the
	// only line to flip.
	const float yOff = (slot == 1) ? +kShrikeShoulderY : -kShrikeShoulderY;
	const FVector local(0.0f, yOff, kShrikeShoulderZ);

	FVector world;
	YawRotate(local, Pawn->Rotation.Yaw, &world);

	outLoc->X = Pawn->Location.X + world.X;
	outLoc->Y = Pawn->Location.Y + world.Y;
	outLoc->Z = Pawn->Location.Z + world.Z;

	if (Logger::IsChannelEnabled("shrike_trace")) {
		Logger::Log("shrike_trace",
			"[ShrikeTrace] slot=%d  yaw=%d  pawn=(%.1f,%.1f,%.1f)  trace=(%.1f,%.1f,%.1f)\n",
			slot, Pawn->Rotation.Yaw,
			Pawn->Location.X, Pawn->Location.Y, Pawn->Location.Z,
			outLoc->X, outLoc->Y, outLoc->Z);
	}

	// MSVC struct-return-by-pointer convention: caller's thunk reads EAX as
	// pointer and derefs it. Always return outLoc.
	return outLoc;
}
