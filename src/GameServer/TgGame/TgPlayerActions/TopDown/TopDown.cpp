#include "src/GameServer/TgGame/TgPlayerActions/TopDown/TopDown.hpp"

#include <cstring>
#include <map>

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TgPlayerActions::TopDownCmd {

namespace {

// UE3 FRotator units: 65536 per full circle. -16384 = -90° = pitch straight down.
constexpr int   kPitchDown      = -16384;
// World-units; UE3 measures in cm, so 30000 ≈ 300m above the pawn's feet.
constexpr float kDefaultLiftZ   = 30000.0f;
// Sanity clamp — if the user passes a wild value we still don't want to
// underflow the float exponent or get clipped by the engine's worldlimit.
constexpr float kMinLiftZ       = 100.0f;
constexpr float kMaxLiftZ       = 200000.0f;

struct Snapshot {
    FVector  location;
    FRotator pawn_rotation;
    FRotator ctrl_rotation;
    FVector  velocity;
    unsigned char physics;
    bool sim_gravity;
    bool collide_world;
    bool collide_actors;
    bool can_fly;
    bool jump_capable;
    // Stored so we can detect death+respawn between toggle calls.
    ATgPawn_Character* original_pawn;
};

// Snapshots keyed by session_guid so reconnects / mid-session lifecycle
// changes can't strand the saved state (same pattern PossessCmd uses).
std::map<std::string, Snapshot> g_snapshots;

ATgPawn_Character* FindPawnBySessionGuid(const std::string& guid) {
    for (auto& kv : GClientConnectionsData) {
        if (kv.second.SessionGuid == guid) return kv.second.Pawn;
    }
    return nullptr;
}

bool IsPlayerController(AController* c) {
    if (!c || !c->Class) return false;
    const char* full = c->Class->GetFullName();
    return full && std::strstr(full, "PlayerController");
}

void Audit(const std::string& guid,
           const std::string& outcome, const std::string& details) {
    IpcClient::SendChatCommandAudit(guid, "-topdown", outcome, details);
}

void ApplyTopDown(ATgPawn_Character* pawn, float lift_z) {
    pawn->Location.Z       += lift_z;
    pawn->Velocity.X = pawn->Velocity.Y = pawn->Velocity.Z = 0.0f;
    pawn->Physics           = 0;   // PHYS_None — pawn freezes in place, no gravity
    pawn->bSimulateGravity  = 0;
    pawn->bCollideWorld     = 0;   // don't get punted by ceilings / volumes
    pawn->bCollideActors    = 0;
    pawn->bCanFly           = 1;
    pawn->bJumpCapable      = 0;

    ATgPlayerController* pc = (ATgPlayerController*)pawn->Controller;
    pc->Rotation.Pitch  = kPitchDown;
    pc->Rotation.Roll   = 0;
    pawn->Rotation.Pitch = kPitchDown;
    pawn->Rotation.Roll  = 0;
    // GetPlayerViewPoint reads c_nCameraYawOffset / c_nCameraPitchOffset for
    // most non-Walking states, and our ProcessEvent intercept re-syncs them
    // from Controller.Rotation right before CalcCameraView — but we set them
    // here too so the very first frame after applying renders top-down even
    // if no GetPlayerViewPoint roundtrip has happened yet.
    pc->c_nCameraYawOffset   = pc->Rotation.Yaw;
    pc->c_nCameraPitchOffset = pc->Rotation.Pitch;
}

void Restore(ATgPawn_Character* pawn, const Snapshot& s) {
    pawn->Location          = s.location;
    pawn->Velocity          = s.velocity;
    pawn->Rotation          = s.pawn_rotation;
    pawn->Physics           = s.physics;
    pawn->bSimulateGravity  = s.sim_gravity   ? 1 : 0;
    pawn->bCollideWorld     = s.collide_world ? 1 : 0;
    pawn->bCollideActors    = s.collide_actors? 1 : 0;
    pawn->bCanFly           = s.can_fly       ? 1 : 0;
    pawn->bJumpCapable      = s.jump_capable  ? 1 : 0;

    ATgPlayerController* pc = (ATgPlayerController*)pawn->Controller;
    pc->Rotation             = s.ctrl_rotation;
    pc->c_nCameraYawOffset   = pc->Rotation.Yaw;
    pc->c_nCameraPitchOffset = pc->Rotation.Pitch;
}

} // namespace

void Execute(const std::string& session_guid, float lift_z) {
    ATgPawn_Character* pawn = FindPawnBySessionGuid(session_guid);
    if (!pawn) {
        Logger::Log("chat-command",
            "[ChatCmd][TopDown] guid=%s: no pawn found\n", session_guid.c_str());
        Audit(session_guid, "ignored", "no player pawn");
        return;
    }
    if (!IsPlayerController(pawn->Controller)) {
        Logger::Log("chat-command",
            "[ChatCmd][TopDown] guid=%s: session controller isn't a PlayerController\n",
            session_guid.c_str());
        Audit(session_guid, "ignored", "session controller is not PlayerController");
        return;
    }

    auto it = g_snapshots.find(session_guid);
    if (it != g_snapshots.end()) {
        // Toggle off — restore (best-effort if pawn changed).
        if (it->second.original_pawn == pawn) {
            Restore(pawn, it->second);
            Logger::Log("chat-command",
                "[ChatCmd][TopDown] guid=%s: restored\n", session_guid.c_str());
            Audit(session_guid, "restored", "topdown off");
        } else {
            Logger::Log("chat-command",
                "[ChatCmd][TopDown] guid=%s: pawn changed since snapshot — "
                "discarding snapshot without restore\n", session_guid.c_str());
            Audit(session_guid, "ignored", "pawn changed since snapshot");
        }
        g_snapshots.erase(it);
        return;
    }

    // Toggle on — snapshot then apply.
    Snapshot s;
    s.location        = pawn->Location;
    s.velocity        = pawn->Velocity;
    s.pawn_rotation   = pawn->Rotation;
    s.ctrl_rotation   = pawn->Controller->Rotation;
    s.physics         = pawn->Physics;
    s.sim_gravity     = pawn->bSimulateGravity  != 0;
    s.collide_world   = pawn->bCollideWorld     != 0;
    s.collide_actors  = pawn->bCollideActors    != 0;
    s.can_fly         = pawn->bCanFly           != 0;
    s.jump_capable    = pawn->bJumpCapable      != 0;
    s.original_pawn   = pawn;
    g_snapshots[session_guid] = s;

    float dz = lift_z;
    if (dz <= 0.0f)        dz = kDefaultLiftZ;
    if (dz < kMinLiftZ)    dz = kMinLiftZ;
    if (dz > kMaxLiftZ)    dz = kMaxLiftZ;

    ApplyTopDown(pawn, dz);

    Logger::Log("chat-command",
        "[ChatCmd][TopDown] guid=%s: applied (lift_z=%.0f, target Z=%.0f)\n",
        session_guid.c_str(), dz, pawn->Location.Z);
    Audit(session_guid, "activated",
        "lift_z=" + std::to_string((int)dz)
        + " target_z=" + std::to_string((int)pawn->Location.Z));
}

} // namespace TgPlayerActions::TopDownCmd
