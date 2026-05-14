#include "src/GameServer/TgGame/TgPlayerActions/SpawnBot/SpawnBot.hpp"

#include <cmath>

#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TgPlayerActions::SpawnBotCmd {

namespace {

ATgPawn_Character* FindPawnBySessionGuid(const std::string& guid) {
    for (auto& kv : GClientConnectionsData) {
        if (kv.second.SessionGuid == guid) {
            return kv.second.Pawn;
        }
    }
    return nullptr;
}

const char* TeamName(Team t) {
    switch (t) {
        case Team::Attackers: return "attackers";
        case Team::Defenders: return "defenders";
    }
    return "?";
}

// Pi as float; we don't pull in numbers because of MSVC defines.
constexpr float kPi = 3.14159265358979323846f;

// Distance ahead of the player to drop the bot, in UE3 units. Pawn collision
// cylinders run ~80 UU radius for character-class bots; 500 UU is far enough
// that the spawn never overlaps the player but close enough to be obviously
// "in front of me" from the player's POV.
constexpr float kSpawnDistanceUU = 500.0f;

// Vertical floor buffer applied on top of the bot's halfHeight when computing
// the spawn Z. Matches the +5uu pattern used by SpawnPet / SpawnDeployable so
// the cylinder bottom sits just above the ground rather than intersecting it.
constexpr float kSpawnFloorBufferUU = 5.0f;

} // namespace

void Execute(const std::string& session_guid, int bot_id, Team team) {
    ATgPawn_Character* Pawn = FindPawnBySessionGuid(session_guid);
    if (!Pawn) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /spawnbot: no pawn for guid=%s; dropping\n",
            session_guid.c_str());
        return;
    }

    // Use the controller's yaw when present (this is what the player is
    // actually aiming) and fall back to the pawn's body yaw otherwise.
    int yaw = Pawn->Controller ? Pawn->Controller->Rotation.Yaw : Pawn->Rotation.Yaw;
    float yaw_rad = static_cast<float>(yaw) * (kPi / 32768.0f);

    // Compute spawn Z from ground + bot.halfHeight + buffer. Pawn->Location.Z
    // is at the PLAYER's cylinder center; subtract the player's halfHeight to
    // get an approximate ground reference at the player's feet, then add the
    // BOT's halfHeight + buffer to place its cylinder center above ground.
    // Without this, taller bots' cylinder centers sit too low → cylinder
    // bottom below ground → mesh sinks (knees clip for crouchable rigs,
    // straight-clip for everything else).
    float botRadius = 0.f, botHalfHeight = 0.f;
    TgGame__SpawnBotById::GetBotCollisionCylinder(bot_id, &botRadius, &botHalfHeight);
    // CDO default player halfHeight; close enough for the eyeball-altitude lift.
    constexpr float kPlayerCDOHalfHeight = 46.0f;

    FVector loc;
    loc.X = Pawn->Location.X + std::cos(yaw_rad) * kSpawnDistanceUU;
    loc.Y = Pawn->Location.Y + std::sin(yaw_rad) * kSpawnDistanceUU;
    loc.Z = (Pawn->Location.Z - kPlayerCDOHalfHeight) + botHalfHeight + kSpawnFloorBufferUU;

    FRotator rot;
    rot.Pitch = 0;
    // Make the bot face the player (its yaw = player yaw + 180°).
    rot.Yaw   = yaw + 32768;
    rot.Roll  = 0;

    ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
    if (!Game) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /spawnbot guid=%s: GGameInfo null; dropping\n",
            session_guid.c_str());
        return;
    }

    Logger::Log("chat-command",
        "[ChatCmd][DLL] /spawnbot guid=%s bot_id=%d team=%s loc=(%.1f,%.1f,%.1f) yaw=%d\n",
        session_guid.c_str(), bot_id, TeamName(team),
        loc.X, loc.Y, loc.Z, yaw);

    // bIgnoreCollision=true so a tight spot in front of the player doesn't
    // veto the spawn. pFactory=nullptr -> SpawnBotById falls back to
    // ActorCache::BotFactory for the AI behavior/patrol scaffolding.
    ATgPawn* Bot = TgGame__SpawnBotById::Call(
        Game,
        nullptr,
        bot_id,
        loc,
        rot,
        /*bKillController*/   false,
        /*pFactory*/          nullptr,
        /*bIgnoreCollision*/  true,
        /*pOwnerPawn*/        nullptr,
        /*bIsDecoy*/          false,
        /*deviceFire*/        nullptr,
        /*fDeployAnimLength*/ 0.0f
    );

    if (!Bot) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /spawnbot guid=%s bot_id=%d: SpawnBotById returned null\n",
            session_guid.c_str(), bot_id);
        return;
    }

    // Team assignment mirrors TgBotFactory::SpawnNextBot — without it the
    // BotRepInfo's r_TaskForce stays null and CheckIsEnemy resolves the bot
    // to "enemy of everyone" / inconsistent state.
    ATgRepInfo_Player* BotRep = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
    if (BotRep) {
        ATgRepInfo_TaskForce* tf =
            (team == Team::Attackers) ? GTeamsData.Attackers : GTeamsData.Defenders;
        BotRep->r_TaskForce = tf;
        BotRep->Team        = tf;
        BotRep->SetTeam(tf);
        Bot->NotifyTeamChanged();
    }

    // Mirror SpawnNextBot's post-spawn replication kick.
    Bot->Role = 3;
    Bot->RemoteRole = 1;
    Bot->bNetDirty = 1;
    Bot->bNetInitial = 1;
    Bot->bForceNetUpdate = 1;
    Bot->bSkipActorPropertyReplication = 0;

    if (BotRep) {
        BotRep->Role = 3;
        BotRep->RemoteRole = 1;
        BotRep->bNetDirty = 1;
        BotRep->bNetInitial = 1;
        BotRep->bForceNetUpdate = 1;
        BotRep->bSkipActorPropertyReplication = 0;
    }

    // Post-spawn state snapshot. If the bot disappears immediately after this
    // line returns (KillZ, AI cleanup, replication issue, etc.), this snapshot
    // is the last record of its in-world state. If Health is 0 or bDeleteMe is
    // 1 here, the bot was born dead — investigate SpawnBotById/InitBehavior.
    Logger::Log("chat-command",
        "[ChatCmd][DLL] /spawnbot snapshot: bot=%p bDeleteMe=%d Role=%d "
        "RemoteRole=%d Health=%d Loc=(%.1f,%.1f,%.1f) BotRep=%p team=%s\n",
        (void*)Bot, (int)Bot->bDeleteMe, (int)Bot->Role, (int)Bot->RemoteRole,
        Bot->Health, Bot->Location.X, Bot->Location.Y, Bot->Location.Z,
        (void*)BotRep, TeamName(team));

    Logger::Log("chat-command",
        "[ChatCmd][DLL] /spawnbot guid=%s: bot=%p (botId=%d) team=%s spawned\n",
        session_guid.c_str(), (void*)Bot, bot_id, TeamName(team));
}

} // namespace TgPlayerActions::SpawnBotCmd
