#include "src/GameServer/PvpBots/PvpBots.hpp"
#include "src/GameServer/Matchmaking/SetupRebalanceTrigger/SetupRebalanceTrigger.hpp"
#include "src/GameServer/PvpBots/BotLooks.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cwchar>
#include <map>
#include <queue>
#include <vector>

#include "src/GameServer/Core/FMallocWindows/GAllocator.hpp"
#include "src/GameServer/Cosmetics/CosmeticEquip.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/TgGame/TgPlayerActions/SpawnBot/SpawnBot.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/TgGame/TgAIController/InitBehavior/TgAIController__InitBehavior.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Engine/MapGameInfo/MapGameInfo.hpp"
#include "src/Config/Config.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Database/Database.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/SpawnPet/TgDeviceFire__SpawnPet.hpp"
#include "src/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectManager/BuildEffectGroup.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/AvaCargo/AvaCargoDll.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.hpp"

namespace PvpBots {

// Defined further down (objective play), used by target choice
bool IsCapturable(ATgMissionObjective* o);
AActor* ObjectiveGoalActor(ATgMissionObjective* o);
std::vector<ATgMissionObjective*> BotObjectives(AWorldInfo* wi, bool pooled);

namespace {

struct ClassKit {
    const char*    name;
    int            suitItemId;   // HoloSuit - <class> - Tier 1
    int            meleeDevice;  // level-1 class melee (equip point 1)
    int            rangedDevice; // level-2 class ranged (equip point 2)
    int            donorBotId;   // behavior donor (class bots have behavior 0)
    const wchar_t* botName;
    int            specialtyDevice = 0; // Equip point 3 (slot 981), 0 = none
};

// Device ids = item ids (equippable-devices.md). Donors: 1571 Field Medic
// (behavior 679, heal loop), 1432 Elite Techro (behavior 730, ranged fighter).
const ClassKit kKits[] = {
    { "assault",  6981, 3973, 6882, 1432, L"[BOT] Vance"  },
    { "medic",    6984, 3967, 4154, 1571, L"[BOT] Mercer", 2906 },  // BioFeedback Beam
    { "recon",    6986, 3970, 4946, 1432, L"[BOT] Shade",  2209 },  // Sprint Stealth
    { "robotics", 6988, 3951, 4166, 1432, L"[BOT] Bolt",   2300 },  // Personal Turret
};
// Robotics also carries the Focused Repair Arm (in-hand, slot 981, same
// beam behaviour as the medic beam) on the free equip point 4 (ES4) and uses
// the medic heal path on its own damaged turret.
// Assault carries the player's EMP Grenade (2022, lv4 offhand, slot 390)
// on the free equip point 3 and throws it through the device's own projectile
// fire (TgDevice.ProjectileFire), recharge from data prop 4 (60 s).
constexpr int kAssaultGrenade = 2022;
constexpr int kGrenadePoint = 3;
// Assault also carries the player's Concussion Grenade (2498, lv23
// offhand, slot 390) on equip point 4 (free on assault): close-range "jump
// slam" = short jetpack hop + concussion thrown down at the foe's feet.
constexpr int kConcGrenade = 2498;
constexpr int kConcPoint = 4;
// Medic carries the player's Healing Wave (2376, lv11 offhand, slot 390)
// on equip point 4 (free on medic). Data: mode 2869 radius prop 6 = 45,
// recharge prop 4 = 60 s, group 7342 health +600 + cleanse groups (req category).
constexpr int kHealWave = 2376;
constexpr int kWavePoint = 4;
// Medic also carries the player's Neutralize Wave (3642, lv15 offhand,
// slot 390) on equip point 5. Data: mode 3786 radius prop 6 = 35, recharge 60 s;
// OFFENSIVE: group 25752 health -115 on every opponent in the radius + 13
// "Remove Effect" groups, each only when the target carries that buff category
// (shields, regen, stims, damage/morale/power buffs, self heal): strip + -110.
constexpr int kNeutWave = 3642;
constexpr int kNeutPoint = 5;
// Robotics also carries the player's Force Wall (2051, lv3 offhand,
// slot 390) on equip point 5 (free on robotics). Data: deployable 76
// (TgDeploy_ForceField, health 2370), recharge prop 4 = 60 s, range 15.
constexpr int kForceWall = 2051;
constexpr int kWallPoint = 5;
constexpr int kWallDeployable = 76;
// Robotics loadout variety - half the robotics carry the player's Medical
// Station (2066, lv7 offhand, slot 390) on point 5 instead of Force Wall. Data:
// deployable 61 (health 1500, heal device 2064), recharge prop 4 = 60 s.
constexpr int kMedStation = 2066;
constexpr int kMedStationDeployable = 61;
// Some robotics carry the player's Power Station (4076, lv17 offhand) on
// point 5 instead of Force Wall. Data: deployable 128 (health 1500), recharge
// prop 4 = 50 s, deploy 15 s. Set down at the objective like the Medical Station.
constexpr int kPowerStation = 4076;
constexpr int kPowerStationDeployable = 128;
// Recon also carries the player's Decoy (2129, lv9 offhand, slot 390)
// on equip point 4 (free on recon). Data: SPAWN_PET fire mode, bot 735
// (TG_BOT_TYPE_DECOY, the spawn path copies the owner's look), life 20 s,
// recharge prop 4 = 60 s.
constexpr int kDecoy = 2129;
constexpr int kDecoyPoint = 4;
// Robotics also carries the player's Sensor (2326, lv15 offhand, slot 390)
// on equip point 6 (3rd offhand: turret, Force Wall, Sensor). Data: deployable 85
// (reveals stealthed enemies in its radius to its team), recharge prop 4 = 40 s.
constexpr int kSensor = 2326;
constexpr int kSensorPoint = 6;
constexpr int kSensorDeployable = 85;
// Robotics loadout variety - half the robotics carry the player's Eye Drone
// (2675, lv5 offhand, slot 390) on point 6 instead of Sensor. Data: pet bot 823
// (fire mode setup +0x2C), recharge prop 4 = 45 s.
constexpr int kEyeDrone = 2675;
constexpr int kEyeDroneBot = 823;
// Half the robotics carry the player's Grizzly Drone (2107, lv13 offhand)
// in the turret spot (point 3) instead of the Personal Turret. Data: pet bot 729,
// recharge prop 4 = 60 s. Placed by TickTurret like the turret.
constexpr int kGrizzly = 2107;
// Recon also carries the player's Standard Mine (2225, lv5 offhand, slot 390)
// on equip point 5. Data: fire mode 2738 -> projectile 158 -> deployable 81 "Mine"
// (400 hp, explodes via device 2311), recharge prop 4 = 10 s, max out prop 154 = 3
// (SpawnDeployableActor's EnforceDeployableLimit removes the oldest, like a player).
// Medic also carries the player's Poison Grenade (2168, lv5 offhand, slot 390)
// on equip point 6 (3rd offhand: Healing Wave, Neutralize Wave, Poison Grenade).
// Data: recharge prop 4 = 60 s, radius 20 (x16 uu), range 1000; thrown like the EMP.
constexpr int kPoisonNade = 2168;
constexpr int kPoisonPoint = 6;
constexpr float kPoisonNadeMax = 1000.0f, kPoisonNadeMin = 200.0f;  // close lobs like players
// Recon also carries the player's Fire Bomb (3056, lv4 offhand) on its free
// offhand point 6 (recon offhands: decoy 4, mine 5, fire bomb 6 = 3 like a player).
constexpr int kFireBomb = 3056;
// Recon loadout variety - Visual Scanner (2176) on point 6 instead of Fire Bomb.
// Data: group 6830 cat 773 Sensor Range 3000 (effect 8829 prop 13), 20 s.
constexpr int kScanner = 2176;
constexpr int kScanCategory = 773;
constexpr float kScanRange = 3000.0f;
constexpr int kFireBombPoint = 6;
constexpr int kMine = 2225;
constexpr int kMinePoint = 5;
constexpr int kMineDeployable = 81;
// Assault also carries the player's Range Shield (2013, lv3 offhand, slot 390)
// on equip point 5 (free on assault). Data: mode 2520 (TgDeviceFireOffhand, self),
// groups 5848 cat 770 'Personal Shield' (Protection - Ranged +100, 10 s) + 24043
// 'Shield Movement Penalty' (ground speed -10 %, 10 s); recharge prop 4 = 60 s, no power cost.
constexpr int kRangeShield = 2013;
constexpr int kShieldPoint = 5;
constexpr int kShieldCategory = 770;
// Assault loadout variety - half the assaults carry the player's Perfect
// Target (3708, lv5 offhand, slot 390) on point 5 instead of Range Shield. Data:
// mode 3830 (target self), groups 10858 cat 862 'Invulnerable' (protections +
// taunt, 10 s) + 24042 (GroundSpeed -25 %, 10 s); recharge prop 4 = 60 s.
constexpr int kPerfectTarget = 3708;
constexpr int kPerfectCategory = 862;
// Medic loadout variety - half the medics carry the player's Regeneration
// (2246, lv3 offhand, slot 390) on point 5 instead of Neutralize Wave. Data: mode
// 2758 (target self), groups 7026 cat 1341 (+115 health every 1 s, 15 s) + 13695
// (cat 1589, 15 s); recharge prop 4 = 60 s, no power cost.
constexpr int kRegen = 2246;
constexpr int kRegenCategory = 1341;
constexpr int kRepairArm = 2918;
constexpr int kRepairPoint = 4;

// Look key for the next Dress (respawn slot; 0 = fresh temp look).
int g_dressKey = 0;
// Bot skill 0..1 for the next Dress (team-bot add sets it; dev bots 0.5).
constexpr float kDefaultSkill = 0.5f;
float g_dressSkill = kDefaultSkill;

const ClassKit* FindKit(const std::string& cls) {
    for (const auto& k : kKits) {
        if (cls == k.name) return &k;
    }
    return nullptr;
}

void SetName(ATgPawn* pawn, const wchar_t* name) {
    if (!pawn->PlayerReplicationInfo) return;
    // p0997f: eventSetPlayerName copies the string engine-side and leaves this buffer to us.
    FString s;
    const int len = (int)wcslen(name) + 1;
    s.Data = (wchar_t*)GAllocator::Malloc(len * sizeof(wchar_t));
    s.Count = s.Max = len;
    wcscpy(s.Data, name);
    pawn->PlayerReplicationInfo->eventSetPlayerName(s);
    GAllocator::Free(s.Data);
    s.Data = nullptr; s.Count = s.Max = 0;
}

ATgAIController* AiOf(ATgPawn* pawn) {
    if (!pawn->Controller ||
        !ObjectClassCache::ClassNameContains(pawn->Controller, "TgAIController")) return nullptr;
    return (ATgAIController*)pawn->Controller;
}

void SwapBehavior(ATgPawn* pawn, int donorBotId) {
    ATgAIController* aic = AiOf(pawn);
    if (!aic) {
        Logger::Log("chat-command", "[PvpBot] no TgAIController - behavior not swapped\n");
        return;
    }
    auto it = CAmBot__LoadBotMarshal::m_BotPointers.find((uint32_t)donorBotId);
    if (it == CAmBot__LoadBotMarshal::m_BotPointers.end() || it->second == 0) {
        Logger::Log("chat-command", "[PvpBot] donor bot %d not marshalled - skipped\n", donorBotId);
        return;
    }
    // Experiment: behavior with the map's fallback factory (dev bots
    // otherwise have none) - does the action loop need one?
    ActorCache::CacheMapActors();
    TgAIController__InitBehavior::CallOriginal(aic, nullptr,
        (void*)(intptr_t)it->second, ActorCache::BotFactory);
}

// Dev-spawned bots never raise their own sight event on the requester, so
// the behavior idles. Tick() feeds the foe through the game's SeePlayer event
// (same call EnemyState restore uses) once the spawn frame is over, and logs
// position + Enemy for a few seconds so a run shows whether the bot acts.
struct Pending {
    ATgAIController* ctrl;
    int              pawnId;
    APawn*           foe;
    float            next;   // seconds until the next check
    int              checks; // remaining checks
};
std::vector<Pending> g_pending;

// Dressed bots with a foe, for the chase drive (OverrideMovement). Entries
// drop when the bot or foe dies / is replaced.
struct Chaser {
    int    pawnId;
    APawn* foe;
    bool   chasing; // last decision (logged on change)
    int    rewrites; // SetWhatToDoNext picks rewritten (logged on change)
    float  diagClock = 0.0f; // periodic "[PvpBot] sight" line
    bool   viaPath = false;           // last pick ran to a waypoint
    AActor* lastWaypoint = nullptr;   // logged on change
    AController* foeCtrl = nullptr;   // foe's controller: follow it across respawns
    int    foePawnId = 0;             // r_nPawnId of c.foe (a respawn can reuse the address)
    ANavigationPoint* foeNav = nullptr; // Our own anchor for the foe
    FVector foeNavAt = {0, 0, 0};       // foe location when foeNav was picked
    bool   noRoute = false;             // logged once per no-route spell
    std::vector<ANavigationPoint*> route; // Our own path-net route
    size_t routeIdx = 0;
    ANavigationPoint* routeGoal = nullptr;
    float  routeAt = -100.0f;           // WorldInfo time of the last plan
    bool   jetting = false;             // Jetpack thrust toward lastWaypoint
    float  fuel = 4.0f;                 // seconds of thrust left
    float  jetFor = 0.0f;               // seconds in the current jet
    int    jets = 0;
    bool   lastSight = false;           // Last WantsChase sight result
    float  stuckClock = 0.0f;           // Time pressing at a close foe without sight
    FVector stuckRef = {0, 0, 0};       // bot location when stuckClock started
    float  blockedFor = -1.0f;          // >=0: holding instead of pressing (foe unreachable)
    bool   hasSightSpot = false;        // Where the bot last saw its foe
    FVector sightSpot = {0, 0, 0};
    bool   backoff = false;             // blocked: running back to sightSpot
    int    fireLos = 0;                 // TargetInLOS overrides (logged every 20)
    float  pickAt = -100.0f;            // WorldInfo time of the last foe pick
    bool   medic = false;               // Heals hurt teammates with equip point 3
    APawn* patient = nullptr;           // Teammate being healed (null = fighting)
    int    patientId = 0;
    int    patientHp0 = 0;              // patient HP when the heal started (log)
    bool   healRun = false;             // running to the patient (OverrideMovement)
    int    beams = 0, beamFail = 0;     // UseDevice results (logged every 10)
    bool   robotics = false;            // Throws its turret (equip point 3) at a seen foe
    float  turretAt = -100.0f;          // WorldInfo time of the last placement try
    float  turretCool = 0.0f;           // Wait after turretAt (data recharge after a placement)
    int    turretThrows = 0;
    APawn* turret = nullptr;            // last placed turret pet
    int    turretId = 0;
    bool   recon = false;               // Cloaks (equip point 3 Sprint Stealth) while closing in
    bool   cloaked = false;             // stealth device in hand + started
    float  cloakAt = -100.0f;           // WorldInfo time cloak started / last ended
    float  cloakWaitLog = -100.0f;      // Last "cloak wait" log
    float  cloakDrainAt = -100.0f;      // Last power drain tick
    float  cloakPowerUsed = 0.0f;       // Power spent this cloak (log)
    float  cloakPool = 0.0f;            // Pool after the last drain (no regen while the device is held)
    bool   cloakEscape = false;         // Cloak taken while retreating (low HP)
    float  tellUntil = 0.0f;            // Uncloaked, facing the foe, holding fire until this time
    bool   assault = false;             // Throws its EMP grenade (equip point 3)
    float  nadeAt = -100.0f;            // Last throw (or failed try)
    float  nadeCool = 0.0f;             // Wait after nadeAt
    float  nadeWind = 0.0f;             // wind-up ends (0 = none)
    APawn* nadeFoe = nullptr;           // wind-up target
    int    nadeThrows = 0;
    float  nadeDiagAt = -100.0f;
    const char* nadeSkip = "-";
    const char* nadeWhy = "";
    float  slamAt = -100.0f;            // Last concussion throw (or failed try)
    float  slamCool = 0.0f;             // Wait after slamAt
    float  slamWind = 0.0f;             // Hop + wind-up ends (0 = none)
    APawn* slamFoe = nullptr;           // wind-up target
    bool   slamJet = false;             // The current jet is the slam hop
    float  slamZ0 = 0.0f;               // Feet Z at hop start
    int    slamN = 0;
    float  slamDiagAt = -100.0f;
    const char* slamSkip = "-";
    const char* slamWhy = "";
    float  waveAt = -100.0f;            // Last Healing Wave (or failed try)
    float  waveCool = 0.0f;
    float  waveWind = 0.0f;             // Tell ends (0 = none)
    float  waveDiagAt = -100.0f;
    int    waveN = 0;
    const char* waveSkip = "-";
    const char* waveWhy = "";
    float  neutAt = -100.0f;            // Last Neutralize Wave (or failed try)
    float  neutCool = 0.0f;
    float  neutWind = 0.0f;             // Tell ends (0 = none)
    float  neutDiagAt = -100.0f;
    int    neutN = 0;
    const char* neutSkip = "-";
    const char* neutWhy = "";
    float  petLogAt = -100.0f;          // Last "ab pets focus" line
    float  hiddenSince = -1.0f;         // Current foe cloaked + unseen since (WorldInfo time), -1 = seen
    float  cloakLostAt = -100.0f;       // Last time a cloaked foe was lost
    float  sensorAt = -100.0f, sensorCool = 0.0f, sensorWind = 0.0f, sensorDiagAt = -100.0f;
    int    sensorN = 0;
    float  mineAt = -100.0f, mineCool = 0.0f, mineWind = 0.0f, mineDiagAt = -100.0f, mineObjAt = -100.0f;
    int    mineN = 0;
    bool   poisonOn = false, poisonWas = false;  // Stepping out of poison / poisoned last tick
    float  poisonAt = -100.0f, poisonEndAt = -100.0f;
    FVector poisonFrom = {0, 0, 0}, poisonTo = {0, 0, 0};
    int    poisonN = 0;
    const char* mineSkip = "-";
    const char* mineWhy = "";
    const char* sensorSkip = "-";
    const char* sensorWhy = "";
    float  decoyAt = -100.0f;           // Last Decoy (or failed try)
    float  decoyCool = 0.0f;
    float  decoyWind = 0.0f;            // Tell ends (0 = none)
    float  decoyDiagAt = -100.0f;
    int    decoyN = 0;
    const char* decoySkip = "-";
    const char* decoyWhy = "";
    float  wallAt = -100.0f;            // Last Force Wall (or failed try)
    float  wallCool = 0.0f;
    float  wallWind = 0.0f;             // Tell ends (0 = none)
    ATgDeployable* cargoPack = nullptr; // Pack we run to
    FVector cargoAt = {0, 0, 0};
    int    cargoDep = 0;                // Carried deployable id (0 = none)
    float  cargoLookAt = -100.0f, cargoSince = 0.0f, cargoWind = 0.0f;
    int    cargoN = 0;
    float  cargoRunAt = 0.0f;
    float  wallDiagAt = -100.0f;
    int    wallN = 0;
    const char* wallSkip = "-";
    const char* wallWhy = "";
    float  shieldAt = -100.0f, shieldCool = 0.0f, shieldDiagAt = -100.0f;  // Range Shield
    int    shieldHp = 0, shieldN = 0;
    const char* shieldSkip = "-";
    bool   ptarget = false;             // Assault carries Perfect Target (point 5) instead of Range Shield
    bool   medst = false;               // Robotics carries Medical Station (point 5) instead of Force Wall
    bool   eyedrone = false;            // Robotics carries Eye Drone (point 6) instead of Sensor
    bool   grizzly = false;             // Robotics carries Grizzly Drone (point 3) instead of the turret
    float  medHold = 0.0f;              // Stay on the station until (heal hold)
    int    medHp0 = 0;                  // HP when the station went down
    FVector medSpot = {0, 0, 0};        // Where the station is
    bool   regen = false;               // Medic carries Regeneration (point 5) instead of Neutralize Wave
    float  regenAt = -100.0f, regenCool = 0.0f, regenDiagAt = -100.0f;
    int    regenN = 0;
    const char* regenSkip = "-";
    bool   ffOn = false;                // Walking around an enemy force field
    float  ffAt = -100.0f, ffEndAt = -100.0f, ffBlockAt = -100.0f, ffHoldLogAt = -100.0f;
    FVector ffTo = {0, 0, 0}, ffFrom = {0, 0, 0};
    int    ffHoldN = 0, ffN = 0;
    int    cloaks = 0, cloakFail = 0;
    float  skill = -1.0f;               // 0..1 fire pacing (<0 = off)
    float  sightSince = -100.0f;        // WorldInfo time the foe came into view (reaction delay)
    float  lastSightAt = -100.0f;       // last fire-check time with the foe in view
    float  burstAt = -100.0f;           // start of the current fire/pause cycle
    int    gated = 0;                   // fire checks held back (logged every 40)
    bool   retreat = false;             // Low HP - falling back to a medic / own start
    APawn* retreatMedic = nullptr;      // teammate medic bot we run to (null = home start)
    FVector retreatTo = {0, 0, 0};      // home start (when no medic)
    float  retreatAt = -100.0f;         // WorldInfo time the retreat started / ended
    std::vector<ANavigationPoint*> avoid; // Nodes this bot failed to reach (planner skips them)
    AActor* jetTo = nullptr;            // Target of the last failed jet
    int    jetMiss = 0;                 // Failed jets in a row toward jetTo
    float  progBest = 1.0e18f;          // Best goal distance this objective
    bool   noDirect = false;            // Straight run stuck -> follow the route even if 'reachable'
    std::vector<ANavigationPoint*> jumpTo; // Route nodes reached over a jump link (jet across a net gap)
    bool   jetGlide = false;            // Current jet follows a jump link (may go down)
    float  progClock = 0.0f;            // Seconds without 200 uu of progress
    float  stillFor = 0.0f;             // Seconds standing still while it should run
    int    nudges = 0;                  // Unstick hops (log)
    int    moveCalls = 0;               // OverrideMovement objective calls (diag)
    FVector hopFrom = {0, 0, 0};        // Where the last unstick hop started
    int    warps = 0;
    bool   coverOn = false;             // Moving to / hiding at a cover spot
    bool   coverHide = false;           // Arrived, hiding (peek when the timer runs out)
    FVector coverTo = {0, 0, 0};
    FVector coverFoeAt = {0, 0, 0};       // Foe spot at cover start (diag)
    float  coverUntil = -100.0f;        // Phase deadline (move timeout / hide end)
    float  coverAt = -100.0f;           // Last cover start
    float  coverEndAt = -100.0f;        // Last cover end (cooldown)
    float  lastHitAt = -100.0f;         // Last 0.5 s tick where HP went down
    int    covers = 0;
    APawn* coverFoe = nullptr;
    float  chaseStill = 0.0f;           // Seconds standing still while chasing
    int    chaseNudges = 0;             // chase-mode unstick hops (log)
    float  chaseTickAt = -100.0f;       // Last chase watchdog tick
    int    retreatHp0 = 0;
    float  homeFor = 0.0f;              // seconds spent at the home start
    int    retreats = 0;
    ATgMissionObjective* objective = nullptr;  // Capture point / payload we play
    bool   objOn = false;               // inside its hold radius
    float  objClock = 0.0f;             // status log clock
    bool   rally = false;               // Waiting near the point for teammates
    bool   rallyDone = false;           // rally already ran for this objective
    float  rallyFor = 0.0f;             // seconds waited
    bool   strafeOn = false;            // side-stepping while holding range with sight
    int    strafeSide = 1;              // +1 / -1 (left / right of the foe line)
    FVector strafeTo = {0, 0, 0};
    float  strafeUntil = -100.0f;       // WorldInfo time the current step / pause ends
    int    strafes = 0, strafeBlocked = 0;
    int    hpPrev = 0;                  // HP at the last 0.5 s tick (dodge on hit)
    float  hpAt = -100.0f;
    float  dodgeAt = -100.0f;
    int    dodges = 0;
    bool   escapeJet = false;           // Retreating bot jets up + away from a close foe
    FVector escapeDir = {0, 0, 0};
    float  escapeAt = -100.0f;
    int    escapes = 0;
    float  escDiagAt = -100.0f;         // Last 'escape skip' log
    bool   meleeOn = false;             // Foe close - melee device (point 1) wanted
    float  weaponAt = -100.0f;          // Last weapon change decision
    float  swFailSince = -100.0f;       // First refused switch of the current try
    int    swaps = 0, swFail = 0, swings = 0;
    bool   sniper = false;              // Recon with a sniper rifle - holds long range, scopes in
    bool   scanner = false;             // Recon carries Visual Scanner (point 6) instead of Fire Bomb
    float  scanAt = -100.0f, scanCool = 0.0f, scanDiagAt = -100.0f;
    int    scanN = 0;
    const char* scanSkip = "-";
    bool   scoped = false;              // r_bIsInSnipeScope set by us
    int    scopes = 0, scopeDiag = 0;
    ANavigationPoint* flankNode = nullptr;  // Side node the bot routes through toward a far unseen foe
    APawn* flankFoe = nullptr;
    float  flankAt = -100.0f, flankEndAt = -100.0f, flankTryAt = -100.0f;
    float  flankLastAt = -100.0f;       // last FlankNext call while active (left chase -> ended in Tick)
    int    flanks = 0;
};
std::map<ATgAIController*, Chaser> g_chasers;

// Chase hysteresis (UU): run at the foe beyond kChaseFar or without line of
// sight; hand movement back to the behavior (stand-and-fire) inside kChaseNear
// with sight.
constexpr float kChaseFar  = 1500.0f;
constexpr float kChaseNear = 900.0f;
// Sniper recon holds far back (Ballista = recon lv7 sniper rifle,
// 'only fires while in scope' -> r_bIsInSnipeScope while holding with sight).
constexpr float kSnipeFar  = 3800.0f;
constexpr float kSnipeNear = 2400.0f;
constexpr int   kSniperRifle = 2110;

bool PawnAlive(APawn* p, APawn* any) {
    if (!p || !any || !any->WorldInfo) return false;
    for (APawn* q = any->WorldInfo->PawnList; q; q = q->NextPawn) {
        if (q == p) return !q->bDeleteMe && q->Health > 0;
    }
    return false;
}

// A pawn's task force (players and bots alike), null when none.
ATgRepInfo_TaskForce* TfOf(APawn* p) {
    ATgRepInfo_Player* pri = p ? (ATgRepInfo_Player*)p->PlayerReplicationInfo : nullptr;
    ATgRepInfo_TaskForce* tf = pri ? pri->r_TaskForce : nullptr;
    return (tf == GTeamsData.Attackers || tf == GTeamsData.Defenders) ? tf : nullptr;
}

float Dist2(APawn* a, APawn* b) {
    const float dx = a->Location.X - b->Location.X;
    const float dy = a->Location.Y - b->Location.Y;
    const float dz = a->Location.Z - b->Location.Z;
    return dx * dx + dy * dy + dz * dz;
}

// Nearest live pawn on the other team (player or bot) - lets several
// bots per side fight each other instead of all chasing the requester.
// Target choice like a player - not purely the nearest: the distance
// is weighted down for an opponent that is hurt (<40 % HP x0.55, <70 % x0.8),
// a medic (player class 567 or a medic bot, x0.7) or standing on an active
// objective (within kTgtObjNear of its goal, x0.7). Lower score = better.
constexpr float kTgtObjNear = 700.0f;
constexpr int   kMedicProfile = 567;
bool PawnIsMedic(APawn* q) {
    if (q->Controller && ObjectClassCache::ClassNameContains(q->Controller, "PlayerController"))
        return q->IsA(ATgPawn_Character::StaticClass()) &&
               ((ATgPawn_Character*)q)->r_nProfileId == kMedicProfile;
    for (auto& kv : g_chasers)
        if (kv.first && kv.first->Pawn == q) return kv.second.medic;
    return false;
}
// An opponent pawn that is neither a player's agent, a tracked bot nor
// a Decoy (a Decoy looks like an agent and is treated as one) is a pet: a
// turret or drone. Pets are targets only within kPetFocus and only when no
// agent scores better (pet score = distance + kPetPenalty); an agent that
// gets within that wins at once.
// cs_settings pvpbot_ab_pets: 0 = pets scored like agents at any range (old),
// 1 = focus rule above (default).
constexpr float kPetFocus = 1500.0f;
constexpr float kPetDrop  = 1800.0f;  // current pet foe farther than this -> repick
constexpr float kPetPenalty = 600.0f; // added to a pet's score, so an agent up to ~600 uu farther still wins
bool PawnIsAgent(APawn* q) {
    if (q->Controller && ObjectClassCache::ClassNameContains(q->Controller, "PlayerController")) return true;
    if (((ATgPawn*)q)->r_bIsDecoy) return true;
    for (auto& kv : g_chasers)
        if (kv.first && kv.first->Pawn == q) return true;
    return false;
}
int PetsKnob(float now) {
    static int v = 1;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_pets'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 1;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 1) nv = 1;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] pets knob pvpbot_ab_pets=%d\n", nv);
        v = nv;
    }
    return v;
}
// A cloaked agent is hidden from bots like from players: stealthed, not
// revealed (sensor alert bit1, damage reveal window = r_nStealthDisabled /
// m_fMakeVisibleCurrent, see TgPawn__TickMakeVisibleCalculation) and farther
// than kCloakSee (the close-up shimmer). Hidden = not picked; a current foe that
// stays hidden kCloakForget s is dropped. Revealed cloakers are preferred (x0.7).
// cs_settings pvpbot_ab_reveal: 0 = bots see cloakers anywhere (old), 1 = fair (default).
constexpr float kCloakSee    = 350.0f;
constexpr float kCloakForget = 1.0f;
int RevealKnob(float now) {
    static int v = 1;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_reveal'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 1;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 1) nv = 1;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] reveal knob pvpbot_ab_reveal=%d\n", nv);
        v = nv;
    }
    return v;
}
bool HasCategory(ATgPawn* q, int cat);
bool CloakRevealed(APawn* q) {
    ATgPawn* t = (ATgPawn*)q;
    return (t->r_nSensorAlertLevel & 2) != 0 || t->r_nStealthDisabled > 0 || t->m_fMakeVisibleCurrent != 0.0f;
}
bool HiddenFrom(ATgPawn* bot, APawn* q) {
    if (!((ATgPawn*)q)->r_bIsStealthed || CloakRevealed(q)) return false;
    if (HasCategory(bot, kScanCategory) && Dist2((APawn*)bot, q) <= kScanRange * kScanRange) return false;  // Visual Scanner up
    return Dist2((APawn*)bot, q) > kCloakSee * kCloakSee;
}
float TargetScore(ATgPawn* bot, APawn* q, const std::vector<AActor*>& goals, const char** why) {
    float f = 1.0f;
    const char* w = "near";
    const int mx = ((ATgPawn*)q)->r_nHealthMaximum > 0 ? ((ATgPawn*)q)->r_nHealthMaximum : q->HealthMax;
    if (mx > 0) {
        const float r = (float)q->Health / (float)mx;
        if (r < 0.4f) { f *= 0.55f; w = "low hp"; }
        else if (r < 0.7f) { f *= 0.8f; w = "hurt"; }
    }
    if (PawnIsMedic(q)) { f *= 0.7f; w = "medic"; }
    if (((ATgPawn*)q)->r_bIsStealthed && CloakRevealed(q)) { f *= 0.7f; w = "revealed"; }
    for (AActor* g : goals) {
        const float dx = g->Location.X - q->Location.X, dy = g->Location.Y - q->Location.Y,
                    dz = g->Location.Z - q->Location.Z;
        if (dx * dx + dy * dy + dz * dz < kTgtObjNear * kTgtObjNear) { f *= 0.7f; w = "on objective"; break; }
    }
    if (why) *why = w;
    return std::sqrt(Dist2((APawn*)bot, q)) * f;
}
std::vector<AActor*> ActiveGoals(AWorldInfo* wi) {
    std::vector<AActor*> v;
    for (ATgMissionObjective* o : BotObjectives(wi, true))
        if (IsCapturable(o) && o->r_bIsActive && !o->r_bIsLocked) v.push_back(ObjectiveGoalActor(o));
    return v;
}
APawn* NearestOpponent(ATgPawn* bot, float* outScore, const char** outWhy = nullptr,
                       const std::vector<AActor*>* goalsIn = nullptr) {
    ATgRepInfo_TaskForce* mine = TfOf((APawn*)bot);
    APawn* best = nullptr;
    float bestS = 0.0f;
    const char* bestWhy = "near";
    if (mine && bot->WorldInfo) {
        std::vector<AActor*> goals = goalsIn ? *goalsIn : ActiveGoals(bot->WorldInfo);
        for (APawn* q = bot->WorldInfo->PawnList; q; q = q->NextPawn) {
            if (q == (APawn*)bot || q->bDeleteMe || q->Health <= 0) continue;
            ATgRepInfo_TaskForce* tf = TfOf(q);
            if (!tf || tf == mine) continue;
            const char* w = "near";
            if (RevealKnob(bot->WorldInfo->TimeSeconds) && HiddenFrom(bot, q)) continue;
            const bool pet = PetsKnob(bot->WorldInfo->TimeSeconds) && !PawnIsAgent(q);
            if (pet && Dist2((APawn*)bot, q) > kPetFocus * kPetFocus) continue;
            const float sc = TargetScore(bot, q, goals, &w);
            if (pet) w = "pet";
            const float sp = pet ? sc + kPetPenalty : sc;  // Agents within ~600 uu more win
            if (!best || sp < bestS) { best = q; bestS = sp; bestWhy = w; }
        }
    }
    if (outScore) *outScore = bestS;
    if (outWhy) *outWhy = bestWhy;
    return best;
}

void SetFoe(ATgAIController* aic, Chaser& c, APawn* foe, const char* why) {
    c.foe = foe;
    c.foePawnId = ((ATgPawn*)foe)->r_nPawnId;
    c.foeCtrl = foe->Controller;
    c.chasing = false;
    c.viaPath = false;
    c.lastWaypoint = nullptr;
    c.foeNav = nullptr;
    c.route.clear();
    c.routeIdx = 0;
    c.routeGoal = nullptr;
    c.blockedFor = -1.0f;
    c.stuckClock = 0.0f;
    c.hasSightSpot = false;
    c.backoff = false;
    aic->SetTargetActor((AActor*)foe);
    aic->AddThreat((ATgPawn*)foe, 1000.0f);
    aic->Enemy = foe;
    aic->m_bInterrupt = 1;
    Logger::Log("chat-command", "[PvpBot] target pawn=%d foe=%d (%s) hp=%d why=%s\n",
        c.pawnId, c.foePawnId, foe->Controller &&
        ObjectClassCache::ClassNameContains(foe->Controller, "PlayerController") ? "player" : "bot",
        foe->Health, why);
}

// Keep a live foe; when it dies or is gone pick the nearest
// opponent (the old foe's respawn too, if it is the nearest). Every 3 s
// switch to an opponent clearly closer (< 0.6x the distance).
// Returns false when there is no live foe right now (skip this round).
constexpr float kRepickEvery = 3.0f;
constexpr float kRepickRatio = 0.6f;  // Score ratio (was distance^2 0.36)
bool RefreshFoe(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    bool alive = c.foe && PawnAlive(c.foe, bot) &&
                       ((ATgPawn*)c.foe)->r_nPawnId == c.foePawnId;
    const float now = bot->WorldInfo ? bot->WorldInfo->TimeSeconds : 0.0f;
    // A pet foe is re-checked every round (an agent may have come closer)
    // Foe cloaked and unseen -> keep chasing its last spot kCloakForget s, then drop it
    if (alive && RevealKnob(now) && HiddenFrom(bot, c.foe)) {
        if (c.hiddenSince < 0.0f) {
            c.hiddenSince = now;
            Logger::Log("chat-command", "[PvpBot] ab reveal hidden pawn=%d foe=%d dist=%.0f alert=%d\n",
                c.pawnId, c.foePawnId, std::sqrt(Dist2((APawn*)bot, c.foe)), ((ATgPawn*)c.foe)->r_nSensorAlertLevel);
        }
        if (now - c.hiddenSince >= kCloakForget) {
            Logger::Log("chat-command", "[PvpBot] ab reveal lost pawn=%d foe=%d dist=%.0f after=%.1f\n",
                c.pawnId, c.foePawnId, std::sqrt(Dist2((APawn*)bot, c.foe)), now - c.hiddenSince);
            if (aic->Enemy == c.foe) aic->Enemy = nullptr;
            if (aic->Focus == (AActor*)c.foe) aic->Focus = nullptr;
            c.foe = nullptr; c.foePawnId = 0; c.foeCtrl = nullptr;
            c.chasing = false; c.hiddenSince = -1.0f; c.cloakLostAt = now;
            alive = false;
        }
    } else if (c.hiddenSince >= 0.0f) {
        if (alive) Logger::Log("chat-command", "[PvpBot] ab reveal seen pawn=%d foe=%d dist=%.0f revealed=%d after=%.1f\n",
            c.pawnId, c.foePawnId, std::sqrt(Dist2((APawn*)bot, c.foe)), (int)CloakRevealed(c.foe), now - c.hiddenSince);
        c.hiddenSince = -1.0f;
    }
    const bool petFoe = alive && PetsKnob(now) && !PawnIsAgent(c.foe);
    if (alive && !petFoe && now - c.pickAt < kRepickEvery) return true;
    if (petFoe && now - c.petLogAt >= 2.0f) {
        c.petLogAt = now;
        Logger::Log("chat-command", "[PvpBot] ab pets focus pawn=%d pet=%d hp=%d/%d dist=%.0f bot=(%.0f,%.0f,%.0f)\n",
            c.pawnId, c.foePawnId, c.foe->Health, ((ATgPawn*)c.foe)->r_nHealthMaximum > 0 ? ((ATgPawn*)c.foe)->r_nHealthMaximum : c.foe->HealthMax, std::sqrt(Dist2((APawn*)bot, c.foe)),
            bot->Location.X, bot->Location.Y, bot->Location.Z);
    }
    if (petFoe && Dist2((APawn*)bot, c.foe) > kPetDrop * kPetDrop) alive = false;  // walked away from it
    if (petFoe && now - c.pickAt < 0.5f) return true;
    c.pickAt = now;
    float sc = 0.0f;
    const char* why = "near";
    std::vector<AActor*> goals = ActiveGoals(bot->WorldInfo);
    APawn* cand = NearestOpponent(bot, &sc, &why, &goals);
    if (alive) {
        // Switch when the new pick scores clearly better (< 0.6x)
        if (petFoe && cand && cand != c.foe && PawnIsAgent(cand) &&
            sc < TargetScore(bot, c.foe, goals, nullptr) + kPetPenalty) { SetFoe(aic, c, cand, "agent closer"); return true; }
        if (cand && cand != c.foe && sc < kRepickRatio * TargetScore(bot, c.foe, goals, nullptr))
            SetFoe(aic, c, cand, why == std::string("near") ? "closer" : why);
        return true;
    }
    if (!cand) return false;
    SetFoe(aic, c, cand, (std::string(c.foe ? "foe down" : "first") + "/" + why).c_str());
    return true;
}

// Decide chase vs hold for one tracked bot; updates c.chasing.
// Medic role: weakest live teammate (players + bots of the same task
// force, not self) under kHealBelow of max HP within kHealSearch.
constexpr float kHealBelow  = 0.75f;
constexpr float kHealSearch = 2500.0f;
constexpr float kHealRange  = 550.0f;   // beam range we work from (uu)
constexpr float kHealFoeNear = 600.0f;  // a foe this close wins unless the patient is critical
APawn* WeakestTeammate(ATgPawn* bot, float* outRatio) {
    ATgRepInfo_TaskForce* mine = TfOf((APawn*)bot);
    APawn* best = nullptr;
    float bestR = kHealBelow;
    if (mine && bot->WorldInfo) {
        for (APawn* q = bot->WorldInfo->PawnList; q; q = q->NextPawn) {
            if (q == (APawn*)bot || q->bDeleteMe || q->Health <= 0) continue;
            if (TfOf(q) != mine) continue;
            const int mx = ((ATgPawn*)q)->r_nHealthMaximum > 0 ? ((ATgPawn*)q)->r_nHealthMaximum : q->HealthMax;
            if (mx <= 0) continue;
            const float r = (float)q->Health / (float)mx;
            if (r >= bestR || Dist2((APawn*)bot, q) > kHealSearch * kHealSearch) continue;
            best = q; bestR = r;
        }
    }
    if (outRatio) *outRatio = bestR;
    return best;
}

void EndHeal(ATgAIController* aic, Chaser& c, const char* why) {
    if (!c.patient) return;
    const bool up = PawnAlive(c.patient, aic->Pawn) && ((ATgPawn*)c.patient)->r_nPawnId == c.patientId;
    // hp -1 = patient gone/dead (pawn list) 
    Logger::Log("chat-command", "[PvpBot] heal end pawn=%d patient=%d hp %d -> %d beams=%d fail=%d why=%s\n",
        c.pawnId, c.patientId, c.patientHp0, up ? c.patient->Health : -1, c.beams, c.beamFail, why);
    c.patient = nullptr;
    c.healRun = false;
    aic->m_pFriendToHelp = nullptr;
    if (APawn* me = aic->Pawn) {  // Beam off
        ATgDevice* w = (ATgDevice*)me->Weapon;
        if (w) { ((ATgPawn*)me)->StopAction(w, 0); ((ATgPawn*)me)->StopAction(w, 1); }
    }
    aic->StopDevices();
    if (c.foe && PawnAlive(c.foe, aic->Pawn)) aic->Enemy = c.foe;
    if (c.foe && PawnAlive(c.foe, aic->Pawn)) {
        aic->SetTargetActor((AActor*)c.foe);
        aic->Focus = (AActor*)c.foe;
    }
    aic->m_bInterrupt = 1;
}

bool TurretAlive(Chaser& c, APawn* any);

// Robotics patient = its own live turret under kHealBelow.
APawn* HurtTurret(ATgPawn* bot, Chaser& c, float* outRatio) {
    if (outRatio) *outRatio = 1.0f;
    if (!TurretAlive(c, (APawn*)bot)) return nullptr;
    const int mx = ((ATgPawn*)c.turret)->r_nHealthMaximum > 0 ? ((ATgPawn*)c.turret)->r_nHealthMaximum : c.turret->HealthMax;
    if (mx <= 0) return nullptr;
    const float r = (float)c.turret->Health / (float)mx;
    if (outRatio) *outRatio = r;
    return r < kHealBelow ? c.turret : nullptr;
}

// Returns true while the medic is busy healing (caller skips the chase).
// Robotics runs the same loop with the repair arm on its own turret.
bool TickHeal(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    const bool rep = c.robotics && bot->m_EquippedDevices[kRepairPoint];
    if (!c.medic && !rep) return false;
    float ratio = 1.0f;
    APawn* p = rep ? HurtTurret(bot, c, &ratio) : WeakestTeammate(bot, &ratio);
    const bool foeNear = c.foe && PawnAlive(c.foe, bot) &&
        Dist2((APawn*)bot, c.foe) < kHealFoeNear * kHealFoeNear;
    if (!p || (foeNear && ratio > 0.35f)) {
        if (c.patient) EndHeal(aic, c, p ? "foe close" : "all healthy");  // TickWeapon puts the gun back
        return false;
    }
    if (p != c.patient) {
        if (c.patient) EndHeal(aic, c, "weaker teammate");
        c.patient = p;
        c.patientId = ((ATgPawn*)p)->r_nPawnId;
        c.patientHp0 = p->Health;
        Logger::Log("chat-command", "[PvpBot] heal start pawn=%d patient=%d (%s) hp=%d ratio=%.2f dist=%.0f\n",
            c.pawnId, c.patientId, rep ? "turret" : p->Controller &&
            ObjectClassCache::ClassNameContains(p->Controller, "PlayerController") ? "player" : "bot",
            p->Health, ratio, std::sqrt(Dist2((APawn*)bot, p)));
    }
    aic->m_pFriendToHelp = (ATgPawn*)p;
    aic->SetTargetActor((AActor*)p);
    aic->Focus = (AActor*)p;
    FVector eye = bot->Location;
    eye.Z += bot->BaseEyeHeight;
    const bool los = aic->LineOfSightTo((AActor*)p, eye, 0) || aic->LineOfSightTo((AActor*)p, eye, 0);
    const float d = std::sqrt(Dist2((APawn*)bot, p));
    // Range only: LineOfSightTo to a teammate in the spawn tunnel failed at
    // 170-500 uu (beams=0) - log los, don't gate on it.
    const bool run = d > kHealRange;
    if (run != c.healRun || c.beams + c.beamFail == 0)
        Logger::Log("chat-command", "[PvpBot] heal %s pawn=%d patient=%d dist=%.0f los=%d hp=%d\n",
            run ? "run" : "hold", c.pawnId, c.patientId, d, (int)los, p->Health);
    c.healRun = run;
    if (c.healRun) {
        if (bot->bIsWalking) bot->bIsWalking = 0;
        aic->eventSetWhatToDoNext(238, 289);  // OverrideMovement aims it at the patient
        return true;
    }
    // In range with sight: specialty (equip point 3) on the patient. Mode 0
    // first; the action rows say fire_mode_sequence 1 - try that if 0 refuses.
    ATgDevice* spec = bot->m_EquippedDevices[rep ? kRepairPoint : 3];
    if (c.beams + c.beamFail == 0) {  // once per heal: what sits where
        std::string pts;
        for (int i = 0; i < 8; i++) {
            ATgDevice* dv = bot->m_EquippedDevices[i];
            char buf[96];
            snprintf(buf, sizeof(buf), " %d=%s", i, dv ? dv->GetName() : "-");
            pts += buf;
        }
        Logger::Log("chat-command", "[PvpBot] heal devs pawn=%d%s ready3=%d\n",
            c.pawnId, pts.c_str(), (int)aic->IsDeviceReady(3));
    }
    // TgAIController.UseDevice refuses to switch while the hand
    // device IsFiring() (the donor behavior keeps the rifle firing at Enemy)
    // and returns false on the switch call - drive the pawn directly:
    // drop Enemy, stop the rifle, SetActiveWeapon(beam), then StartAction.
    bool ok = false;
    if (aic->Enemy) aic->Enemy = nullptr;
    ATgDevice* cur = (ATgDevice*)bot->Weapon;
    if (spec && cur != spec) {
        const int curFiring = cur ? (int)bot->IsFiring() : -1;
        if (cur) { bot->StopAction(cur, 0); bot->StopAction(cur, 1); }
        aic->StopDevices();
        const bool sw = bot->eventSetActiveWeapon((AWeapon*)spec, 0, 1);
        Logger::Log("chat-command", "[PvpBot] heal switch pawn=%d cur=%d@%d firing=%d spec=%d@%d sw=%d now=%s\n",
            c.pawnId, cur ? cur->r_nDeviceId : -1, cur ? (int)cur->r_eEquippedAt : -1, curFiring,
            spec->r_nDeviceId, (int)spec->r_eEquippedAt, (int)sw, bot->Weapon ? bot->Weapon->GetName() : "none");
    } else if (spec && !bot->IsFiring()) {
        ok = bot->StartAction(spec, 0);
        if (!ok) ok = bot->StartAction(spec, 1);
    } else if (spec) {
        ok = true;  // beam already on
    }
    if (ok) c.beams++; else c.beamFail++;
    if ((c.beams + c.beamFail) % 4 == 1) {
        const float now = aic->WorldInfo ? aic->WorldInfo->TimeSeconds : 0.0f;
        const std::string wpn = bot->Weapon ? bot->Weapon->GetName() : "none";
        Logger::Log("chat-command", "[PvpBot] heal beam pawn=%d patient=%d hp=%d/%d dist=%.0f los=%d ok=%d beams=%d fail=%d "
            "wpn=%s fired=%.1f friend=%d\n",
            c.pawnId, c.patientId, p->Health, ((ATgPawn*)p)->r_nHealthMaximum, d, (int)los, (int)ok, c.beams, c.beamFail,
            wpn.c_str(), now - aic->m_fLastFireTime, (int)(aic->m_pFriendToHelp == (ATgPawn*)p));
    }
    return true;
}

// Robotics role: with a seen foe in range and no live turret of its
// own, place the Personal Turret (equip point 3). GA turrets are PETS: the
// fire mode (asm device mode 2804) carries bot_id 1316 (setup +0x2C), no
// deployable_id - StartAction alone only opened the player deploy preview.
// So spawn it through the player turret path (TgDeviceFire__SpawnPet::SpawnAt)
// on the ground 250 uu toward the foe, facing it; the bot keeps fighting.
constexpr float kTurretRange = 2200.0f;
constexpr float kTurretRetry = 8.0f;    // s between attempts

// A device property straight from the fire mode's property list
// (the same rows the player's device uses: 4 recharge, 53 refire, 242 power
// cost). First fire mode that has it wins; 0 when missing.
float DeviceProp(ATgDevice* dev, int propId) {
    for (int m = 0; dev && m < dev->m_FireMode.Count; m++) {
        UTgDeviceFire* fm = dev->m_FireMode.Data[m];
        for (int i = 0; fm && i < fm->m_Properties.Count; i++) {
            UTgProperty* p = fm->m_Properties.Data[i];
            if (p && p->m_nPropertyId == propId) return p->m_fBase != 0.0f ? p->m_fBase : p->m_fRaw;
        }
    }
    return 0.0f;
}
constexpr int kPropRecharge = 4, kPropRefire = 53, kPropPowerCost = 242;

bool TurretAlive(Chaser& c, APawn* any) {
    return c.turret && PawnAlive(c.turret, any) && ((ATgPawn*)c.turret)->r_nPawnId == c.turretId;
}

// Medic Healing Wave: what a player's wave does - every effect group of
// the device's fire mode (heal 600 + the cleanse groups, each only when the
// target carries its required category) on every teammate inside the mode's
// radius, medic included. Trigger: >= 2 teammates (not self) under kWaveHurt
// in the radius (why=group) or, knob 2, one under kWaveCrit (why=critical).
// Tell: 0.6 s stop, no rifle/beam. Recharge from data prop 4 (60 s).
// cs_settings pvpbot_ab_wave: 0 = off, 1 = groups only, 2 = both (default).
constexpr float kWaveHurt = 0.70f, kWaveCrit = 0.35f, kWaveWind = 0.6f, kWaveRetry = 5.0f;
int WaveKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_wave'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] wave knob pvpbot_ab_wave=%d\n", nv);
        v = nv;
    }
    return v;
}

float WaveRadius(UTgDeviceFire* fm) {
    float r = fm ? fm->GetDamageRadius() : 0.0f;
    return r >= 100.0f ? r : 720.0f;  // data 45 x 16 uu
}

bool HasCategory(ATgPawn* q, int cat) {
    ATgEffectManager* mgr = q->r_EffectManager;
    for (int i = 0; mgr && i < mgr->s_AppliedEffectGroups.Count; i++) {
        UTgEffectGroup* g = mgr->s_AppliedEffectGroups.Data[i];
        if (g && g->m_nCategoryCode == cat) return true;
    }
    return false;
}

// Target carries a category one of the fire mode's groups requires
// (= the wave would remove something from it).
bool HasReqCategory(ATgPawn* q, UTgDeviceFire* fm) {
    for (int i = 0; fm && i < fm->s_EffectGroupList.Count; i++) {
        UTgEffectGroup* t = fm->s_EffectGroupList.Data[i];
        if (t && t->m_nReqCategoryCode && HasCategory(q, t->m_nReqCategoryCode)) return true;
    }
    return false;
}

bool TickWave(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!c.medic || !aic->WorldInfo) return false;
    ATgDevice* wave = bot->m_EquippedDevices[kWavePoint];
    const float now = aic->WorldInfo->TimeSeconds;
    UTgDeviceFire* fm = wave && wave->m_FireMode.Count > 0 ? wave->m_FireMode.Data[0] : nullptr;
    const float rad = WaveRadius(fm);
    if (now - c.waveDiagAt >= 10.0f) {
        c.waveDiagAt = now;
        Logger::Log("chat-command", "[PvpBot] ab wave state pawn=%d dev=%d radius=%.0f cool=%.1f/%.1f hp=%d skip=%s\n",
            c.pawnId, wave ? wave->r_nDeviceId : 0, rad, now - c.waveAt, c.waveCool, bot->Health, c.waveSkip);
    }
    if (!fm) return false;
    ATgRepInfo_TaskForce* mine = TfOf((APawn*)bot);
    if (!mine) { c.waveSkip = "notf"; return false; }
    if (c.waveWind > 0.0f) {  // tell running
        aic->Enemy = nullptr;
        bot->Acceleration = FVector(0, 0, 0);
        if (now < c.waveWind) return true;
        c.waveWind = 0.0f;
        c.waveAt = now; c.waveCool = kWaveRetry;
        if (fm->s_EffectGroupList.Count == 0) { int idx = 0; TgDeviceFire__GetEffectGroup::Call(fm, nullptr, -1, &idx); }
        int hit = 0, applied = 0, healed = 0;
        for (APawn* q = bot->WorldInfo->PawnList; q; q = q->NextPawn) {
            if (q->bDeleteMe || q->Health <= 0 || TfOf(q) != mine) continue;
            if (Dist2((APawn*)bot, q) > rad * rad) continue;
            ATgPawn* tq = (ATgPawn*)q;
            if (!tq->r_EffectManager) continue;
            const int hp0 = q->Health;
            hit++;
            for (int i = 0; i < fm->s_EffectGroupList.Count; i++) {
                UTgEffectGroup* t = fm->s_EffectGroupList.Data[i];
                if (!t || (t->m_nReqCategoryCode && !HasCategory(tq, t->m_nReqCategoryCode))) continue;
                UTgEffectGroup* g = BuildEffectGroup(t->m_nEffectGroupId, t->m_nType);
                if (!g) continue;
                FImpactInfo impact{};
                tq->r_EffectManager->eventProcessEffect(g, 0, nullptr, (AActor*)bot, impact);
                applied++;
            }
            healed += q->Health - hp0;
        }
        const float rc = DeviceProp(wave, kPropRecharge);
        if (applied > 0) { c.waveCool = rc > 0.0f ? rc : 60.0f; c.waveN++; }
        Logger::Log("chat-command", "[PvpBot] ab wave use pawn=%d why=%s ok=%d hit=%d groups=%d/%d healed=%d radius=%.0f n=%d recharge=%.1f\n",
            c.pawnId, c.waveWhy, (int)(applied > 0), hit, applied, fm->s_EffectGroupList.Count, healed, rad, c.waveN, c.waveCool);
        if (c.foe && PawnAlive(c.foe, (APawn*)bot)) aic->Enemy = c.foe;
        return false;
    }
    if (c.neutWind > 0.0f) { c.waveSkip = "neut"; return false; }  // One tell at a time
    if (c.cloaked) { c.waveSkip = "cloak"; return false; }
    if (now - c.waveAt < c.waveCool) { c.waveSkip = "cool"; return false; }
    const int knob = WaveKnob(now);
    if (knob == 0) { c.waveSkip = "knob0"; return false; }
    int hurt = 0, crit = 0, dirty = 0;
    if (fm->s_EffectGroupList.Count == 0) { int idx = 0; TgDeviceFire__GetEffectGroup::Call(fm, nullptr, -1, &idx); }
    for (APawn* q = bot->WorldInfo->PawnList; q; q = q->NextPawn) {
        if (q->bDeleteMe || q->Health <= 0 || TfOf(q) != mine) continue;
        if (Dist2((APawn*)bot, q) > rad * rad) continue;
        if (HasReqCategory((ATgPawn*)q, fm)) dirty++;  // Carries a debuff the wave removes (poison, ignite, slow, ...)
        if (q == (APawn*)bot) continue;
        const int mx = ((ATgPawn*)q)->r_nHealthMaximum > 0 ? ((ATgPawn*)q)->r_nHealthMaximum : q->HealthMax;
        if (mx <= 0) continue;
        const float r = (float)q->Health / (float)mx;
        if (r < kWaveHurt) hurt++;
        if (r < kWaveCrit) crit++;
    }
    const char* why = nullptr;
    if (hurt >= 2) why = "group";
    else if (knob >= 2 && crit >= 1) why = "critical";
    else if (knob >= 2 && dirty >= 1) why = "cleanse";
    if (!why) { c.waveSkip = "nowhy"; return false; }
    c.waveSkip = "-";
    c.waveWhy = why; c.waveWind = now + kWaveWind;
    ATgDevice* cur = (ATgDevice*)bot->Weapon;
    if (cur && bot->IsFiring()) { bot->StopAction(cur, 0); bot->StopAction(cur, 1); }
    aic->Enemy = nullptr;
    bot->Acceleration = FVector(0, 0, 0);
    Logger::Log("chat-command", "[PvpBot] ab wave windup pawn=%d why=%s hurt=%d crit=%d dirty=%d radius=%.0f knob=%d\n",
        c.pawnId, why, hurt, crit, dirty, rad, knob);
    return true;
}

// Medic Neutralize Wave: what a player's wave does - every effect group
// of the fire mode (-115 + each buff-strip group whose category the target
// carries) on every OPPONENT inside the radius. Trigger: an opponent in the
// radius carries a buff it strips (why=strip), >= 2 opponents in the radius
// (why=clump) or, knob 2, the chased foe in sight inside the radius
// (why=close). Tell 0.6 s stop, no rifle/beam. Recharge from data prop 4.
// cs_settings pvpbot_ab_neut: 0 = off, 1 = strip + clump, 2 = + close (default).
int NeutKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_neut'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] neut knob pvpbot_ab_neut=%d\n", nv);
        v = nv;
    }
    return v;
}

bool TickNeut(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!c.medic || c.regen || !aic->WorldInfo) return false;  // Regen medics carry no Neutralize Wave
    ATgDevice* wave = bot->m_EquippedDevices[kNeutPoint];
    const float now = aic->WorldInfo->TimeSeconds;
    UTgDeviceFire* fm = wave && wave->m_FireMode.Count > 0 ? wave->m_FireMode.Data[0] : nullptr;
    float rad = fm ? fm->GetDamageRadius() : 0.0f;
    if (rad < 100.0f) rad = 560.0f;  // data 35 x 16 uu
    if (now - c.neutDiagAt >= 10.0f) {
        c.neutDiagAt = now;
        Logger::Log("chat-command", "[PvpBot] ab neut state pawn=%d dev=%d radius=%.0f cool=%.1f/%.1f hp=%d skip=%s\n",
            c.pawnId, wave ? wave->r_nDeviceId : 0, rad, now - c.neutAt, c.neutCool, bot->Health, c.neutSkip);
    }
    if (!fm) return false;
    ATgRepInfo_TaskForce* mine = TfOf((APawn*)bot);
    if (!mine) { c.neutSkip = "notf"; return false; }
    if (fm->s_EffectGroupList.Count == 0) { int idx = 0; TgDeviceFire__GetEffectGroup::Call(fm, nullptr, -1, &idx); }
    if (c.neutWind > 0.0f) {  // tell running
        aic->Enemy = nullptr;
        bot->Acceleration = FVector(0, 0, 0);
        if (now < c.neutWind) return true;
        c.neutWind = 0.0f;
        c.neutAt = now; c.neutCool = kWaveRetry;
        int hit = 0, applied = 0, stripped = 0, dmg = 0;
        for (APawn* q = bot->WorldInfo->PawnList; q; q = q->NextPawn) {
            ATgRepInfo_TaskForce* tf = q->bDeleteMe || q->Health <= 0 ? nullptr : TfOf(q);
            if (!tf || tf == mine || Dist2((APawn*)bot, q) > rad * rad) continue;
            ATgPawn* tq = (ATgPawn*)q;
            if (!tq->r_EffectManager) continue;
            const int hp0 = q->Health;
            hit++;
            for (int i = 0; i < fm->s_EffectGroupList.Count; i++) {
                UTgEffectGroup* t = fm->s_EffectGroupList.Data[i];
                if (!t || (t->m_nReqCategoryCode && !HasCategory(tq, t->m_nReqCategoryCode))) continue;
                UTgEffectGroup* g = BuildEffectGroup(t->m_nEffectGroupId, t->m_nType);
                if (!g) continue;
                FImpactInfo impact{};
                tq->r_EffectManager->eventProcessEffect(g, 0, nullptr, (AActor*)bot, impact);
                applied++;
                if (t->m_nReqCategoryCode) stripped++;
            }
            dmg += hp0 - q->Health;
        }
        const float rc = DeviceProp(wave, kPropRecharge);
        if (applied > 0) { c.neutCool = rc > 0.0f ? rc : 60.0f; c.neutN++; }
        Logger::Log("chat-command", "[PvpBot] ab neut use pawn=%d why=%s ok=%d hit=%d groups=%d/%d stripped=%d dmg=%d radius=%.0f n=%d recharge=%.1f\n",
            c.pawnId, c.neutWhy, (int)(applied > 0), hit, applied, fm->s_EffectGroupList.Count, stripped, dmg, rad, c.neutN, c.neutCool);
        if (c.foe && PawnAlive(c.foe, (APawn*)bot)) aic->Enemy = c.foe;
        return false;
    }
    if (c.cloaked) { c.neutSkip = "cloak"; return false; }
    if (now - c.neutAt < c.neutCool) { c.neutSkip = "cool"; return false; }
    const int knob = NeutKnob(now);
    if (knob == 0) { c.neutSkip = "knob0"; return false; }
    int foes = 0, buffed = 0;
    for (APawn* q = bot->WorldInfo->PawnList; q; q = q->NextPawn) {
        ATgRepInfo_TaskForce* tf = q->bDeleteMe || q->Health <= 0 ? nullptr : TfOf(q);
        if (!tf || tf == mine || Dist2((APawn*)bot, q) > rad * rad) continue;
        foes++;
        if (HasReqCategory((ATgPawn*)q, fm)) buffed++;
    }
    const bool closeFoe = c.foe && c.lastSight && PawnAlive(c.foe, (APawn*)bot) && Dist2((APawn*)bot, c.foe) <= rad * rad;
    const char* why = nullptr;
    if (buffed >= 1) why = "strip";
    else if (foes >= 2) why = "clump";
    else if (knob >= 2 && closeFoe) why = "close";
    if (!why) { c.neutSkip = foes ? "nowhy" : "nofoe"; return false; }
    c.neutSkip = "-";
    c.neutWhy = why; c.neutWind = now + kWaveWind;
    ATgDevice* cur = (ATgDevice*)bot->Weapon;
    if (cur && bot->IsFiring()) { bot->StopAction(cur, 0); bot->StopAction(cur, 1); }
    aic->Enemy = nullptr;
    bot->Acceleration = FVector(0, 0, 0);
    Logger::Log("chat-command", "[PvpBot] ab neut windup pawn=%d why=%s foes=%d buffed=%d radius=%.0f knob=%d\n",
        c.pawnId, why, foes, buffed, rad, knob);
    return true;
}

// Robotics Force Wall, placed like a player: the device's deployable
// (fire-mode setup +0x28, data 76) through the same SpawnDeployableActor path
// the player's deploy uses, on the ground kWallDist toward the foe, spanning
// across the bot's view (the helper turns force fields 90 deg). Trigger: foe in
// sight 300-1800 uu and either the bot is playing an objective (why=objective)
// or, knob 2, it is hurt below 60 % (why=hurt; < 35 % = retreat runs first).
// Tell 0.5 s (stop, face the foe, hold fire). Recharge = data prop 4 (60 s),
// failed tries 8 s. cs_settings pvpbot_ab_wall: 0 off, 1 objective only,
// 2 objective + hurt (default).
constexpr float kWallWind = 0.5f, kWallMin = 300.0f, kWallMax = 1800.0f, kWallHurt = 0.60f, kWallRetry = 8.0f;
int WallKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_wall'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] wall knob pvpbot_ab_wall=%d\n", nv);
        v = nv;
    }
    return v;
}

// Robotics Sensor, placed like a player: deployable from the device's fire
// mode (setup +0x28, data 85) through SpawnDeployableActor, on the ground 80 uu in
// front of the bot. why=objective: on / within 700 uu of its objective and no foe
// in sight (guards the point against cloakers); why=cloak (knob 2): a cloaked foe
// was lost < 6 s ago -> drop it where it is. Tell 0.5 s. Recharge = data prop 4
// (40 s), failed tries 8 s. cs_settings pvpbot_ab_sensor: 0 off, 1 objective only,
// 2 objective + cloak (default).
constexpr float kSensorWind = 0.5f, kSensorNear = 700.0f, kSensorLost = 6.0f, kSensorRetry = 8.0f;
int SensorKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_sensor'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] sensor knob pvpbot_ab_sensor=%d\n", nv);
        v = nv;
    }
    return v;
}
// cs_settings pvpbot_ab_eyedrone: 0 off (every robotics keeps Sensor),
// 1 = half the robotics carry Eye Drone, objective only; 2 = + cloak (default).
// Class "robotics-eye" / "robotics-sensor" forces the variant.
int EyeKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_eyedrone'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] eyedrone knob pvpbot_ab_eyedrone=%d\n", nv);
        v = nv;
    }
    return v;
}
// Eye Drone through the player pet path, 100 uu in front of the bot.
ATgPawn* PlaceEyeDrone(ATgPawn* bot, ATgDevice* dev, int* petId, bool* ground, FVector* at) {
    UTgDeviceFire* fm = nullptr;
    *petId = 0;
    for (int i = 0; i < dev->m_FireMode.Num(); i++) {
        UTgDeviceFire* f = dev->m_FireMode(i);
        char* setup = f ? (char*)f->m_pFireModeSetup.Dummy : nullptr;
        if (setup && *(int*)(setup + 0x2C)) { fm = f; *petId = *(int*)(setup + 0x2C); break; }
    }
    if (!fm) return nullptr;
    const float yaw = bot->Rotation.Yaw * 3.14159265f / 32768.0f;
    FVector start = {bot->Location.X + std::cos(yaw) * 100.0f, bot->Location.Y + std::sin(yaw) * 100.0f, bot->Location.Z + 60.0f};
    FVector end = {start.X, start.Y, start.Z - 700.0f};
    FVector spot = {0, 0, 0}, n = {0, 0, 0};
    FTraceHitInfo hi;
    *ground = bot->Trace(end, start, 0, FVector(0, 0, 0), 0, &spot, &n, &hi) != nullptr;
    if (!*ground) {
        spot = bot->Location;
        if (bot->CylinderComponent) spot.Z -= bot->CylinderComponent->CollisionHeight;
    }
    float radius = 0.0f, halfHeight = 0.0f;
    TgGame__SpawnBotById::GetBotCollisionCylinder(*petId, &radius, &halfHeight);
    spot.Z += halfHeight + 5.0f;
    *at = spot;
    FRotator rot = {0, bot->Rotation.Yaw, 0};
    return TgDeviceFire__SpawnPet::SpawnAt(fm, bot, spot, rot);
}
// cs_settings pvpbot_ab_grizzly: 0 off (every robotics keeps the turret),
// 1 = half the robotics carry Grizzly Drone (default).
int GrizzlyKnob(float now) {
    static int v = 1;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_grizzly'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 1;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 1) nv = 1;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] grizzly knob pvpbot_ab_grizzly=%d\n", nv);
        v = nv;
    }
    return v;
}
bool TickSensor(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!c.robotics || !aic->WorldInfo) return false;
    ATgDevice* dev = bot->m_EquippedDevices[kSensorPoint];
    const float now = aic->WorldInfo->TimeSeconds;
    UTgDeviceFire* fm = dev && dev->m_FireMode.Count > 0 ? dev->m_FireMode.Data[0] : nullptr;
    int depId = 0;
    if (fm && fm->m_pFireModeSetup.Dummy) depId = *(int*)((char*)fm->m_pFireModeSetup.Dummy + 0x28);
    if (depId <= 0) depId = kSensorDeployable;
    const bool eyed = dev && dev->r_nDeviceId == kEyeDrone;
    if (now - c.sensorDiagAt >= 10.0f) {
        c.sensorDiagAt = now;
        Logger::Log("chat-command", "[PvpBot] ab sensor state pawn=%d dev=%d dep=%d cool=%.1f/%.1f obj=%d on=%d lost=%.1f skip=%s\n",
            c.pawnId, dev ? dev->r_nDeviceId : 0, depId, now - c.sensorAt, c.sensorCool,
            c.objective ? c.objective->nObjectiveId : -1, (int)c.objOn, now - c.cloakLostAt, c.sensorSkip);
    }
    if (!dev) return false;
    if (c.sensorWind > 0.0f) {  // tell running
        aic->Enemy = nullptr;
        bot->Acceleration = FVector(0, 0, 0);
        if (now < c.sensorWind) return true;
        c.sensorWind = 0.0f;
        c.sensorAt = now; c.sensorCool = kSensorRetry;
        if (eyed) {  // Eye Drone pet
            int petId = 0; bool gr = false; FVector at = {0, 0, 0};
            ATgPawn* pet = PlaceEyeDrone(bot, dev, &petId, &gr, &at);
            const float rc1 = DeviceProp(dev, kPropRecharge);
            if (pet) { c.sensorCool = rc1 > 0.0f ? rc1 : 45.0f; c.sensorN++; }
            Logger::Log("chat-command", "[PvpBot] ab eyedrone use pawn=%d why=%s ok=%d pet=%d id=%d ground=%d at=(%.0f,%.0f,%.0f) obj=%d hp=%d n=%d recharge=%.1f\n",
                c.pawnId, c.sensorWhy, (int)(pet != nullptr), petId, pet ? pet->r_nPawnId : -1, (int)gr, at.X, at.Y, at.Z,
                c.objective ? c.objective->nObjectiveId : -1, pet ? pet->Health : 0, c.sensorN, c.sensorCool);
            if (c.foe && PawnAlive(c.foe, (APawn*)bot)) aic->Enemy = c.foe;
            return false;
        }
        const float yaw = bot->Rotation.Yaw * 3.14159265f / 32768.0f;
        FVector start = {bot->Location.X + std::cos(yaw) * 80.0f, bot->Location.Y + std::sin(yaw) * 80.0f, bot->Location.Z + 60.0f};
        FVector end = {start.X, start.Y, start.Z - 700.0f};
        FVector spot = {0, 0, 0}, n = {0, 0, 1};
        FTraceHitInfo hi;
        const bool ground = bot->Trace(end, start, 0, FVector(0, 0, 0), 0, &spot, &n, &hi) != nullptr;
        if (!ground) {
            spot = bot->Location; n = FVector(0, 0, 1);
            if (bot->CylinderComponent) spot.Z -= bot->CylinderComponent->CollisionHeight;
        }
        ATgDeployable* d = TgProj_Deployable__SpawnDeployable::SpawnDeployableActor(bot, depId, spot, n, dev, fm);
        const float rc = DeviceProp(dev, kPropRecharge);
        if (d) { c.sensorCool = rc > 0.0f ? rc : 40.0f; c.sensorN++; }
        Logger::Log("chat-command", "[PvpBot] ab sensor use pawn=%d why=%s ok=%d dep=%d ground=%d at=(%.0f,%.0f,%.0f) obj=%d hp=%d n=%d recharge=%.1f\n",
            c.pawnId, c.sensorWhy, (int)(d != nullptr), depId, (int)ground, spot.X, spot.Y, spot.Z,
            c.objective ? c.objective->nObjectiveId : -1, d ? d->r_nHealth : 0, c.sensorN, c.sensorCool);
        if (c.foe && PawnAlive(c.foe, (APawn*)bot)) aic->Enemy = c.foe;
        return false;
    }
    if (c.cloaked) { c.sensorSkip = "cloak"; return false; }
    if (now - c.sensorAt < c.sensorCool) { c.sensorSkip = "cool"; return false; }
    const int knob = eyed ? EyeKnob(now) : SensorKnob(now);
    if (knob == 0) { c.sensorSkip = "knob0"; return false; }
    const bool foeSeen = c.foe && PawnAlive(c.foe, (APawn*)bot) && c.lastSight;
    const char* why = nullptr;
    if (knob >= 2 && now - c.cloakLostAt < kSensorLost) why = "cloak";
    else if (c.objective && !foeSeen) {
        AActor* g = ObjectiveGoalActor(c.objective);
        float gd2 = 1.0e18f;
        if (g) { const float gx = bot->Location.X - g->Location.X, gy = bot->Location.Y - g->Location.Y; gd2 = gx * gx + gy * gy; }
        if (c.objOn || gd2 < kSensorNear * kSensorNear) why = "objective";
    }
    if (!why) { c.sensorSkip = foeSeen ? "fight" : "nowhy"; return false; }
    c.sensorSkip = "-";
    c.sensorWhy = why; c.sensorWind = now + kSensorWind;
    ATgDevice* cur = (ATgDevice*)bot->Weapon;
    if (cur && bot->IsFiring()) { bot->StopAction(cur, 0); bot->StopAction(cur, 1); }
    aic->Enemy = nullptr;
    bot->Acceleration = FVector(0, 0, 0);
    Logger::Log("chat-command", eyed ? "[PvpBot] ab eyedrone windup pawn=%d why=%s obj=%d knob=%d\n" : "[PvpBot] ab sensor windup pawn=%d why=%s obj=%d knob=%d\n",
        c.pawnId, why, c.objective ? c.objective->nObjectiveId : -1, knob);
    return true;
}

// Recon Standard Mine, laid like a player's: the mine deployable (data 81,
// fire mode setup +0x28 when set) through SpawnDeployableActor on the ground at the
// bot's feet (escape/close) or 60 uu in front (objective). why=objective: on /
// within 700 uu of its objective, no foe in sight, at most one per 30 s there;
// why=close: a seen foe within 450 uu; why=escape (knob 2): retreating with a foe
// < 900 uu -> dropped behind without stopping. Tell 0.4 s (objective/close).
// Recharge = data prop 4 (10 s), failed tries 5 s. Never while cloaked.
// cs_settings pvpbot_ab_mine: 0 off, 1 objective only, 2 all (default).
constexpr float kMineWind = 0.4f, kMineNear = 700.0f, kMineClose = 450.0f, kMineEscape = 900.0f,
                kMineObjGap = 30.0f, kMineRetry = 5.0f;
int MineKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_mine'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] mine knob pvpbot_ab_mine=%d\n", nv);
        v = nv;
    }
    return v;
}
static void LayMine(ATgAIController* aic, ATgPawn* bot, Chaser& c, ATgDevice* dev, float ahead, float now) {
    UTgDeviceFire* fm = dev->m_FireMode.Count > 0 ? dev->m_FireMode.Data[0] : nullptr;
    int depId = 0;
    if (fm && fm->m_pFireModeSetup.Dummy) depId = *(int*)((char*)fm->m_pFireModeSetup.Dummy + 0x28);
    if (depId <= 0) depId = kMineDeployable;
    c.mineAt = now; c.mineCool = kMineRetry;
    const float yaw = bot->Rotation.Yaw * 3.14159265f / 32768.0f;
    FVector start = {bot->Location.X + std::cos(yaw) * ahead, bot->Location.Y + std::sin(yaw) * ahead, bot->Location.Z + 60.0f};
    FVector end = {start.X, start.Y, start.Z - 700.0f};
    FVector spot = {0, 0, 0}, n = {0, 0, 1};
    FTraceHitInfo hi;
    const bool ground = bot->Trace(end, start, 0, FVector(0, 0, 0), 0, &spot, &n, &hi) != nullptr;
    if (!ground) {
        spot = bot->Location; n = FVector(0, 0, 1);
        if (bot->CylinderComponent) spot.Z -= bot->CylinderComponent->CollisionHeight;
    }
    ATgDeployable* d = TgProj_Deployable__SpawnDeployable::SpawnDeployableActor(bot, depId, spot, n, dev, fm);
    const float rc = DeviceProp(dev, kPropRecharge);
    if (d) { c.mineCool = rc > 0.0f ? rc : 10.0f; c.mineN++; if (std::string(c.mineWhy) == "objective") c.mineObjAt = now; }
    Logger::Log("chat-command", "[PvpBot] ab mine use pawn=%d why=%s ok=%d dep=%d ground=%d at=(%.0f,%.0f,%.0f) obj=%d hp=%d n=%d recharge=%.1f\n",
        c.pawnId, c.mineWhy, (int)(d != nullptr), depId, (int)ground, spot.X, spot.Y, spot.Z,
        c.objective ? c.objective->nObjectiveId : -1, d ? d->r_nHealth : 0, c.mineN, c.mineCool);
}
bool TickMine(ATgAIController* aic, ATgPawn* bot, Chaser& c, bool escaping = false) {
    if (!c.recon || !aic->WorldInfo) return false;
    ATgDevice* dev = bot->m_EquippedDevices[kMinePoint];
    const float now = aic->WorldInfo->TimeSeconds;
    const bool foeAlive = c.foe && PawnAlive(c.foe, (APawn*)bot);
    float fd = 1.0e9f;
    if (foeAlive) {
        const float dx = bot->Location.X - c.foe->Location.X, dy = bot->Location.Y - c.foe->Location.Y, dz = bot->Location.Z - c.foe->Location.Z;
        fd = std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    if (now - c.mineDiagAt >= 10.0f) {
        c.mineDiagAt = now;
        Logger::Log("chat-command", "[PvpBot] ab mine state pawn=%d dev=%d cool=%.1f/%.1f obj=%d on=%d foe=%.0f esc=%d skip=%s\n",
            c.pawnId, dev ? dev->r_nDeviceId : 0, now - c.mineAt, c.mineCool,
            c.objective ? c.objective->nObjectiveId : -1, (int)c.objOn, foeAlive ? fd : -1.0f, (int)escaping, c.mineSkip);
    }
    if (!dev) return false;
    if (c.mineWind > 0.0f) {  // tell running
        aic->Enemy = nullptr;
        bot->Acceleration = FVector(0, 0, 0);
        if (now < c.mineWind) return true;
        c.mineWind = 0.0f;
        LayMine(aic, bot, c, dev, std::string(c.mineWhy) == "objective" ? 60.0f : 0.0f, now);
        if (foeAlive) aic->Enemy = c.foe;
        return false;
    }
    if (c.cloaked) { c.mineSkip = "cloak"; return false; }
    if (now - c.mineAt < c.mineCool) { c.mineSkip = "cool"; return false; }
    const int knob = MineKnob(now);
    if (knob == 0) { c.mineSkip = "knob0"; return false; }
    if (escaping) {  // dropped behind while running, no stop
        if (knob < 2 || !foeAlive || fd > kMineEscape) { c.mineSkip = "esc-far"; return false; }
        c.mineSkip = "-"; c.mineWhy = "escape";
        LayMine(aic, bot, c, dev, 0.0f, now);
        return false;
    }
    const bool foeSeen = foeAlive && c.lastSight;
    const char* why = nullptr;
    if (knob >= 2 && foeSeen && fd < kMineClose) why = "close";
    else if (c.objective && !foeSeen && now - c.mineObjAt >= kMineObjGap) {
        AActor* g = ObjectiveGoalActor(c.objective);
        float gd2 = 1.0e18f;
        if (g) { const float gx = bot->Location.X - g->Location.X, gy = bot->Location.Y - g->Location.Y; gd2 = gx * gx + gy * gy; }
        if (c.objOn || gd2 < kMineNear * kMineNear) why = "objective";
    }
    if (!why) { c.mineSkip = foeSeen ? "fight" : "nowhy"; return false; }
    c.mineSkip = "-";
    c.mineWhy = why; c.mineWind = now + kMineWind;
    ATgDevice* cur = (ATgDevice*)bot->Weapon;
    if (cur && bot->IsFiring()) { bot->StopAction(cur, 0); bot->StopAction(cur, 1); }
    aic->Enemy = nullptr;
    bot->Acceleration = FVector(0, 0, 0);
    Logger::Log("chat-command", "[PvpBot] ab mine windup pawn=%d why=%s foe=%.0f obj=%d knob=%d\n",
        c.pawnId, why, foeAlive ? fd : -1.0f, c.objective ? c.objective->nObjectiveId : -1, knob);
    return true;
}

// cs_settings pvpbot_ab_medst: 0 off (every robotics keeps Force Wall),
// 1 = half the robotics carry Medical Station, objective only; 2 = + hurt with
// no foe in sight, staying on it through the 15 s deploy (default). Class "robotics-medst" / "robotics-wall" forces the variant.
constexpr float kMedstHurt = 0.60f, kMedstAhead = 80.0f, kMedstHold = 8.0f, kMedstFull = 0.90f, kMedstLeave = 250.0f;
constexpr int kPropMedstDeploy = 279;  // Deploy time on the thrower mode (15 s): it heals only after that
int MedstKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_medst'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] medst knob pvpbot_ab_medst=%d\n", nv);
        v = nv;
    }
    return v;
}
// cs_settings pvpbot_ab_pwst: 0 off (no Power Station variant), 1 = on (default).
int PwstKnob(float now) {
    static int v = 1;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_pwst'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 1;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 1) nv = 1;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] pwst knob pvpbot_ab_pwst=%d\n", nv);
        v = nv;
    }
    return v;
}
bool TickWall(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!c.robotics || !aic->WorldInfo) return false;
    ATgDevice* dev = bot->m_EquippedDevices[kWallPoint];
    const float now = aic->WorldInfo->TimeSeconds;
    UTgDeviceFire* fm = dev && dev->m_FireMode.Count > 0 ? dev->m_FireMode.Data[0] : nullptr;
    int depId = 0;
    if (fm && fm->m_pFireModeSetup.Dummy) depId = *(int*)((char*)fm->m_pFireModeSetup.Dummy + 0x28);
    const bool pw = dev && dev->r_nDeviceId == kPowerStation;  // Set down like the Medical Station
    const bool ms = dev && (dev->r_nDeviceId == kMedStation || pw);
    if (depId <= 0) depId = pw ? kPowerStationDeployable : ms ? kMedStationDeployable : kWallDeployable;
    const int mx = bot->r_nHealthMaximum > 0 ? bot->r_nHealthMaximum : bot->HealthMax;
    if (now - c.wallDiagAt >= 10.0f) {
        c.wallDiagAt = now;
        Logger::Log("chat-command", "[PvpBot] ab wall state pawn=%d dev=%d dep=%d cool=%.1f/%.1f hp=%d/%d obj=%d skip=%s\n",
            c.pawnId, dev ? dev->r_nDeviceId : 0, depId, now - c.wallAt, c.wallCool, bot->Health, mx,
            c.objective ? c.objective->nObjectiveId : -1, c.wallSkip);
    }
    if (!dev) return false;
    const bool foeOk = c.foe && PawnAlive(c.foe, (APawn*)bot);
    if (ms && c.medHold > 0.0f) {  // Heal hold on the station
        const float hx = bot->Location.X - c.medSpot.X, hy = bot->Location.Y - c.medSpot.Y;
        const char* end = nullptr;
        if (mx > 0 && bot->Health >= (int)(mx * kMedstFull)) end = "full";
        else if (hx * hx + hy * hy > kMedstLeave * kMedstLeave) end = "left";
        else if (now >= c.medHold) end = "time";
        if (end) {
            Logger::Log("chat-command", "[PvpBot] ab medst hold end pawn=%d why=%s hp=%d->%d/%d d=%.0f over=%.1f\n", c.pawnId, end, c.medHp0, bot->Health, mx,
                std::sqrt(hx * hx + hy * hy), now - c.medHold);
            c.medHold = 0.0f;
        } else {
            bot->Acceleration = FVector(0, 0, 0);
            bot->Velocity.X = 0.0f; bot->Velocity.Y = 0.0f;
            if (foeOk && c.lastSight) aic->Enemy = c.foe;
            return true;
        }
    }
    if (c.wallWind > 0.0f) {  // tell running
        aic->Enemy = nullptr;
        bot->Acceleration = FVector(0, 0, 0);
        if (now < c.wallWind) return true;
        c.wallWind = 0.0f;
        c.wallAt = now; c.wallCool = kWallRetry;
        if (ms) {  // Medical Station on the ground just in front of the bot
            const float yaw = bot->Rotation.Yaw * 3.14159265f / 32768.0f;
            FVector st0 = {bot->Location.X + std::cos(yaw) * kMedstAhead, bot->Location.Y + std::sin(yaw) * kMedstAhead, bot->Location.Z + 60.0f};
            FVector en0 = {st0.X, st0.Y, st0.Z - 700.0f};
            FVector sp = {0, 0, 0}, nn = {0, 0, 1};
            FTraceHitInfo h0;
            const bool gr = bot->Trace(en0, st0, 0, FVector(0, 0, 0), 0, &sp, &nn, &h0) != nullptr;
            if (!gr) {
                sp = bot->Location; nn = FVector(0, 0, 1);
                if (bot->CylinderComponent) sp.Z -= bot->CylinderComponent->CollisionHeight;
            }
            ATgDeployable* m = TgProj_Deployable__SpawnDeployable::SpawnDeployableActor(bot, depId, sp, nn, dev, fm);
            const float rc0 = DeviceProp(dev, kPropRecharge);
            if (m) { c.wallCool = rc0 > 0.0f ? rc0 : 60.0f; c.wallN++; if (std::string(c.wallWhy) == "hurt") { c.medHold = now + DeviceProp(dev, kPropMedstDeploy) + kMedstHold; c.medSpot = bot->Location; c.medHp0 = bot->Health; } }
            Logger::Log("chat-command", "[PvpBot] ab %s use pawn=%d dev=%d why=%s ok=%d dep=%d ground=%d at=(%.0f,%.0f,%.0f) hp=%d/%d dephp=%d n=%d recharge=%.1f\n",
                pw ? "pwst" : "medst", c.pawnId, dev->r_nDeviceId, c.wallWhy, (int)(m != nullptr), depId, (int)gr, sp.X, sp.Y, sp.Z, bot->Health, mx,
                m ? m->r_nHealth : 0, c.wallN, c.wallCool);
            if (foeOk) aic->Enemy = c.foe;
            return false;
        }
        FVector to = foeOk ? c.foe->Location : bot->Location;
        const float dx = to.X - bot->Location.X, dy = to.Y - bot->Location.Y;
        const float l = std::sqrt(dx * dx + dy * dy);
        const float ux = l > 1.0f ? dx / l : 1.0f, uy = l > 1.0f ? dy / l : 0.0f;
        float dist = DeviceProp(dev, 5) * 16.0f;  // data range (15 = 240 uu)
        if (dist < 150.0f || dist > 400.0f) dist = 240.0f;
        if (l > 1.0f && dist > l * 0.5f) dist = l * 0.5f;
        FVector start = {bot->Location.X + ux * dist, bot->Location.Y + uy * dist, bot->Location.Z + 60.0f};
        FVector end = {start.X, start.Y, start.Z - 700.0f};
        FVector spot = {0, 0, 0}, n = {0, 0, 1};
        FTraceHitInfo hi;
        const bool ground = bot->Trace(end, start, 0, FVector(0, 0, 0), 0, &spot, &n, &hi) != nullptr;
        if (!ground) {
            spot = bot->Location; n = FVector(0, 0, 1);
            if (bot->CylinderComponent) spot.Z -= bot->CylinderComponent->CollisionHeight;
        }
        // Face the foe so the wall spans across the line to it.
        FRotator face = {0, (int)(std::atan2(uy, ux) * 32768.0f / 3.14159265f), 0};
        aic->Rotation = face; bot->Rotation = face;
        ATgDeployable* w = TgProj_Deployable__SpawnDeployable::SpawnDeployableActor(bot, depId, spot, n, dev, fm);
        const float rc = DeviceProp(dev, kPropRecharge);
        if (w) { c.wallCool = rc > 0.0f ? rc : 60.0f; c.wallN++; }
        Logger::Log("chat-command", "[PvpBot] ab wall use pawn=%d why=%s ok=%d dep=%d ground=%d at=(%.0f,%.0f,%.0f) dist=%.0f foe=%.0f hp=%d n=%d recharge=%.1f\n",
            c.pawnId, c.wallWhy, (int)(w != nullptr), depId, (int)ground, spot.X, spot.Y, spot.Z, dist, l,
            w ? w->r_nHealth : 0, c.wallN, c.wallCool);
        if (foeOk) aic->Enemy = c.foe;
        return false;
    }
    if (c.cloaked) { c.wallSkip = "cloak"; return false; }
    if (now - c.wallAt < c.wallCool) { c.wallSkip = "cool"; return false; }
    if (ms) {  // Medical Station: hurt, or holding its objective with no foe in sight
        const int mk = pw ? PwstKnob(now) : MedstKnob(now);
        if (mk == 0) { c.wallSkip = "knob0"; return false; }
        const bool seen = foeOk && c.lastSight;
        const char* mwhy = nullptr;
        if (c.objective && c.objOn && !seen) mwhy = "objective";
        else if (!pw && mk >= 2 && !seen && mx > 0 && bot->Health < (int)(mx * kMedstHurt)) mwhy = "hurt";  // Power Station: objective only
        if (!mwhy) { c.wallSkip = "nowhy"; return false; }
        c.wallSkip = "-";
        c.wallWhy = mwhy; c.wallWind = now + kWallWind;
        ATgDevice* cw = (ATgDevice*)bot->Weapon;
        if (cw && bot->IsFiring()) { bot->StopAction(cw, 0); bot->StopAction(cw, 1); }
        aic->Enemy = nullptr;
        bot->Acceleration = FVector(0, 0, 0);
        Logger::Log("chat-command", "[PvpBot] ab %s windup pawn=%d why=%s foe=%.0f hp=%d/%d knob=%d\n",
            pw ? "pwst" : "medst", c.pawnId, mwhy, foeOk ? std::sqrt(Dist2((APawn*)bot, c.foe)) : -1.0f, bot->Health, mx, mk);
        return true;
    }
    const int knob = WallKnob(now);
    if (knob == 0) { c.wallSkip = "knob0"; return false; }
    if (!foeOk || !c.lastSight) { c.wallSkip = "nofoe"; return false; }
    const float d2 = Dist2((APawn*)bot, c.foe);
    if (d2 < kWallMin * kWallMin || d2 > kWallMax * kWallMax) { c.wallSkip = "range"; return false; }
    const char* why = nullptr;
    if (c.objective) why = "objective";
    else if (knob >= 2 && mx > 0 && bot->Health < (int)(mx * kWallHurt)) why = "hurt";
    if (!why) { c.wallSkip = "nowhy"; return false; }
    c.wallSkip = "-";
    c.wallWhy = why; c.wallWind = now + kWallWind;
    ATgDevice* cur = (ATgDevice*)bot->Weapon;
    if (cur && bot->IsFiring()) { bot->StopAction(cur, 0); bot->StopAction(cur, 1); }
    aic->Enemy = nullptr;
    aic->Focus = (AActor*)c.foe;
    bot->Acceleration = FVector(0, 0, 0);
    Logger::Log("chat-command", "[PvpBot] ab wall windup pawn=%d why=%s foe=%.0f hp=%d/%d knob=%d\n",
        c.pawnId, why, std::sqrt(d2), bot->Health, mx, knob);
    return true;
}

// Assault Range Shield: what a player does with it - pops it when taking
// fire from a foe at range (why=fire: HP fell since the last tick, foe in sight
// >= 500 uu) or when hurt (why=hurt: HP < 50 %, foe in sight >= 500 uu). Not
// at melee range (ranged protection only), not cloaked, not while a personal
// shield is already up. Applies the fire mode's own groups to itself (same
// groups/visual a player gets); recharge = data prop 4 (60 s), failed tries 8 s.
// Instant self buff (no wind-up; the shield itself is the tell).
// cs_settings pvpbot_ab_shield: 0 off, 1 hurt only, 2 + under fire (default).
constexpr float kShieldMin = 500.0f, kShieldHurt = 0.50f, kShieldRetry = 8.0f;
int ShieldKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_shield'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] shield knob pvpbot_ab_shield=%d\n", nv);
        v = nv;
    }
    return v;
}
// cs_settings pvpbot_ab_ptarget: 0 off (every assault keeps Range Shield),
// 1 = half the assaults carry Perfect Target, hurt only; 2 = half + under fire
// (default). Class "assault-pt" / "assault-shield" forces the variant.
int PtargetKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_ptarget'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] ptarget knob pvpbot_ab_ptarget=%d\n", nv);
        v = nv;
    }
    return v;
}
void TickShield(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!c.assault || !aic->WorldInfo || !bot->r_EffectManager) return;
    ATgDevice* dev = bot->m_EquippedDevices[kShieldPoint];
    const float now = aic->WorldInfo->TimeSeconds;
    UTgDeviceFire* fm = dev && dev->m_FireMode.Count > 0 ? dev->m_FireMode.Data[0] : nullptr;
    const int hp = bot->Health;
    const bool dropped = c.shieldHp > 0 && hp < c.shieldHp;
    c.shieldHp = hp;
    const int mx = bot->r_nHealthMaximum > 0 ? bot->r_nHealthMaximum : bot->HealthMax;
    if (now - c.shieldDiagAt >= 10.0f) {
        c.shieldDiagAt = now;
        Logger::Log("chat-command", "[PvpBot] ab shield state pawn=%d dev=%d cool=%.1f/%.1f hp=%d/%d up=%d skip=%s\n",
            c.pawnId, dev ? dev->r_nDeviceId : 0, now - c.shieldAt, c.shieldCool, hp, mx,
            (int)HasCategory(bot, kShieldCategory), c.shieldSkip);
    }
    if (!fm) return;
    const bool pt = dev->r_nDeviceId == kPerfectTarget;
    const int cat = pt ? kPerfectCategory : kShieldCategory;
    if (now - c.shieldAt < c.shieldCool) { c.shieldSkip = "cool"; return; }
    const int knob = pt ? PtargetKnob(now) : ShieldKnob(now);
    if (knob == 0) { c.shieldSkip = "knob0"; return; }
    if (c.cloaked) { c.shieldSkip = "cloak"; return; }
    if (!c.foe || !PawnAlive(c.foe, (APawn*)bot) || !c.lastSight) { c.shieldSkip = "nofoe"; return; }
    const float d = std::sqrt(Dist2((APawn*)bot, c.foe));
    if (!pt && d < kShieldMin) { c.shieldSkip = "melee"; return; }
    if (HasCategory(bot, cat)) { c.shieldSkip = "up"; return; }
    const char* why = nullptr;
    if (mx > 0 && hp < (int)(mx * kShieldHurt)) why = "hurt";
    else if (knob >= 2 && dropped) why = "fire";
    if (!why) { c.shieldSkip = "nowhy"; return; }
    c.shieldSkip = "-";
    if (fm->s_EffectGroupList.Count == 0) { int idx = 0; TgDeviceFire__GetEffectGroup::Call(fm, nullptr, -1, &idx); }
    int applied = 0;
    for (int i = 0; i < fm->s_EffectGroupList.Count; i++) {
        UTgEffectGroup* t = fm->s_EffectGroupList.Data[i];
        if (!t) continue;
        UTgEffectGroup* g = BuildEffectGroup(t->m_nEffectGroupId, t->m_nType);
        if (!g) continue;
        FImpactInfo impact{};
        bot->r_EffectManager->eventProcessEffect(g, 0, nullptr, (AActor*)bot, impact);
        applied++;
    }
    const int up = (int)HasCategory(bot, cat);
    const float rc = DeviceProp(dev, kPropRecharge);
    c.shieldAt = now;
    c.shieldCool = up ? (rc > 0.0f ? rc : 60.0f) : kShieldRetry;
    if (up) c.shieldN++;
    Logger::Log("chat-command", "[PvpBot] ab %s use pawn=%d why=%s ok=%d groups=%d/%d foe=%.0f hp=%d/%d n=%d recharge=%.1f\n",
        pt ? "ptarget" : "shield", c.pawnId, why, up, applied, fm->s_EffectGroupList.Count, d, hp, mx, c.shieldN, c.shieldCool);
}

// Medic Regeneration: what a player does with it - pops it when hurt
// (why=hurt: HP < 50 % with a foe in sight; why=low: HP < 30 %
// any time, knob 2). Not while the regen is already running. Applies the fire
// mode's own groups to itself (same groups/visual a player gets); recharge =
// data prop 4 (60 s), failed tries 8 s. Instant self buff (no wind-up).
// cs_settings pvpbot_ab_regen: 0 off (every medic keeps Neutralize Wave),
// 1 = half the medics carry it, hurt only; 2 = half + why=low (default).
// Class "medic-regen" / "medic-neut" forces the variant (dev/test bots).
constexpr float kRegenHurt = 0.50f, kRegenLow = 0.30f, kRegenRetry = 8.0f;
int RegenKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_regen'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] regen knob pvpbot_ab_regen=%d\n", nv);
        v = nv;
    }
    return v;
}
void TickRegen(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!c.medic || !c.regen || !aic->WorldInfo || !bot->r_EffectManager) return;
    ATgDevice* dev = bot->m_EquippedDevices[kNeutPoint];
    const float now = aic->WorldInfo->TimeSeconds;
    UTgDeviceFire* fm = dev && dev->r_nDeviceId == kRegen && dev->m_FireMode.Count > 0 ? dev->m_FireMode.Data[0] : nullptr;
    const int hp = bot->Health;
    const int mx = bot->r_nHealthMaximum > 0 ? bot->r_nHealthMaximum : bot->HealthMax;
    if (now - c.regenDiagAt >= 10.0f) {
        c.regenDiagAt = now;
        Logger::Log("chat-command", "[PvpBot] ab regen state pawn=%d dev=%d cool=%.1f/%.1f hp=%d/%d up=%d skip=%s\n",
            c.pawnId, dev ? dev->r_nDeviceId : 0, now - c.regenAt, c.regenCool, hp, mx,
            (int)HasCategory(bot, kRegenCategory), c.regenSkip);
    }
    if (!fm) { c.regenSkip = "nodev"; return; }
    if (now - c.regenAt < c.regenCool) { c.regenSkip = "cool"; return; }
    const int knob = RegenKnob(now);
    if (knob == 0) { c.regenSkip = "knob0"; return; }
    if (hp <= 0 || mx <= 0) { c.regenSkip = "dead"; return; }
    if (HasCategory(bot, kRegenCategory)) { c.regenSkip = "up"; return; }
    const bool fight = c.foe && PawnAlive(c.foe, (APawn*)bot) && c.lastSight;
    const char* why = nullptr;
    if (hp < (int)(mx * kRegenHurt) && fight) why = "hurt";
    else if (knob >= 2 && hp < (int)(mx * kRegenLow)) why = "low";
    if (!why) { c.regenSkip = "nowhy"; return; }
    c.regenSkip = "-";
    if (fm->s_EffectGroupList.Count == 0) { int idx = 0; TgDeviceFire__GetEffectGroup::Call(fm, nullptr, -1, &idx); }
    int applied = 0;
    for (int i = 0; i < fm->s_EffectGroupList.Count; i++) {
        UTgEffectGroup* t = fm->s_EffectGroupList.Data[i];
        if (!t) continue;
        UTgEffectGroup* g = BuildEffectGroup(t->m_nEffectGroupId, t->m_nType);
        if (!g) continue;
        FImpactInfo impact{};
        bot->r_EffectManager->eventProcessEffect(g, 0, nullptr, (AActor*)bot, impact);
        applied++;
    }
    const int up = (int)HasCategory(bot, kRegenCategory);
    const float rc = DeviceProp(dev, kPropRecharge);
    c.regenAt = now;
    c.regenCool = up ? (rc > 0.0f ? rc : 60.0f) : kRegenRetry;
    if (up) c.regenN++;
    Logger::Log("chat-command", "[PvpBot] ab regen use pawn=%d why=%s ok=%d groups=%d/%d hp=%d/%d n=%d recharge=%.1f\n",
        c.pawnId, why, up, applied, fm->s_EffectGroupList.Count, hp, mx, c.regenN, c.regenCool);
}

// Recon Visual Scanner: what a player does with it - pops it when a cloaked
// foe just vanished (why=hidden: current foe hidden now; why=cloak: one was lost
// < 6 s ago) or, knob 2, on its objective with no foe in sight (why=objective).
// Not while the scan is up or the bot is cloaked itself. Own fire-mode groups;
// recharge = data prop 4 (60 s), failed tries 8 s.
// cs_settings pvpbot_ab_scan: 0 off (every recon keeps Fire Bomb), 1 = half the
// recons carry it, cloak whys only; 2 = + objective (default).
// Class suffix "-scan" / "-bomb" forces the variant (dev/test bots).
int ScanKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_scan'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] scan knob pvpbot_ab_scan=%d\n", nv);
        v = nv;
    }
    return v;
}
void TickScan(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!c.recon || !c.scanner || !aic->WorldInfo || !bot->r_EffectManager) return;
    ATgDevice* dev = bot->m_EquippedDevices[kFireBombPoint];
    const float now = aic->WorldInfo->TimeSeconds;
    UTgDeviceFire* fm = dev && dev->r_nDeviceId == kScanner && dev->m_FireMode.Count > 0 ? dev->m_FireMode.Data[0] : nullptr;
    const bool up = HasCategory(bot, kScanCategory);
    if (now - c.scanDiagAt >= 10.0f) {
        c.scanDiagAt = now;
        Logger::Log("chat-command", "[PvpBot] ab scan state pawn=%d dev=%d cool=%.1f/%.1f up=%d hidden=%d lostAge=%.1f obj=%d skip=%s\n",
            c.pawnId, dev ? dev->r_nDeviceId : 0, now - c.scanAt, c.scanCool, (int)up, (int)(c.hiddenSince >= 0.0f),
            now - c.cloakLostAt, (int)c.objOn, c.scanSkip);
    }
    if (!fm) { c.scanSkip = "nodev"; return; }
    if (now - c.scanAt < c.scanCool) { c.scanSkip = "cool"; return; }
    const int knob = ScanKnob(now);
    if (knob == 0) { c.scanSkip = "knob0"; return; }
    if (bot->Health <= 0) { c.scanSkip = "dead"; return; }
    if (up) { c.scanSkip = "up"; return; }
    if (bot->r_bIsStealthed) { c.scanSkip = "cloaked"; return; }
    const bool fight = c.foe && PawnAlive(c.foe, (APawn*)bot) && c.lastSight;
    const char* why = nullptr;
    if (c.foe && c.hiddenSince >= 0.0f) why = "hidden";
    else if (now - c.cloakLostAt < kSensorLost) why = "cloak";
    else if (knob >= 2 && c.objOn && !fight) why = "objective";
    if (!why) { c.scanSkip = "nowhy"; return; }
    c.scanSkip = "-";
    if (fm->s_EffectGroupList.Count == 0) { int idx = 0; TgDeviceFire__GetEffectGroup::Call(fm, nullptr, -1, &idx); }
    int applied = 0;
    for (int i = 0; i < fm->s_EffectGroupList.Count; i++) {
        UTgEffectGroup* t = fm->s_EffectGroupList.Data[i];
        if (!t) continue;
        UTgEffectGroup* g = BuildEffectGroup(t->m_nEffectGroupId, t->m_nType);
        if (!g) continue;
        FImpactInfo impact{};
        bot->r_EffectManager->eventProcessEffect(g, 0, nullptr, (AActor*)bot, impact);
        applied++;
    }
    const int ok = (int)HasCategory(bot, kScanCategory);
    const float rc = DeviceProp(dev, kPropRecharge);
    c.scanAt = now;
    c.scanCool = ok ? (rc > 0.0f ? rc : 60.0f) : 8.0f;
    if (ok) c.scanN++;
    Logger::Log("chat-command", "[PvpBot] ab scan use pawn=%d why=%s ok=%d groups=%d/%d foe=%d obj=%d n=%d recharge=%.1f\n",
        c.pawnId, why, ok, applied, fm->s_EffectGroupList.Count, c.foePawnId,
        c.objective ? c.objective->nObjectiveId : -1, c.scanN, c.scanCool);
}

// Recon Decoy: what a player's Decoy does - the device's SPAWN_PET fire
// mode (bot 735, a copy of the owner that draws fire, 20 s) through the same
// player path as the turret (TgDeviceFire__SpawnPet::SpawnAt). Placed 150 uu
// toward the foe and 120 uu to the side, facing the foe. Triggers: retreating
// (why=escape: dropped at once, then the escape cloak runs) or HP < 60 % with a
// foe in sight 400-2200 uu (why=hurt), or, knob 2, a foe in sight >= 1.5 s at
// 600-2200 uu while not cloaked (why=bait). Bait/hurt tell 0.3 s (stop, face
// the foe, hold fire). Recharge = data prop 4 (60 s), failed tries 8 s.
// cs_settings pvpbot_ab_decoy: 0 off, 1 escape + hurt, 2 + bait (default).
constexpr float kDecoyWind = 0.3f, kDecoyMin = 400.0f, kDecoyMax = 2200.0f, kDecoyBaitMin = 600.0f,
                kDecoyHurt = 0.60f, kDecoyRetry = 8.0f;
int DecoyKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_decoy'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] decoy knob pvpbot_ab_decoy=%d\n", nv);
        v = nv;
    }
    return v;
}

void PlaceDecoy(ATgAIController* aic, ATgPawn* bot, Chaser& c, ATgDevice* dev, float now) {
    c.decoyAt = now; c.decoyCool = kDecoyRetry;
    UTgDeviceFire* fm = nullptr;
    int petId = 0;
    for (int i = 0; i < dev->m_FireMode.Num(); i++) {
        UTgDeviceFire* f = dev->m_FireMode(i);
        char* setup = f ? (char*)f->m_pFireModeSetup.Dummy : nullptr;
        if (setup && *(int*)(setup + 0x2C)) { fm = f; petId = *(int*)(setup + 0x2C); break; }
    }
    if (!fm) {
        Logger::Log("chat-command", "[PvpBot] ab decoy nofm pawn=%d modes=%d\n", c.pawnId, dev->m_FireMode.Num());
        return;
    }
    const bool foeOk = c.foe && PawnAlive(c.foe, (APawn*)bot);
    const FVector to = foeOk ? c.foe->Location : bot->Location;
    const float dx = to.X - bot->Location.X, dy = to.Y - bot->Location.Y;
    const float l = std::sqrt(dx * dx + dy * dy);
    const float ux = l > 1.0f ? dx / l : 1.0f, uy = l > 1.0f ? dy / l : 0.0f;
    const float side = (c.decoyN % 2) ? -120.0f : 120.0f;
    FVector start = {bot->Location.X + ux * 150.0f - uy * side, bot->Location.Y + uy * 150.0f + ux * side, bot->Location.Z + 60.0f};
    FVector end = {start.X, start.Y, start.Z - 700.0f};
    FVector spot = {0, 0, 0}, n = {0, 0, 0};
    FTraceHitInfo hi;
    const bool ground = bot->Trace(end, start, 0, FVector(0, 0, 0), 0, &spot, &n, &hi) != nullptr;
    if (!ground) {
        spot = bot->Location;
        if (bot->CylinderComponent) spot.Z -= bot->CylinderComponent->CollisionHeight;
    }
    float radius = 0.0f, halfHeight = 0.0f;
    TgGame__SpawnBotById::GetBotCollisionCylinder(petId, &radius, &halfHeight);
    spot.Z += halfHeight + 5.0f;
    FRotator rot = {0, (int)(std::atan2(uy, ux) * 32768.0f / 3.14159265f), 0};
    ATgPawn* pet = TgDeviceFire__SpawnPet::SpawnAt(fm, bot, spot, rot);
    const float rc = DeviceProp(dev, kPropRecharge);
    if (pet) { c.decoyCool = rc > 0.0f ? rc : 60.0f; c.decoyN++; }
    Logger::Log("chat-command", "[PvpBot] ab decoy use pawn=%d why=%s ok=%d pet=%d id=%d decoy=%d ground=%d at=(%.0f,%.0f,%.0f) foe=%.0f "
        "hp=%d n=%d recharge=%.1f\n",
        c.pawnId, c.decoyWhy, (int)(pet != nullptr), petId, pet ? pet->r_nPawnId : -1, pet ? (int)pet->r_bIsDecoy : -1,
        (int)ground, spot.X, spot.Y, spot.Z, l, pet ? pet->Health : 0, c.decoyN, c.decoyCool);
}

// Returns true while the tell runs (the bot stands still).
bool TickDecoy(ATgAIController* aic, ATgPawn* bot, Chaser& c, bool escaping = false) {
    if (!c.recon || !aic->WorldInfo) return false;
    ATgDevice* dev = bot->m_EquippedDevices[kDecoyPoint];
    const float now = aic->WorldInfo->TimeSeconds;
    const int mx = bot->r_nHealthMaximum > 0 ? bot->r_nHealthMaximum : bot->HealthMax;
    if (!escaping && now - c.decoyDiagAt >= 10.0f) {
        c.decoyDiagAt = now;
        Logger::Log("chat-command", "[PvpBot] ab decoy state pawn=%d dev=%d cool=%.1f/%.1f hp=%d/%d cloaked=%d skip=%s\n",
            c.pawnId, dev ? dev->r_nDeviceId : 0, now - c.decoyAt, c.decoyCool, bot->Health, mx, (int)c.cloaked, c.decoySkip);
    }
    if (!dev) return false;
    const bool foeOk = c.foe && PawnAlive(c.foe, (APawn*)bot);
    if (c.decoyWind > 0.0f) {  // tell running
        if (escaping) { c.decoyWind = 0.0f; }  // retreat wins: drop it now
        else {
            aic->Enemy = nullptr;
            bot->Acceleration = FVector(0, 0, 0);
            if (now < c.decoyWind) return true;
            c.decoyWind = 0.0f;
        }
        PlaceDecoy(aic, bot, c, dev, now);
        if (foeOk && !escaping) aic->Enemy = c.foe;
        return false;
    }
    if (c.cloaked) { c.decoySkip = "cloak"; return false; }
    if (now - c.decoyAt < c.decoyCool) { c.decoySkip = "cool"; return false; }
    const int knob = DecoyKnob(now);
    if (knob == 0) { c.decoySkip = "knob0"; return false; }
    if (!foeOk || !c.lastSight) { c.decoySkip = "nofoe"; return false; }
    const float d2 = Dist2((APawn*)bot, c.foe);
    if (escaping) {
        if (d2 > kDecoyMax * kDecoyMax) { c.decoySkip = "range"; return false; }
        c.decoySkip = "-"; c.decoyWhy = "escape";
        PlaceDecoy(aic, bot, c, dev, now);
        return false;
    }
    if (d2 < kDecoyMin * kDecoyMin || d2 > kDecoyMax * kDecoyMax) { c.decoySkip = "range"; return false; }
    const char* why = nullptr;
    if (mx > 0 && bot->Health < (int)(mx * kDecoyHurt)) why = "hurt";
    else if (knob >= 2 && d2 >= kDecoyBaitMin * kDecoyBaitMin && now - c.sightSince >= 1.5f) why = "bait";
    if (!why) { c.decoySkip = "nowhy"; return false; }
    c.decoySkip = "-";
    c.decoyWhy = why; c.decoyWind = now + kDecoyWind;
    ATgDevice* cur = (ATgDevice*)bot->Weapon;
    if (cur && bot->IsFiring()) { bot->StopAction(cur, 0); bot->StopAction(cur, 1); }
    aic->Enemy = nullptr;
    aic->Focus = (AActor*)c.foe;
    bot->Acceleration = FVector(0, 0, 0);
    Logger::Log("chat-command", "[PvpBot] ab decoy windup pawn=%d why=%s foe=%.0f hp=%d/%d knob=%d\n",
        c.pawnId, why, std::sqrt(d2), bot->Health, mx, knob);
    return true;
}

bool TickTurret(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!c.robotics || !aic->WorldInfo) return false;
    ATgDevice* spec = bot->m_EquippedDevices[3];
    if (!spec) return false;
    const float now = aic->WorldInfo->TimeSeconds;
    if (!c.foe || !PawnAlive(c.foe, (APawn*)bot) || !c.lastSight || now - c.turretAt < (std::max)(kTurretRetry, c.turretCool)) return false;
    if (Dist2((APawn*)bot, c.foe) > kTurretRange * kTurretRange || TurretAlive(c, (APawn*)bot)) return false;
    c.turretAt = now;
    c.turretCool = kTurretRetry;
    UTgDeviceFire* fm = nullptr;
    int petId = 0;
    for (int i = 0; i < spec->m_FireMode.Num(); i++) {
        UTgDeviceFire* f = spec->m_FireMode(i);
        char* setup = f ? (char*)f->m_pFireModeSetup.Dummy : nullptr;
        if (setup && *(int*)(setup + 0x2C)) { fm = f; petId = *(int*)(setup + 0x2C); break; }
    }
    if (!fm) {
        Logger::Log("chat-command", "[PvpBot] turret nofm pawn=%d modes=%d\n", c.pawnId, spec->m_FireMode.Num());
        return false;
    }
    const float dx = c.foe->Location.X - bot->Location.X, dy = c.foe->Location.Y - bot->Location.Y;
    const float l = std::sqrt(dx * dx + dy * dy);
    const float ux = l > 1.0f ? dx / l : 1.0f, uy = l > 1.0f ? dy / l : 0.0f;
    FVector start = {bot->Location.X + ux * 250.0f, bot->Location.Y + uy * 250.0f, bot->Location.Z + 60.0f};
    FVector end = {start.X, start.Y, start.Z - 700.0f};
    FVector spot = {0, 0, 0}, n = {0, 0, 0};
    FTraceHitInfo hi;
    const bool ground = bot->Trace(end, start, 0, FVector(0, 0, 0), 0, &spot, &n, &hi) != nullptr;
    if (!ground) {  // gap ahead: at the bot's own feet
        spot = bot->Location;
        if (bot->CylinderComponent) spot.Z -= bot->CylinderComponent->CollisionHeight;
    }
    float radius = 0.0f, halfHeight = 0.0f;
    TgGame__SpawnBotById::GetBotCollisionCylinder(petId, &radius, &halfHeight);
    spot.Z += halfHeight + 5.0f;
    FRotator rot = {0, (int)(std::atan2(uy, ux) * 32768.0f / 3.14159265f), 0};
    ATgPawn* pet = TgDeviceFire__SpawnPet::SpawnAt(fm, bot, spot, rot);
    if (pet) {
        c.turret = (APawn*)pet; c.turretId = pet->r_nPawnId; c.turretThrows++;
        // Same recharge as the player's Personal Turret (data prop 4,
        // 40 s on 2300), counted from the placement like the player's cooldown.
        const float rc = DeviceProp(spec, kPropRecharge);
        c.turretCool = rc > 0.0f ? rc : 40.0f;
    }
    Logger::Log("chat-command", "[PvpBot] turret placed pawn=%d dev=%d pet=%d id=%d ok=%d ground=%d at=(%.0f,%.0f,%.0f) hp=%d n=%d dist=%.0f recharge=%.1f\n",
        c.pawnId, spec->r_nDeviceId, petId, pet ? pet->r_nPawnId : -1, (int)(pet != nullptr), (int)ground, spot.X, spot.Y, spot.Z,
        pet ? pet->Health : 0, c.turretThrows, l, c.turretCool);
    return false;
}

// Assault grenade. Trigger (like a player): foe in sight 450-2000 uu
// and either >= 2 opponents inside the grenade's damage radius around it
// (why=clump) or, knob 2, a single foe seen for >= 1 s (why=single). Tell:
// kNadeWind s standing still facing the spot (Enemy cleared = no rifle),
// then ProjectileFire on the grenade's projectile fire mode; the projectile
// velocity is replaced by a toss onto the foe's feet (the AI aim would
// throw flat). Never within 3 s of a cloak change. cs_settings
// pvpbot_ab_grenade: 0 = off, 1 = clumps only, 2 = clumps + single (default).
constexpr float kNadeWind = 0.6f, kNadeMin = 450.0f, kNadeMax = 2000.0f, kNadeRetry = 8.0f;
int GrenadeKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_grenade'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] grenade knob pvpbot_ab_grenade=%d\n", nv);
        v = nv;
    }
    return v;
}

// cs_settings pvpbot_ab_poisonnade (medic Poison Grenade): 0 off, 1 clumps only, 2 + single (default).
int PoisonNadeKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_poisonnade'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] poisonnade knob pvpbot_ab_poisonnade=%d\n", nv);
        v = nv;
    }
    return v;
}

// cs_settings pvpbot_ab_firebomb (recon Fire Bomb): 0 off, 1 clumps only, 2 + single (default).
int FireBombKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_firebomb'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] firebomb knob pvpbot_ab_firebomb=%d\n", nv);
        v = nv;
    }
    return v;
}

int ProjectileMode(ATgDevice* dev) {
    for (int m = 0; dev && m < dev->m_FireMode.Count; m++) {
        UTgDeviceFire* fm = dev->m_FireMode.Data[m];
        if (fm && fm->m_nFireType == 1 /*EWFT_Projectile*/) return m;
    }
    return -1;
}

bool TickGrenade(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if ((!c.assault && !c.medic && !c.recon) || !aic->WorldInfo) return false;  // + recon
    // Medic: Poison Grenade; recon: Fire Bomb
    ATgDevice* nade = bot->m_EquippedDevices[c.assault ? kGrenadePoint : c.recon ? kFireBombPoint : kPoisonPoint];
    const float now = aic->WorldInfo->TimeSeconds;
    if (now - c.nadeDiagAt >= 10.0f) {  // Status line (tuning)
        c.nadeDiagAt = now;
        Logger::Log("chat-command", "[PvpBot] ab grenade state pawn=%d dev=%d mode=%d foe=%d sight=%d dist=%.0f cool=%.1f/%.1f cloakAge=%.1f skip=%s sightAge=%.1f\n",
            c.pawnId, nade ? nade->r_nDeviceId : 0, nade ? ProjectileMode(nade) : -2, (int)(c.foe != nullptr), (int)c.lastSight,
            c.foe ? std::sqrt(Dist2((APawn*)bot, c.foe)) : -1.0f, now - c.nadeAt, c.nadeCool, now - c.cloakAt, c.nadeSkip, now - c.sightSince);
    }
    if (!nade) return false;
    if (c.recon && nade->r_nDeviceId != kFireBomb) return false;  // Scanner recon
    if (c.nadeWind > 0.0f) {  // wind-up running
        if (!c.nadeFoe || c.nadeFoe != c.foe || !PawnAlive(c.nadeFoe, (APawn*)bot)) {
            Logger::Log("chat-command", "[PvpBot] ab grenade skip pawn=%d why=foe lost in wind-up\n", c.pawnId);
            c.nadeWind = 0.0f; c.nadeFoe = nullptr; c.nadeAt = now; c.nadeCool = kNadeRetry;
            return false;
        }
        aic->Enemy = nullptr; aic->Focus = (AActor*)c.nadeFoe;
        bot->Acceleration = FVector(0, 0, 0);
        if (now < c.nadeWind) return true;
        c.nadeWind = 0.0f;
        const int mi = ProjectileMode(nade);
        c.nadeAt = now; c.nadeCool = kNadeRetry;
        if (mi < 0) {
            Logger::Log("chat-command", "[PvpBot] ab grenade fail pawn=%d why=no projectile mode modes=%d type0=%d\n", c.pawnId,
                nade->m_FireMode.Count, nade->m_FireMode.Count > 0 && nade->m_FireMode.Data[0] ? (int)nade->m_FireMode.Data[0]->m_nFireType : -1);
            return false;
        }
        nade->CurrentFireMode = (unsigned char)mi;
        AProjectile* pr = nade->ProjectileFire();
        FVector dest = c.nadeFoe->Location;
        if (c.nadeFoe->CylinderComponent) dest.Z -= c.nadeFoe->CylinderComponent->CollisionHeight * 0.8f;
        int tossed = 0;
        float spd = 0.0f;
        float grav = 0.0f;
        if (pr) {
            spd = pr->Speed > 0.0f ? pr->Speed : nade->m_FireMode.Data[mi]->GetProjectileSpeed();
            if (pr->Physics == 2 /*PHYS_Falling*/) {
                // Own ballistic solve (SuggestTossVelocity found none): low
                // arc at the projectile's speed under its gravity; out of reach -> 45 deg.
                grav = pr->GetGravityZ();
                const float g = grav < -1.0f ? -grav : 520.0f;
                const float hx = dest.X - pr->Location.X, hy = dest.Y - pr->Location.Y, hz = dest.Z - pr->Location.Z;
                const float h = std::sqrt(hx * hx + hy * hy);
                if (spd > 0.0f && h > 1.0f) {
                    const float v2 = spd * spd;
                    const float disc = v2 * v2 - g * (g * h * h + 2.0f * hz * v2);
                    float ang = 0.7853982f;
                    if (disc >= 0.0f) { ang = std::atan((v2 - std::sqrt(disc)) / (g * h)); tossed = 1; }
                    else tossed = 3;
                    const float ch = std::cos(ang) * spd, cz = std::sin(ang) * spd;
                    pr->Velocity = FVector(hx / h * ch, hy / h * ch, cz);
                }
            } else if (spd > 0.0f) {
                FVector d = {dest.X - pr->Location.X, dest.Y - pr->Location.Y, dest.Z - pr->Location.Z};
                const float l = std::sqrt(d.X * d.X + d.Y * d.Y + d.Z * d.Z);
                if (l > 1.0f) { pr->Velocity = FVector(d.X / l * spd, d.Y / l * spd, d.Z / l * spd); tossed = 2; }
            }
            pr->bNetDirty = 1; pr->bForceNetUpdate = 1;
            const float rc = DeviceProp(nade, kPropRecharge);
            c.nadeCool = rc > 0.0f ? rc : 60.0f;
            c.nadeThrows++;
        }
        Logger::Log("chat-command", "[PvpBot] ab grenade use pawn=%d why=%s ok=%d proj=%s phys=%d toss=%d grav=%.0f speed=%.0f dist=%.0f n=%d recharge=%.1f dev=%d\n",
            c.pawnId, c.nadeWhy, (int)(pr != nullptr), pr ? pr->GetName() : "-", pr ? (int)pr->Physics : -1, tossed, grav, spd,
            std::sqrt(Dist2((APawn*)bot, c.nadeFoe)), c.nadeThrows, c.nadeCool, nade->r_nDeviceId);
        aic->Enemy = c.nadeFoe;
        c.nadeFoe = nullptr;
        return false;
    }
    // C.nadeSkip = why the last check did not throw (state line)
    if (!c.foe || !c.lastSight || !PawnAlive(c.foe, (APawn*)bot)) { c.nadeSkip = "nofoe"; return false; }
    if (c.cloaked || c.tellUntil > 0.0f) { c.nadeSkip = "cloak"; return false; }
    if (now - c.nadeAt < c.nadeCool || now - c.cloakAt < 3.0f) { c.nadeSkip = "cool"; return false; }
    const int knob = c.assault ? GrenadeKnob(now) : c.recon ? FireBombKnob(now) : PoisonNadeKnob(now);
    if (knob == 0) { c.nadeSkip = "knob0"; return false; }
    const float d2 = Dist2((APawn*)bot, c.foe);
    const float nmax = c.assault ? kNadeMax : kPoisonNadeMax, nmin = c.assault ? kNadeMin : kPoisonNadeMin;
    if (d2 < nmin * nmin || d2 > nmax * nmax) { c.nadeSkip = "range"; return false; }
    const int mi = ProjectileMode(nade);
    float rad = mi >= 0 ? nade->m_FireMode.Data[mi]->GetDamageRadius() : 0.0f;
    if (rad < 150.0f) rad = 300.0f;
    ATgRepInfo_TaskForce* mine = TfOf((APawn*)bot);
    int nFoes = 0;
    for (APawn* q = bot->WorldInfo->PawnList; q; q = q->NextPawn) {
        if (q->bDeleteMe || q->Health <= 0 || !TfOf(q) || TfOf(q) == mine) continue;
        if (Dist2(q, c.foe) <= rad * rad) nFoes++;
    }
    const char* why = nullptr;
    if (nFoes >= 2) why = "clump";
    else if (knob >= 2 && now - c.sightSince >= 1.0f) why = "single";
    if (!why) { c.nadeSkip = "nowhy"; return false; }
    c.nadeSkip = "-";
    c.nadeWhy = why; c.nadeFoe = c.foe; c.nadeWind = now + kNadeWind;
    aic->Enemy = nullptr; aic->Focus = (AActor*)c.foe;
    bot->Acceleration = FVector(0, 0, 0);
    Logger::Log("chat-command", "[PvpBot] ab grenade windup pawn=%d why=%s foes=%d radius=%.0f dist=%.0f knob=%d dev=%d\n",
        c.pawnId, why, nFoes, rad, std::sqrt(d2), knob, nade->r_nDeviceId);
    return true;
}

bool PbClear(AActor* t, const FVector& end, const FVector& start);
constexpr float kSlamUp = 520.0f, kSlamJetFor = 0.45f;  // Hop
// Assault "jump slam": there is no player slam device for Assault, so
// the bot does what players did - a short jetpack hop, then the Concussion
// Grenade (knockback, data radius 20) thrown down at the foe's feet. Trigger:
// >= 2 opponents within kSlamR of the bot (clump) or, knob 2, one foe in
// sight >= 0.5 s within kSlamSingleR (close). The hop is the tell (kSlamWind s,
// no rifle fire). Recharge from data prop 4 (60 s); never right after an EMP
// throw. cs_settings pvpbot_ab_slam: 0 = off, 1 = clumps only, 2 = both (default).
constexpr float kSlamR = 600.0f, kSlamSingleR = 400.0f, kSlamWind = 0.5f, kSlamRetry = 6.0f, kSlamMinFuel = 1.0f;
int SlamKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_slam'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] slam knob pvpbot_ab_slam=%d\n", nv);
        v = nv;
    }
    return v;
}

// Low ballistic arc for a falling projectile onto dest (45 deg when out of
// reach), straight line otherwise. Returns 1 arc / 3 out of reach / 2 line / 0 none.
int AimProjectileAt(AProjectile* pr, float spd, const FVector& dest, float& grav) {
    grav = 0.0f;
    if (!pr || spd <= 0.0f) return 0;
    const float hx = dest.X - pr->Location.X, hy = dest.Y - pr->Location.Y, hz = dest.Z - pr->Location.Z;
    if (pr->Physics == 2 /*PHYS_Falling*/) {
        grav = pr->GetGravityZ();
        const float g = grav < -1.0f ? -grav : 520.0f;
        const float h = std::sqrt(hx * hx + hy * hy);
        if (h <= 1.0f) { pr->Velocity = FVector(0.0f, 0.0f, -spd * 0.3f); return 1; }
        const float v2 = spd * spd;
        const float disc = v2 * v2 - g * (g * h * h + 2.0f * hz * v2);
        float ang = 0.7853982f;
        int r = 3;
        if (disc >= 0.0f) { ang = std::atan((v2 - std::sqrt(disc)) / (g * h)); r = 1; }
        const float ch = std::cos(ang) * spd, cz = std::sin(ang) * spd;
        pr->Velocity = FVector(hx / h * ch, hy / h * ch, cz);
        return r;
    }
    const float l = std::sqrt(hx * hx + hy * hy + hz * hz);
    if (l <= 1.0f) return 0;
    pr->Velocity = FVector(hx / l * spd, hy / l * spd, hz / l * spd);
    return 2;
}

bool TickSlam(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!c.assault || !aic->WorldInfo) return false;
    ATgDevice* conc = bot->m_EquippedDevices[kConcPoint];
    const float now = aic->WorldInfo->TimeSeconds;
    if (now - c.slamDiagAt >= 10.0f) {
        c.slamDiagAt = now;
        Logger::Log("chat-command", "[PvpBot] ab slam state pawn=%d dev=%d mode=%d foe=%d dist=%.0f cool=%.1f/%.1f fuel=%.1f skip=%s\n",
            c.pawnId, conc ? conc->r_nDeviceId : 0, conc ? ProjectileMode(conc) : -2, (int)(c.foe != nullptr),
            c.foe ? std::sqrt(Dist2((APawn*)bot, c.foe)) : -1.0f, now - c.slamAt, c.slamCool, c.fuel, c.slamSkip);
    }
    if (!conc) return false;
    if (c.slamWind > 0.0f) {  // hop + wind-up running
        if (!c.slamFoe || c.slamFoe != c.foe || !PawnAlive(c.slamFoe, (APawn*)bot)) {
            Logger::Log("chat-command", "[PvpBot] ab slam skip pawn=%d why=foe lost in wind-up\n", c.pawnId);
            c.slamWind = 0.0f; c.slamFoe = nullptr; c.slamAt = now; c.slamCool = kSlamRetry;
            if (c.slamJet) { c.slamJet = false; c.jetting = false; }
            return false;
        }
        aic->Enemy = nullptr; aic->Focus = (AActor*)c.slamFoe;
        if (now < c.slamWind) return true;
        c.slamWind = 0.0f;
        if (c.slamJet) { c.slamJet = false; c.jetting = false; }
        const int mi = ProjectileMode(conc);
        c.slamAt = now; c.slamCool = kSlamRetry;
        if (mi < 0) {
            Logger::Log("chat-command", "[PvpBot] ab slam fail pawn=%d why=no projectile mode modes=%d\n", c.pawnId, conc->m_FireMode.Count);
            aic->Enemy = c.slamFoe; c.slamFoe = nullptr;
            return false;
        }
        conc->CurrentFireMode = (unsigned char)mi;
        AProjectile* pr = conc->ProjectileFire();
        FVector dest = c.slamFoe->Location;
        if (c.slamFoe->CylinderComponent) dest.Z -= c.slamFoe->CylinderComponent->CollisionHeight * 0.8f;
        int tossed = 0;
        float spd = 0.0f, grav = 0.0f;
        if (pr) {
            spd = pr->Speed > 0.0f ? pr->Speed : conc->m_FireMode.Data[mi]->GetProjectileSpeed();
            tossed = AimProjectileAt(pr, spd, dest, grav);
            pr->bNetDirty = 1; pr->bForceNetUpdate = 1;
            const float rc = DeviceProp(conc, kPropRecharge);
            c.slamCool = rc > 0.0f ? rc : 60.0f;
            c.slamN++;
        }
        Logger::Log("chat-command", "[PvpBot] ab slam use pawn=%d why=%s ok=%d proj=%s phys=%d toss=%d grav=%.0f speed=%.0f dist=%.0f rise=%.0f n=%d recharge=%.1f radius=%.0f\n",
            c.pawnId, c.slamWhy, (int)(pr != nullptr), pr ? pr->GetName() : "-", pr ? (int)pr->Physics : -1, tossed, grav, spd,
            std::sqrt(Dist2((APawn*)bot, c.slamFoe)), bot->Location.Z - c.slamZ0, c.slamN, c.slamCool,
            conc->m_FireMode.Data[mi]->GetDamageRadius());
        aic->Enemy = c.slamFoe;
        c.slamFoe = nullptr;
        return false;
    }
    if (!c.foe || !c.lastSight || !PawnAlive(c.foe, (APawn*)bot)) { c.slamSkip = "nofoe"; return false; }
    if (c.cloaked || c.tellUntil > 0.0f) { c.slamSkip = "cloak"; return false; }
    if (now - c.slamAt < c.slamCool) { c.slamSkip = "cool"; return false; }
    if (c.nadeWind > 0.0f || now - c.nadeAt < 2.0f) { c.slamSkip = "nade"; return false; }
    const int knob = SlamKnob(now);
    if (knob == 0) { c.slamSkip = "knob0"; return false; }
    if (c.jetting || bot->Physics != 1 /*PHYS_Walking*/ || c.fuel < kSlamMinFuel) { c.slamSkip = "jet"; return false; }
    const float d2 = Dist2((APawn*)bot, c.foe);
    if (d2 > kSlamR * kSlamR) { c.slamSkip = "range"; return false; }
    ATgRepInfo_TaskForce* mine = TfOf((APawn*)bot);
    int nFoes = 0;
    for (APawn* q = bot->WorldInfo->PawnList; q; q = q->NextPawn) {
        if (q->bDeleteMe || q->Health <= 0 || !TfOf(q) || TfOf(q) == mine) continue;
        if (Dist2(q, (APawn*)bot) <= kSlamR * kSlamR) nFoes++;
    }
    const char* why = nullptr;
    if (nFoes >= 2) why = "clump";
    else if (knob >= 2 && d2 <= kSlamSingleR * kSlamSingleR && now - c.sightSince >= 0.5f) why = "close";
    if (!why) { c.slamSkip = "nowhy"; return false; }
    const FVector up(bot->Location.X, bot->Location.Y, bot->Location.Z + 260.0f);
    if (!PbClear((AActor*)bot, up, bot->Location)) { c.slamSkip = "ceiling"; c.slamAt = now; c.slamCool = 2.0f; return false; }
    c.slamSkip = "-";
    c.slamWhy = why; c.slamFoe = c.foe; c.slamWind = now + kSlamWind; c.slamZ0 = bot->Location.Z;
    c.jetting = true; c.slamJet = true; c.escapeJet = false; c.jetGlide = false; c.jetFor = 0.0f; c.jets++;
    bot->SetPhysics(2 /*PHYS_Falling*/);
    aic->Enemy = nullptr; aic->Focus = (AActor*)c.foe;
    Logger::Log("chat-command", "[PvpBot] ab slam windup pawn=%d why=%s foes=%d dist=%.0f fuel=%.1f knob=%d\n",
        c.pawnId, why, nFoes, std::sqrt(d2), c.fuel, knob);
    return true;
}

// Recon role: Sprint Stealth (2209) is an in-hand continuous device
// (asm mode 2721, slot 981) - the player holds it while sneaking and swaps
// to a weapon to fight. Bot: while chasing a foe farther than kCloakOff,
// drop Enemy (the donor behavior would keep the rifle firing and refuse the
// switch, same as the medic beam), switch to the stealth device and
// StartAction it; inside kCloakOff, on chase end, power out or after
// kCloakMax: stop it, back to the ranged weapon, Enemy = foe.
constexpr float kCloakOn   = 1300.0f;
constexpr float kCloakOff  = 750.0f;
constexpr float kCloakMax  = 12.0f;
constexpr float kCloakCool = 5.0f;
// Cloak tactics: cloak from closer when the foe faces away (flank),
// while retreating on low HP (escape), and uncloak kCloakTell s before the
// first shot (the visible tell). cs_settings pvpbot_ab_cloak: 0 = bots never
// cloak, 1 = range rule only, 2 = tactics (default).
constexpr float kCloakFlankOn = 900.0f;
constexpr float kCloakEscapeR = 2500.0f;   // escape cloak only with a foe this close
constexpr float kCloakTell    = 0.5f;

int CloakKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_cloak'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] cloak knob pvpbot_ab_cloak=%d\n", nv);
        v = nv;
    }
    return v;
}

// true when the foe looks away from the bot (bot outside its forward ~100 deg)
bool FoeFacingAway(APawn* foe, APawn* bot) {
    const float yaw = (float)foe->Rotation.Yaw * 6.2831853f / 65536.0f;
    const float dx = bot->Location.X - foe->Location.X, dy = bot->Location.Y - foe->Location.Y;
    const float l = std::sqrt(dx * dx + dy * dy);
    if (l < 1.0f) return false;
    return (std::cos(yaw) * dx + std::sin(yaw) * dy) / l < -0.2f;
}

// The device route never fires on a bot (switch refused, StartAction
// ok=0, canStart/canFire 0). A
// player's stealth is just the device fire mode's category-621 effect group
// (1 s life, refreshed while the fire button is held - UObject__ProcessEvent
// RefireCheckTimer). So the bot gets that same group straight through
// ProcessEffect, keeps its rifle in hand (Enemy cleared = no fire), and we
// re-arm the group's LifeDone timer each tick while it closes in.
constexpr int kStealthCategory = 621;

UTgEffectGroup* StealthTemplate(ATgDevice* spec) {
    for (int m = 0; spec && m < spec->m_FireMode.Count; m++) {
        UTgDeviceFire* fm = spec->m_FireMode.Data[m];
        if (!fm) continue;
        if (fm->s_EffectGroupList.Count == 0) {
            int idx = 0;
            TgDeviceFire__GetEffectGroup::Call(fm, nullptr, -1, &idx);
        }
        for (int i = 0; i < fm->s_EffectGroupList.Count; i++) {
            UTgEffectGroup* g = fm->s_EffectGroupList.Data[i];
            if (g && g->m_nCategoryCode == kStealthCategory) return g;
        }
    }
    return nullptr;
}

int RefreshCloak(ATgPawn* bot) {
    ATgEffectManager* mgr = bot->r_EffectManager;
    int n = 0;
    for (int i = 0; mgr && i < mgr->s_AppliedEffectGroups.Count; i++) {
        UTgEffectGroup* g = mgr->s_AppliedEffectGroups.Data[i];
        if (!g || g->m_nCategoryCode != kStealthCategory) continue;
        for (int t = 0; t < mgr->Timers.Count; t++)
            if (mgr->Timers.Data[t].TimerObj == (UObject*)g) mgr->Timers.Data[t].Count = 0.0f;
        n++;
    }
    return n;
}

void EndCloak(ATgAIController* aic, ATgPawn* bot, Chaser& c, const char* why) {
    const float now = aic->WorldInfo ? aic->WorldInfo->TimeSeconds : 0.0f;
    if (bot->r_EffectManager)
        TgEffectManager__RemoveEffectGroupsByCategory::Call(bot->r_EffectManager, nullptr, kStealthCategory, 4);
    const bool tell = CloakKnob(now) >= 2 && c.foe && PawnAlive(c.foe, (APawn*)bot);
    if (c.foe && PawnAlive(c.foe, (APawn*)bot)) {
        aic->Enemy = tell ? nullptr : c.foe;  // Face the foe, shoot after the tell
        aic->SetTargetActor((AActor*)c.foe);
        aic->Focus = (AActor*)c.foe;
    }
    c.tellUntil = tell ? now + kCloakTell : 0.0f;
    aic->m_bInterrupt = 1;
    Logger::Log("chat-command", "[PvpBot] cloak off pawn=%d why=%s escape=%d tell=%.1f held=%.1f stealthed=%d dist=%.0f power=%.0f/%.0f used=%.0f\n",
        c.pawnId, why, (int)c.cloakEscape, tell ? kCloakTell : 0.0f, now - c.cloakAt, (int)bot->r_bIsStealthed,
        c.foe ? std::sqrt(Dist2((APawn*)bot, c.foe)) : -1.0f,
        bot->r_fCurrentPowerPool, bot->r_fMaxPowerPool, c.cloakPowerUsed);
    c.cloaked = false;
    c.cloakEscape = false;
    c.cloakAt = now;
}

void TickStealth(ATgAIController* aic, ATgPawn* bot, Chaser& c, bool chasing, bool escaping = false) {
    if (!c.recon || !aic->WorldInfo) return;
    ATgDevice* spec = bot->m_EquippedDevices[3];
    if (!spec) return;
    const float now = aic->WorldInfo->TimeSeconds;
    const bool foeUp = c.foe && PawnAlive(c.foe, (APawn*)bot);
    const float d = foeUp ? std::sqrt(Dist2((APawn*)bot, c.foe)) : 0.0f;
    const int knob = CloakKnob(now);
    if (!c.cloaked && c.tellUntil > 0.0f) {  // Tell: uncloaked, aiming, not firing yet
        if (!foeUp || escaping) { c.tellUntil = 0.0f; }
        else if (now < c.tellUntil) { aic->Enemy = nullptr; aic->Focus = (AActor*)c.foe; return; }
        else {
            c.tellUntil = 0.0f;
            aic->Enemy = c.foe;
            aic->SetTargetActor((AActor*)c.foe);
            aic->Focus = (AActor*)c.foe;
            aic->m_bInterrupt = 1;
            Logger::Log("chat-command", "[PvpBot] cloak tell done pawn=%d dist=%.0f -> fire\n", c.pawnId, d);
        }
    }
    // Sprint Stealth costs prop 242 power every refire (prop 53, 0.5 s)
    // while held, like the player's continuous device; out of power = uncloak.
    const float cost = DeviceProp(spec, kPropPowerCost);
    const float refire = (std::max)(0.1f, DeviceProp(spec, kPropRefire) > 0.0f ? DeviceProp(spec, kPropRefire) : 0.5f);
    const bool hasPool = bot->r_fMaxPowerPool > 0.0f;
    if (c.cloaked) {
        if (hasPool && cost > 0.0f) {
            const float dt = (std::min)(1.0f, (std::max)(0.0f, now - c.cloakDrainAt));
            const float cur = (std::min)(bot->r_fCurrentPowerPool, c.cloakPool);  // drop regen gained while held
            const float use = (std::min)(cur, cost * dt / refire);
            bot->r_fCurrentPowerPool = cur - use;
            c.cloakPool = bot->r_fCurrentPowerPool;
            c.cloakPowerUsed += use;
            c.cloakDrainAt = now;
        }
        const int live = RefreshCloak(bot);
        const bool powerOut = hasPool && cost > 0.0f && bot->r_fCurrentPowerPool < cost;
        const char* why = c.cloakEscape
                        ? (!escaping ? "escape end" : powerOut ? "power out" : now - c.cloakAt > kCloakMax ? "max time"
                           : live == 0 ? "effect gone" : nullptr)
                        : knob == 0 ? "knob off" : escaping ? "retreat"
                        : !foeUp ? "foe down" : !chasing ? "chase end" : d < kCloakOff ? "close"
                        : powerOut ? "power out"
                        : now - c.cloakAt > kCloakMax ? "max time" : live == 0 ? "effect gone" : nullptr;
        if (why) { EndCloak(aic, bot, c, why); return; }
        aic->Enemy = nullptr;  // keep the behavior off the rifle (firing would give it away)
        return;
    }
    if (knob == 0 || !foeUp || now - c.cloakAt < kCloakCool) return;
    // Why: escape (retreating, foe near), flank (foe looks away), range (far chase)
    const char* cwhy = nullptr;
    if (escaping) { if (knob >= 2 && d < kCloakEscapeR) cwhy = "escape"; }
    else if (chasing && d >= kCloakOn) cwhy = "range";
    else if (chasing && knob >= 2 && d >= kCloakFlankOn && FoeFacingAway(c.foe, (APawn*)bot)) cwhy = "flank";
    if (!cwhy) return;
    // Same gates a player has: no stealth during setup-time immunity, and not
    // while in combat if the mode says m_bRestrictInCombat.
    UTgDeviceFire* fm0 = spec->m_FireMode.Count > 0 ? spec->m_FireMode.Data[0] : nullptr;
    const bool restrict = fm0 && fm0->m_bRestrictInCombat;
    const bool inCombat = bot->InCombat();
    const bool invuln = bot->r_EffectManager && bot->r_EffectManager->IsInvulnerable();
    UTgEffectGroup* tmpl = StealthTemplate(spec);
    const bool lowPower = hasPool && cost > 0.0f && bot->r_fCurrentPowerPool < cost * 4.0f;  // >= 2 s of stealth
    if ((restrict && inCombat) || invuln || !tmpl || !bot->r_EffectManager || lowPower) {
        if (now - c.cloakWaitLog > 3.0f) {
            c.cloakWaitLog = now;
            Logger::Log("chat-command", "[PvpBot] cloak wait pawn=%d restrict=%d combat=%d invuln=%d tmpl=%d dist=%.0f power=%.0f cost=%.1f\n",
                c.pawnId, (int)restrict, (int)inCombat, (int)invuln, (int)(tmpl != nullptr), d, bot->r_fCurrentPowerPool, cost);
        }
        if (!tmpl) c.cloakAt = now + 30.0f;  // no stealth group on this device: stop asking
        return;
    }
    UTgEffectGroup* g = BuildEffectGroup(tmpl->m_nEffectGroupId, tmpl->m_nType);
    if (g) {
        FImpactInfo impact{};
        bot->r_EffectManager->eventProcessEffect(g, 0, nullptr, (AActor*)bot, impact);
    }
    ATgDevice* cur = (ATgDevice*)bot->Weapon;
    if (cur) { bot->StopAction(cur, 0); bot->StopAction(cur, 1); }
    aic->StopDevices();
    aic->Enemy = nullptr;
    const int live = RefreshCloak(bot);
    c.cloakAt = now;
    c.cloakDrainAt = now;
    c.cloakPowerUsed = 0.0f;
    c.cloakPool = bot->r_fCurrentPowerPool;
    if (live > 0) { c.cloaked = true; c.cloakEscape = escaping; c.cloaks++; } else { c.cloakFail++; }
    Logger::Log("chat-command", "[PvpBot] cloak on pawn=%d why=%s eg=%d type=%d built=%d live=%d stealthed=%d dist=%.0f n=%d fail=%d power=%.0f/%.0f cost=%.1f refire=%.2f\n",
        c.pawnId, cwhy, tmpl->m_nEffectGroupId, tmpl->m_nType, (int)(g != nullptr), live, (int)bot->r_bIsStealthed,
        d, c.cloaks, c.cloakFail, bot->r_fCurrentPowerPool, bot->r_fMaxPowerPool, cost, refire);
}

bool WantsChase(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    const float dx = bot->Location.X - c.foe->Location.X;
    const float dy = bot->Location.Y - c.foe->Location.Y;
    const float dz = bot->Location.Z - c.foe->Location.Z;
    const float d2 = dx * dx + dy * dy + dz * dz;
    FVector eye = bot->Location;
    eye.Z += bot->BaseEyeHeight;
    // The AI stamps its own sight memory: m_fTargetLastSeenTime (sight
    // events), m_fTargetLastClearLOSTime (native TargetInLOS during fire
    // maintenance) and m_fLastTargetHitTime (it hit the foe = it sees it).
    // Freshest stamp wins; only with no stamp fall back to LineOfSightTo
    // (alternates test points per call via bLOSflag - ask twice).
    float age = 999.0f;
    if (aic->WorldInfo && aic->Enemy == c.foe) {
        const float now = aic->WorldInfo->TimeSeconds;
        const float stamps[3] = { aic->m_fTargetLastSeenTime,
            aic->m_fTargetLastClearLOSTime, aic->m_fLastTargetHitTime };
        for (float t : stamps)
            if (t > 0.0f && now - t < age) age = now - t;
    }
    const bool losNow = aic->LineOfSightTo((AActor*)c.foe, eye, 0) ||
                        aic->LineOfSightTo((AActor*)c.foe, eye, 0);
    const bool sight = age < (c.scoped ? 5.0f : 2.5f) || losNow;  // Scoped sniper waits out a flicker
    const float seenAge = age;
    c.lastSight = sight;
    // Remember a FRESH sight position (the stamps lag up to 2.5 s)
    if (losNow || age < 0.6f) { c.sightSpot = bot->Location; c.hasSightSpot = true; }
    bool chase = c.chasing;
    const float cFar = c.sniper ? kSnipeFar : kChaseFar;
    const float cNear = c.sniper ? kSnipeNear : kChaseNear;
    if ((!sight && !c.coverOn ) || d2 > cFar * cFar) chase = true;
    else if (d2 < cNear * cNear) chase = false;
    if (chase != c.chasing) {
        Logger::Log("chat-command", "[PvpBot] chase pawn=%d %s dist=%.0f sight=%d seenAge=%.1f\n",
            c.pawnId, chase ? "on" : "off", std::sqrt(d2), (int)sight, seenAge);
        c.chasing = chase;
    }
    return chase;
}

// world-only clear line (no actors). Actor.FastTrace through the SDK
// wrapper returns false for EVERY call here (run walkable neighbour
// and 200 uu straight up both 'blocked'), so use Actor.Trace like WorldGroups.
bool PbClear(AActor* t, const FVector& end, const FVector& start) {
    if (!t) return false;
    FVector hl, hn; FTraceHitInfo hi; std::memset(&hi, 0, sizeof(hi));
    return !t->Trace(end, start, 0, FVector(0, 0, 0), 0, &hl, &hn, &hi);
}

// Nearest path node to `at` that is linked into the path net (has
// reach specs) on network `net` (-1 = any); prefers one with a clear trace.
ANavigationPoint* NearestLinkedNav(AActor* tracer, const FVector& at, int net,
                                   float* outDist, bool* outVis, int* outScanned, float maxD = 2000.0f,
                                   const std::vector<ANavigationPoint*>* skip = nullptr) {
    ANavigationPoint* best = nullptr; float bestD = 1.0e18f;
    ANavigationPoint* bestVis = nullptr; float bestVisD = 1.0e18f;
    const FVector zero = {0, 0, 0};
    int n = 0;
    for (ANavigationPoint* nav = tracer->WorldInfo->NavigationPointList; nav && n < 20000;
         nav = nav->nextNavigationPoint, n++) {
        if (nav->bDeleteMe || nav->PathList.Num() == 0) continue;
        if (net >= 0 && nav->NetworkID != net) continue;
        if (skip && std::find(skip->begin(), skip->end(), nav) != skip->end()) continue;
        const float dx = nav->Location.X - at.X, dy = nav->Location.Y - at.Y, dz = nav->Location.Z - at.Z;
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 > maxD * maxD) continue;
        if (d2 < bestD) { bestD = d2; best = nav; }
        if (d2 < bestVisD && PbClear((AActor*)tracer, nav->Location, at)) { bestVisD = d2; bestVis = nav; }
    }
    ANavigationPoint* pick = bestVis ? bestVis : best;
    if (outDist) *outDist = pick ? std::sqrt(pick == bestVis ? bestVisD : bestD) : -1.0f;
    if (outVis) *outVis = pick && pick == bestVis;
    if (outScanned) *outScanned = n;
    return pick;
}

// The engine's FindPathToward/FindPathTo return nothing for these bots
// even with a linked anchor on the same network, so plan
// over the level's reach specs ourselves: Dijkstra on spec Distance, skipping
// blocked nodes and specs narrower/lower than the bot's cylinder.
constexpr float kJetMinRise = 110.0f;   // waypoint this much above the feet -> jet
constexpr float kJetMax2D   = 1500.0f;  // only toward a waypoint this close (2D)
constexpr float kJetSpeed   = 600.0f;   // uu/s while thrusting
constexpr float kJetFuel    = 4.0f;     // seconds of thrust when full
constexpr float kJetRefill  = 0.8f;     // fuel/s back while walking
constexpr float kJetMinFuel = 1.5f;
constexpr float kEscapeFor  = 1.6f;     // S of thrust per air escape
constexpr float kEscapeH    = 430.0f;   // uu/s sideways/away
constexpr float kEscapeUp   = 420.0f;   // uu/s climb (first 0.8 s)     // need this much to take off
bool IsAirNode(AActor* a);
constexpr int kAirCostMul = 3;  // Air (jet-only) links cost 3x -> ground routes win when they exist
struct PlanStats { int expanded = 0, nullEnd = 0, sizeSkip = 0, blocked = 0, partial = 0, jumps = 0; };

// JUMP LINKS. Some maps split the walkable path net into separate
// networks joined only by jetpack flights (Rot_Trafalgar_P: spawn decks vs
// the koth platforms). A jump link = ground node -> ground node of
// ANOTHER network, clear trace, within one tank of thrust (plus the glide a
// drop gives). Built once per node per level, used only when the plain plan
// cannot reach the goal; cost = 6x distance + 500 so real paths win.
constexpr float kJumpFuelUse  = 0.8f;    // plan with 80 % of a full tank
constexpr float kJumpMaxRise  = 500.0f;
constexpr float kJumpMaxDrop  = 2500.0f;
constexpr float kFallG        = 1040.0f; // pawn gravity ~2x WorldGravityZ
constexpr size_t kJumpPerNode = 16;  // cached per node; PlanRoute keeps only targets the plain plan could not reach
bool IsAirNode(AActor* a);
struct JumpCache {
    AWorldInfo* wi = nullptr; ANavigationPoint* head = nullptr;
    std::vector<ANavigationPoint*> nodes;
    std::map<ANavigationPoint*, std::vector<std::pair<ANavigationPoint*, int>>> links;
};
// Ledges any bot failed to jet onto (2 misses), kept for the level so
// respawned/other bots don't repeat the same fuel-outs.
struct BadJetMem { AWorldInfo* wi = nullptr; ANavigationPoint* head = nullptr; std::vector<ANavigationPoint*> nodes; };
BadJetMem g_badJet;
std::vector<ANavigationPoint*>& BadJetFor(AActor* a) {
    AWorldInfo* wi = a ? a->WorldInfo : nullptr;
    if (!wi || g_badJet.wi != wi || g_badJet.head != wi->NavigationPointList) {
        g_badJet = BadJetMem(); g_badJet.wi = wi; g_badJet.head = wi ? wi->NavigationPointList : nullptr;
    }
    return g_badJet.nodes;
}
JumpCache g_jump;
const std::vector<std::pair<ANavigationPoint*, int>>& JumpLinks(AActor* tracer, ANavigationPoint* n) {
    AWorldInfo* wi = tracer->WorldInfo;
    if (g_jump.wi != wi || g_jump.head != wi->NavigationPointList) {
        g_jump = JumpCache();
        g_jump.wi = wi; g_jump.head = wi->NavigationPointList;
        int k = 0;
        for (ANavigationPoint* nav = wi->NavigationPointList; nav && k < 20000; nav = nav->nextNavigationPoint, k++)
            if (!nav->bDeleteMe && nav->PathList.Num() > 0 && !IsAirNode((AActor*)nav)) g_jump.nodes.push_back(nav);
    }
    auto f = g_jump.links.find(n);
    if (f != g_jump.links.end()) return f->second;
    std::vector<std::pair<float, ANavigationPoint*>> cand;
    const float budget = kJetFuel * kJetSpeed * kJumpFuelUse;
    if (!IsAirNode((AActor*)n)) {
        for (ANavigationPoint* e : g_jump.nodes) {
            if (e == n || e->bDeleteMe || e->bBlocked) continue;
            // NetworkID is no reachability proof (Trafalgar: deck + main area both net 60, deck reaches 8 of 282)
            bool linked = false;
            for (int i = 0; i < n->PathList.Num() && !linked; i++)
                if (n->PathList(i) && n->PathList(i)->End.Actor == (AActor*)e) linked = true;
            if (linked) continue;
            const float dx = e->Location.X - n->Location.X, dy = e->Location.Y - n->Location.Y;
            const float dz = e->Location.Z - n->Location.Z;
            if (dz > kJumpMaxRise || dz < -kJumpMaxDrop) continue;
            const float h = std::sqrt(dx * dx + dy * dy);
            const float glide = dz < 0.0f ? kJetSpeed * std::sqrt(2.0f * -dz / kFallG) * kJumpFuelUse : 0.0f;
            const float fly = std::sqrt(h * h + (dz > 0.0f ? dz * dz : 0.0f));
            if (fly > budget + glide || h < 200.0f) continue;
            cand.push_back(std::make_pair(fly, e));
        }
        std::sort(cand.begin(), cand.end(),
                  [](const std::pair<float, ANavigationPoint*>& a, const std::pair<float, ANavigationPoint*>& b) { return a.first < b.first; });
    }
    std::vector<std::pair<ANavigationPoint*, int>>& out = g_jump.links[n];
    const FVector zero = {0, 0, 0};
    int traces = 0;
    for (auto& c : cand) {
        if (out.size() >= kJumpPerNode || traces >= 80) break;
        FVector a = n->Location, b = c.second->Location;
        a.Z += 60.0f; b.Z += 60.0f;
        traces++;
        if (!PbClear((AActor*)tracer, b, a)) continue;
        out.push_back(std::make_pair(c.second, (int)(c.first * 2.0f) * kAirCostMul + 500));
    }
    // Diag: nearest other-node per node (geometry reject vs trace reject)
    if (g_jump.links.size() <= 40) {
        float nearH = 1e9f, nearDz = 0; int inBox = 0;
        for (ANavigationPoint* e : g_jump.nodes) {
            if (e == n) continue;
            const float dx = e->Location.X - n->Location.X, dy = e->Location.Y - n->Location.Y;
            const float h = std::sqrt(dx * dx + dy * dy);
            if (h < 4000.0f) inBox++;
            if (h > 200.0f && h < nearH) { nearH = h; nearDz = e->Location.Z - n->Location.Z; }
        }
        // control: trace to a spec-linked neighbour (walkable -> should be clear), both arg orders
        int nb1 = -1, nb2 = -1, up = -1;
        AActor* nbA = n->PathList.Num() > 0 && n->PathList(0) ? n->PathList(0)->End.Actor : nullptr;
        if (nbA) {
            FVector a = n->Location, b = nbA->Location; a.Z += 60.0f; b.Z += 60.0f;
            nb1 = (int)PbClear((AActor*)tracer, b, a);
            nb2 = (int)PbClear((AActor*)tracer, a, b);
            FVector c = a; c.Z += 200.0f;
            up = (int)PbClear((AActor*)tracer, c, a);
        }
        Logger::Log("chat-command", "[PvpBot] jumplinks node=(%.0f,%.0f,%.0f) net=%d cand=%d traces=%d ok=%d within4k=%d nearest h=%.0f dz=%.0f nbTrace=%d/%d up=%d\n",
            n->Location.X, n->Location.Y, n->Location.Z, n->NetworkID, (int)cand.size(), traces, (int)out.size(), inBox, nearH, nearDz, nb1, nb2, up);
    }
    return out;
}

// Once per level, when a route stays partial: what the path networks
// look like (count + box per NetworkID) and every non-plain node class
// (jump pads, teleporters, lifts) - shows how players cross between nets.
void NavNetDump(AActor* tracer) {
    static void* s_head = nullptr;
    AWorldInfo* wi = tracer->WorldInfo;
    if (!wi || s_head == (void*)wi->NavigationPointList) return;
    s_head = (void*)wi->NavigationPointList;
    struct Box { int n = 0; float x0 = 1e9f, y0 = 1e9f, z0 = 1e9f, x1 = -1e9f, y1 = -1e9f, z1 = -1e9f; };
    std::map<int, Box> nets;
    std::map<std::string, int> cls;
    int k = 0, odd = 0;
    for (ANavigationPoint* nav = wi->NavigationPointList; nav && k < 20000; nav = nav->nextNavigationPoint, k++) {
        if (nav->bDeleteMe) continue;
        Box& b = nets[nav->NetworkID]; b.n++;
        b.x0 = std::min(b.x0, nav->Location.X); b.x1 = std::max(b.x1, nav->Location.X);
        b.y0 = std::min(b.y0, nav->Location.Y); b.y1 = std::max(b.y1, nav->Location.Y);
        b.z0 = std::min(b.z0, nav->Location.Z); b.z1 = std::max(b.z1, nav->Location.Z);
        const std::string c = nav->Class ? nav->Class->GetName() : "?";
        cls[c]++;
        if (c != "PathNode" && c != "VolumePathNode" && c != "TgTeamPlayerStart" && c != "PlayerStart" && odd < 60) {
            odd++;
            Logger::Log("chat-command", "[PvpBot] navdump node cls=%s net=%d links=%d loc=(%.0f,%.0f,%.0f)\n",
                c.c_str(), nav->NetworkID, nav->PathList.Num(), nav->Location.X, nav->Location.Y, nav->Location.Z);
        }
    }
    for (auto& kv : nets)
        Logger::Log("chat-command", "[PvpBot] navdump net=%d n=%d x=%.0f..%.0f y=%.0f..%.0f z=%.0f..%.0f\n", kv.first, kv.second.n,
            kv.second.x0, kv.second.x1, kv.second.y0, kv.second.y1, kv.second.z0, kv.second.z1);
    for (auto& kv : cls) Logger::Log("chat-command", "[PvpBot] navdump cls=%s n=%d\n", kv.first.c_str(), kv.second);
}

bool PlanRoute(ATgPawn* bot, ANavigationPoint* start, ANavigationPoint* goal,
               std::vector<ANavigationPoint*>& out, int* outLen, PlanStats* st,
               const FVector* fallbackNear, const std::vector<ANavigationPoint*>* avoid = nullptr,
               std::vector<ANavigationPoint*>* jumpTo = nullptr,
               std::vector<ANavigationPoint*>* reachedOut = nullptr,
               const std::vector<ANavigationPoint*>* plainReached = nullptr) {
    out.clear();
    if (jumpTo) jumpTo->clear();
    std::map<ANavigationPoint*, bool> viaJump;
    if (!start || !goal) return false;
    const float rad = bot->CylinderComponent ? bot->CylinderComponent->CollisionRadius : 0.0f;
    const float hgt = bot->CylinderComponent ? bot->CylinderComponent->CollisionHeight : 0.0f;
    std::map<ANavigationPoint*, int> dist;
    std::map<ANavigationPoint*, ANavigationPoint*> prev;
    typedef std::pair<int, ANavigationPoint*> QE;
    std::priority_queue<QE, std::vector<QE>, std::greater<QE>> pq;
    dist[start] = 0; pq.push(QE(0, start));
    int expanded = 0;
    while (!pq.empty() && expanded < 20000) {
        QE top = pq.top(); pq.pop();
        ANavigationPoint* n = top.second;
        if (top.first != dist[n]) continue;
        if (n == goal) break;
        expanded++;
        for (int i = 0; i < n->PathList.Num(); i++) {
            UReachSpec* sp = n->PathList(i);
            if (!sp) continue;
            ANavigationPoint* e = (ANavigationPoint*)sp->End.Actor;
            if (!e) { if (st) st->nullEnd++; continue; }
            if (e->bDeleteMe || e->bBlocked) { if (st) st->blocked++; continue; }
            if (avoid && e != goal && std::find(avoid->begin(), avoid->end(), e) != avoid->end()) {
                if (st) st->blocked++; continue;
            }
            if ((float)sp->CollisionRadius < rad || (float)sp->CollisionHeight < hgt) { if (st) st->sizeSkip++; continue; }
            int cost = sp->Distance > 0 ? sp->Distance : 1;
            if (IsAirNode((AActor*)e) || IsAirNode((AActor*)n)) cost *= kAirCostMul;
            const int nd = top.first + cost;
            auto f = dist.find(e);
            if (f == dist.end() || nd < f->second) { dist[e] = nd; prev[e] = n; viaJump[e] = false; pq.push(QE(nd, e)); }
        }
        if (jumpTo) {
            for (auto& jl : JumpLinks((AActor*)bot, n)) {
                ANavigationPoint* e = jl.first;
                if (plainReached && std::binary_search(plainReached->begin(), plainReached->end(), e)) continue;
                if (avoid && e != goal && std::find(avoid->begin(), avoid->end(), e) != avoid->end()) continue;
                const int nd = top.first + jl.second;
                auto f = dist.find(e);
                if (f == dist.end() || nd < f->second) { dist[e] = nd; prev[e] = n; viaJump[e] = true; pq.push(QE(nd, e)); }
            }
        }
    }
    if (st) st->expanded = expanded;
    if (reachedOut) {  // Sorted, for the jump pass
        reachedOut->clear();
        for (auto& kv : dist) reachedOut->push_back(kv.first);
        std::sort(reachedOut->begin(), reachedOut->end());
    }
    if (!dist.count(goal)) {
        // The goal can sit on an island the bot's side cannot reach
        // (one-way drops / jet-only links: only part of the
        // nodes are reachable). Head for the reached node nearest the foe instead.
        if (!fallbackNear || dist.empty()) return false;
        ANavigationPoint* bestN = nullptr; float bestD = 1.0e18f;
        for (auto& kv : dist) {
            const FVector& l = kv.first->Location;
            const float dx = l.X - fallbackNear->X, dy = l.Y - fallbackNear->Y, dz = l.Z - fallbackNear->Z;
            const float d2 = dx * dx + dy * dy + dz * dz;
            if (d2 < bestD) { bestD = d2; bestN = kv.first; }
        }
        if (!bestN || bestN == start) return false;
        if (st) st->partial = (int)std::sqrt(bestD);
        goal = bestN;
    }
    for (ANavigationPoint* n = goal; n; ) {
        out.push_back(n);
        if (n == start) break;
        auto f = prev.find(n);
        n = (f == prev.end()) ? nullptr : f->second;
    }
    std::reverse(out.begin(), out.end());
    if (jumpTo)
        for (size_t i = 1; i < out.size(); i++)
            if (viaJump[out[i]]) { jumpTo->push_back(out[i]); if (st) st->jumps++; }
    if (outLen) *outLen = dist[goal];
    return !out.empty() && out.front() == start;
}

// Next node of our own route toward `goal` (replans every 5 s or on a new
// goal); nullptr = no route.
AActor* OwnRouteNext(ATgAIController* aic, ATgPawn* bot, Chaser& c, ANavigationPoint* goal, const FVector* nearAt = nullptr) {
    const float now = bot->WorldInfo ? bot->WorldInfo->TimeSeconds : 0.0f;
    if (goal != c.routeGoal || now - c.routeAt > 5.0f) {
        const int net = goal ? goal->NetworkID : -1;
        // Avoided nodes (own + level-wide bad ledges) can't be the start
        // either.
        for (ANavigationPoint* b : BadJetFor((AActor*)bot))
            if (std::find(c.avoid.begin(), c.avoid.end(), b) == c.avoid.end()) c.avoid.push_back(b);
        ANavigationPoint* start = NearestLinkedNav((AActor*)bot, bot->Location, net, nullptr, nullptr, nullptr, 2000.0f, &c.avoid);
        if (!start && net >= 0) {
            // Push_Dust_P defender start has no linked node of
            // the goal's network within 2000 uu -> start=none, bot froze 50 s.
            // Plan from the nearest node of any network (partial route if the
            // nets don't join) instead of standing still.
            float d = -1.0f;
            start = NearestLinkedNav((AActor*)bot, bot->Location, -1, &d, nullptr, nullptr, 2000.0f, &c.avoid);
            Logger::Log("chat-command", "[PvpBot] ownroute anynet pawn=%d goalNet=%d startNet=%d d=%.0f\n",
                c.pawnId, net, start ? start->NetworkID : -1, d);
        }
        int len = 0;
        PlanStats st;
        // partial-route fallback: the reached node nearest nearAt (default the foe; retreat: home)
        const FVector nearFoe = nearAt ? *nearAt : (c.foe ? c.foe->Location : goal->Location);
        std::vector<ANavigationPoint*> plainReached;
        bool ok = PlanRoute(bot, start, goal, c.route, &len, &st, &nearFoe, &c.avoid, nullptr, &plainReached);
        c.jumpTo.clear();
        if (start && (!ok || st.partial > 0)) {
            // Goal on another network -> retry with jump links
            std::vector<ANavigationPoint*> r2, j2; int len2 = 0; PlanStats st2;
            const bool ok2 = PlanRoute(bot, start, goal, r2, &len2, &st2, &nearFoe, &c.avoid, &j2, nullptr, &plainReached);
            if (ok2 && (st2.partial == 0 || !ok || st2.partial < st.partial)) {
                Logger::Log("chat-command", "[PvpBot] ownroute jump pawn=%d nodes=%d jumps=%d partialGap=%d (plain %d)\n",
                    c.pawnId, (int)r2.size(), st2.jumps, st2.partial, ok ? st.partial : -1);
                c.route.swap(r2); c.jumpTo.swap(j2); len = len2; ok = ok2; st = st2;
            } else {
                Logger::Log("chat-command", "[PvpBot] ownroute nojump pawn=%d ok=%d reached=%d jumps=%d partialGap=%d\n",
                    c.pawnId, (int)ok2, st2.expanded, st2.jumps, st2.partial);
            }
            if (st.partial > 3000) NavNetDump((AActor*)bot);
        }
        c.routeGoal = goal; c.routeAt = now; c.routeIdx = 0;
        if (ok && c.route.size() >= 2) {
            // Already past the first node (it lies behind us)? start at the 2nd.
            const FVector& a = c.route[0]->Location; const FVector& b = c.route[1]->Location;
            const float ab = (a.X - b.X) * (a.X - b.X) + (a.Y - b.Y) * (a.Y - b.Y);
            const float pb = (bot->Location.X - b.X) * (bot->Location.X - b.X) +
                             (bot->Location.Y - b.Y) * (bot->Location.Y - b.Y);
            if (pb < ab) c.routeIdx = 1;
        }
        Logger::Log("chat-command",
            "[PvpBot] ownroute pawn=%d ok=%d nodes=%d len=%d start=%s reached=%d nullEnd=%d sizeSkip=%d blocked=%d partialGap=%d\n",
            c.pawnId, (int)ok, (int)c.route.size(), len, start ? "ok" : "none",
            st.expanded, st.nullEnd, st.sizeSkip, st.blocked, st.partial);
        if (!ok) { c.route.clear(); return nullptr; }
    }
    if (c.route.empty()) return nullptr;
    while (c.routeIdx < c.route.size()) {
        const FVector& w = c.route[c.routeIdx]->Location;
        const float dx = w.X - bot->Location.X, dy = w.Y - bot->Location.Y;
        if (dx * dx + dy * dy > 150.0f * 150.0f) break;
        c.routeIdx++;
    }
    if (c.routeIdx >= c.route.size()) return nullptr;
    return (AActor*)c.route[c.routeIdx];
}

// Nearest path node to the foe (prefer one with a clear trace to the
// foe's eyes-level), cached until the foe moves 300 uu. Walks the level's
// NavigationPointList; ~hundreds of nodes, only called while path-chasing.
ANavigationPoint* FoeNav(Chaser& c, ATgPawn* bot) {
    if (!c.foe || !c.foe->WorldInfo) return nullptr;
    const FVector f = c.foe->Location;
    if (c.foeNav) {
        const float dx = f.X - c.foeNavAt.X, dy = f.Y - c.foeNavAt.Y, dz = f.Z - c.foeNavAt.Z;
        if (dx * dx + dy * dy + dz * dz < 300.0f * 300.0f) return c.foeNav;
    }
    // only nodes linked into the path net (player starts often have no reach
    // specs) on the bot's network
    const int net = (bot && bot->Anchor) ? bot->Anchor->NetworkID : -1;
    float d = 0; bool vis = false; int n = 0;
    ANavigationPoint* pick = NearestLinkedNav((AActor*)c.foe, f, net, &d, &vis, &n);
    if (pick != c.foeNav) {
        const std::string nm = pick ? pick->GetName() : "none";
        Logger::Log("chat-command", "[PvpBot] foenav pawn=%d nav=%s dist=%.0f vis=%d scanned=%d net=%d/%d\n",
            c.pawnId, nm.c_str(), d, (int)vis, n, pick ? pick->NetworkID : -1, net);
    }
    c.foeNav = pick;
    c.foeNavAt = f;
    return pick;
}

// Flank routes. Chasing a foe >= 1400 uu away: when a teammate bot
// already runs straight at the same foe, this bot (skill >= 0.25) goes round
// the side instead (pincer); a lone bot does so ~30 % of the time. Flank node
// = a linked path node ~60 % of the way, 600-1800 uu off the direct line,
// preferably out of the foe's sight; our own route to it, then the normal
// chase. Ends when the foe is < 900 uu, the node is reached, after 12 s, with
// no route or a new foe. (Run chase only happens with the foe seen or
// < 2000 uu - objective play otherwise - so 'far + unseen' never started one.)
constexpr float kFlankMinD = 1400.0f;
constexpr float kFlankFor  = 12.0f;
constexpr float kFlankGap  = 15.0f;
bool IsAirNode(AActor* a);
inline float FlankRand() { return (float)std::rand() / (float)RAND_MAX; }

ANavigationPoint* PickFlankNode(ATgPawn* bot, Chaser& c, int side, float* outOff, bool* outHidden) {
    const FVector b = bot->Location, f = c.foe->Location;
    float ux = f.X - b.X, uy = f.Y - b.Y;
    const float D = std::sqrt(ux * ux + uy * uy);
    if (D < 1.0f || !bot->WorldInfo) return nullptr;
    ux /= D; uy /= D;
    const float off = std::min(1800.0f, std::max(600.0f, 0.5f * D));
    if (outOff) *outOff = off;
    // side>0 = left of the bot->foe line (perp = (-uy, ux))
    const float px = b.X + ux * 0.6f * D - uy * side * off, py = b.Y + uy * 0.6f * D + ux * side * off;
    const float pz = b.Z * 0.4f + f.Z * 0.6f;
    const int net = bot->Anchor ? bot->Anchor->NetworkID : -1;
    std::vector<std::pair<float, ANavigationPoint*>> cand;
    int n = 0;
    for (ANavigationPoint* nav = bot->WorldInfo->NavigationPointList; nav && n < 20000; nav = nav->nextNavigationPoint, n++) {
        if (nav->bDeleteMe || nav->PathList.Num() == 0) continue;
        if (net >= 0 && nav->NetworkID != net) continue;
        if (IsAirNode((AActor*)nav)) continue;
        if (std::find(c.avoid.begin(), c.avoid.end(), nav) != c.avoid.end()) continue;
        const float rx = nav->Location.X - b.X, ry = nav->Location.Y - b.Y;
        const float along = rx * ux + ry * uy, lat = (-rx * uy + ry * ux) * side;
        if (along < 0.2f * D || along > 0.9f * D || lat < 0.5f * off) continue;  // must really be out to that side
        const float dx = nav->Location.X - px, dy = nav->Location.Y - py, dz = nav->Location.Z - pz;
        const float d2 = dx * dx + dy * dy + 4.0f * dz * dz;
        if (d2 < 1100.0f * 1100.0f) cand.push_back(std::make_pair(d2, nav));
    }
    if (cand.empty()) return nullptr;
    std::sort(cand.begin(), cand.end(), [](const std::pair<float, ANavigationPoint*>& x, const std::pair<float, ANavigationPoint*>& y) { return x.first < y.first; });
    FVector feye = f; feye.Z += c.foe->BaseEyeHeight;
    for (size_t i = 0; i < cand.size() && i < 8; i++) {
        FVector e = cand[i].second->Location; e.Z += 60.0f;
        if (!PbClear((AActor*)bot, e, feye)) { if (outHidden) *outHidden = true; return cand[i].second; }
    }
    if (outHidden) *outHidden = false;
    return cand[0].second;
}

// Next waypoint of an active flank (starts one when it fits); nullptr = run the normal chase.
AActor* FlankNext(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!bot->WorldInfo || !c.foe) return nullptr;
    const float now = bot->WorldInfo->TimeSeconds;
    const float fx = c.foe->Location.X - bot->Location.X, fy = c.foe->Location.Y - bot->Location.Y;
    const float d = std::sqrt(fx * fx + fy * fy);
    if (c.flankNode) {
        const char* why = nullptr;
        const float nx = c.flankNode->Location.X - bot->Location.X, ny = c.flankNode->Location.Y - bot->Location.Y;
        if (c.flankFoe != c.foe) why = "foe";
        else if (d < kChaseNear) why = "close";
        else if (nx * nx + ny * ny < 250.0f * 250.0f) why = "reached";
        else if (now - c.flankAt > kFlankFor) why = "timeout";
        AActor* next = nullptr;
        if (!why) {
            next = OwnRouteNext(aic, bot, c, c.flankNode, &c.flankNode->Location);
            if (!next) why = "noroute";
        }
        if (!why) { c.flankLastAt = now; return next; }
        Logger::Log("chat-command", "[PvpBot] flank end pawn=%d why=%s for=%.1f foeD=%.0f at=(%.0f,%.0f,%.0f)\n",
            c.pawnId, why, now - c.flankAt, d, bot->Location.X, bot->Location.Y, bot->Location.Z);
        c.flankNode = nullptr; c.flankFoe = nullptr; c.flankEndAt = now; c.routeGoal = nullptr;
        return nullptr;
    }
    if (c.sniper || c.backoff || d < kFlankMinD) return nullptr;
    if (now - c.flankEndAt < kFlankGap || now - c.flankTryAt < 4.0f) return nullptr;
    if (c.skill >= 0.0f && c.skill < 0.25f) return nullptr;  // easy bots run straight in
    c.flankTryAt = now;
    ATgRepInfo_TaskForce* tf = TfOf(bot);
    int mates = 0, mateFlank = 0;
    for (auto& kv : g_chasers) {
        if (&kv.second == &c || kv.second.foe != c.foe || !kv.first) continue;
        APawn* mp = kv.first->Pawn;
        if (!mp || mp->Health <= 0 || TfOf(mp) != tf) continue;
        if (kv.second.flankNode) mateFlank++; else mates++;
    }
    const char* why = nullptr;
    if (mateFlank > 0) return nullptr;              // one flanker per foe is enough
    if (mates > 0) why = "pincer";
    else if (FlankRand() < 0.3f) why = "solo";
    if (!why) return nullptr;
    int side = FlankRand() < 0.5f ? 1 : -1;
    float off = 0; bool hidden = false;
    ANavigationPoint* fnode = PickFlankNode(bot, c, side, &off, &hidden);
    if (!fnode) { side = -side; fnode = PickFlankNode(bot, c, side, &off, &hidden); }
    if (!fnode) {
        Logger::Log("chat-command", "[PvpBot] flank skip pawn=%d why=nonode foeD=%.0f\n", c.pawnId, d);
        return nullptr;
    }
    c.flankNode = fnode; c.flankFoe = c.foe; c.flankAt = now; c.flankLastAt = now; c.flanks++; c.routeGoal = nullptr;
    const std::string nm = fnode->GetName();
    Logger::Log("chat-command",
        "[PvpBot] flank start pawn=%d n=%d why=%s side=%d foeD=%.0f off=%.0f node=%s (%.0f,%.0f,%.0f) hidden=%d sight=%d mates=%d at=(%.0f,%.0f,%.0f)\n",
        c.pawnId, c.flanks, why, side, d, off, nm.c_str(), fnode->Location.X, fnode->Location.Y, fnode->Location.Z,
        (int)hidden, (int)c.lastSight, mates, bot->Location.X, bot->Location.Y, bot->Location.Z);
    AActor* next = OwnRouteNext(aic, bot, c, fnode, &fnode->Location);
    if (!next) {
        Logger::Log("chat-command", "[PvpBot] flank end pawn=%d why=noroute for=0.0 foeD=%.0f at=(%.0f,%.0f,%.0f)\n",
            c.pawnId, d, bot->Location.X, bot->Location.Y, bot->Location.Z);
        c.flankNode = nullptr; c.flankFoe = nullptr; c.flankEndAt = now; c.routeGoal = nullptr;
    }
    return next;
}

// Where to run this pick: the foe itself when it is directly
// reachable, else the next waypoint of a path-network route toward it (a
// straight run gets stuck on ridges/walls, e.g. MissileComplex snow hills).
FVector ChaseTarget(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (c.backoff && c.hasSightSpot) return c.sightSpot;
    if (AActor* fl = FlankNext(aic, bot, c)) {  // Going round the side
        c.viaPath = true;
        c.lastWaypoint = fl;
        return fl->Location;
    }
    if (aic->ActorReachable((AActor*)c.foe)) {
        if (c.viaPath) {
            Logger::Log("chat-command", "[PvpBot] path pawn=%d direct again\n", c.pawnId);
            c.viaPath = false;
        }
        return c.foe->Location;
    }
    AActor* next = aic->FindPathToward((AActor*)c.foe, 0, 0, 1);
    if (!next || next == (AActor*)c.foe) {
        // A player foe often has no Anchor, so the native search finds
        // no goal from far away (foeNav=none). Pick the foe's nearest path
        // node ourselves and route to that instead.
        ANavigationPoint* nav = FoeNav(c, bot);
        next = nav ? aic->FindPathToward((AActor*)nav, 0, 0, 1) : nullptr;
        if (!next && nav) next = OwnRouteNext(aic, bot, c, nav);
        if (!next) {
            if (!c.noRoute) {
                Logger::Log("chat-command", "[PvpBot] path pawn=%d no route (foeNav=%s)\n",
                    c.pawnId, nav ? "ok" : "none");
                // Diag: why the route search fails (anchor links, spec
                // size vs our cylinder, location-based search).
                ANavigationPoint* an = bot->Anchor;
                UReachSpec* sp = (an && an->PathList.Num() > 0) ? an->PathList(0) : nullptr;
                AActor* byLoc = nav ? aic->FindPathTo(nav->Location, 0, 1) : nullptr;
                const std::string anName = an ? an->GetName() : "none";
                Logger::Log("chat-command",
                    "[PvpBot] route diag pawn=%d anchor=%s links=%d spec=%dx%d flags=%d cyl=%.0fx%.0f "
                    "foeNavLinks=%d byLoc=%d route=%d jump=%d walk=%d phys=%d\n",
                    c.pawnId, anName.c_str(), an ? (int)an->PathList.Num() : -1,
                    sp ? sp->CollisionRadius : -1, sp ? sp->CollisionHeight : -1, sp ? sp->reachFlags : -1,
                    bot->CylinderComponent ? bot->CylinderComponent->CollisionRadius : -1.0f,
                    bot->CylinderComponent ? bot->CylinderComponent->CollisionHeight : -1.0f,
                    nav ? (int)nav->PathList.Num() : -1, (int)(byLoc != nullptr), (int)aic->RouteCache.Num(),
                    (int)bot->bCanJump, (int)bot->bCanWalk, (int)bot->Physics);
                c.noRoute = true;
            }
            return c.foe->Location;
        }
    }
    c.noRoute = false;
    if (!c.viaPath || next != c.lastWaypoint) {
        const std::string nm = next->GetName();
        Logger::Log("chat-command",
            "[PvpBot] path pawn=%d via=%s (%.0f,%.0f,%.0f) route=%d bot=(%.0f,%.0f,%.0f)\n",
            c.pawnId, nm.c_str(), next->Location.X, next->Location.Y, next->Location.Z,
            (int)aic->RouteCache.Num(), bot->Location.X, bot->Location.Y, bot->Location.Z);
    }
    c.viaPath = true;
    c.lastWaypoint = next;
    return next->Location;
}

// Jetpack. Routes climb onto ledges and chain air nodes
// (VolumePathNode) that walking cannot reach (run stalled under one
// ~600 uu up). Players jet there; until the bot drives the real jetpack device
// the server flies the pawn: thrust toward the waypoint each frame while fuel
// lasts, then let it fall; fuel refills on the ground. A one-shot ballistic hop
// fell short (pawn gravity ~2x WorldGravityZ) and
// cannot follow air-node chains.
// Stop routing this bot through `n` (a ledge it keeps failing to jet
// onto, or a node it makes no progress toward); forces a replan.
void AvoidNode(ATgPawn* bot, Chaser& c, AActor* a, const char* why) {
    ANavigationPoint* n = (ANavigationPoint*)a;
    if (!n) return;
    if (std::find(c.avoid.begin(), c.avoid.end(), n) == c.avoid.end()) {
        if (c.avoid.size() >= 12) c.avoid.erase(c.avoid.begin());
        c.avoid.push_back(n);
    }
    c.routeAt = -100.0f; c.jetTo = nullptr; c.jetMiss = 0;
    if (std::strcmp(why, "jet miss") == 0) {  // Remember for the level
        std::vector<ANavigationPoint*>& bj = BadJetFor((AActor*)bot);
        if (std::find(bj.begin(), bj.end(), n) == bj.end()) { if (bj.size() >= 16) bj.erase(bj.begin()); bj.push_back(n); }
    }
    Logger::Log("chat-command", "[PvpBot] avoid pawn=%d node=(%.0f,%.0f,%.0f) why=%s n=%d bot=(%.0f,%.0f,%.0f)\n",
        c.pawnId, n->Location.X, n->Location.Y, n->Location.Z, why, (int)c.avoid.size(),
        bot->Location.X, bot->Location.Y, bot->Location.Z);
}


bool IsAirNode(AActor* a) {
    if (!a) return false;
    const char* n = a->GetName();
    return n && std::strstr(n, "VolumePathNode") != nullptr;
}

float FeetZ(ATgPawn* bot) {
    return bot->Location.Z - (bot->CylinderComponent ? bot->CylinderComponent->CollisionHeight : 42.0f);
}

// every 0.5 s while chasing: take off when the next waypoint is up
void MaybeStartJet(ATgPawn* bot, Chaser& c) {
    if (c.jetting || !c.viaPath || !c.lastWaypoint) return;
    if (c.fuel < kJetMinFuel) return;
    const FVector& w = c.lastWaypoint->Location;
    // Next hop is a jump link -> fly it (also from a fall off the edge)
    if ((bot->Physics == 1 || bot->Physics == 2) &&
        std::find(c.jumpTo.begin(), c.jumpTo.end(), (ANavigationPoint*)c.lastWaypoint) != c.jumpTo.end()) {
        const float jx = w.X - bot->Location.X, jy = w.Y - bot->Location.Y, jz = w.Z - FeetZ(bot);
        const float h = std::sqrt(jx * jx + jy * jy);
        if (h < 150.0f) return;
        const float need = std::sqrt(h * h + (jz > 0.0f ? jz * jz : 0.0f)) / kJetSpeed;
        if (bot->Physics == 1 && c.fuel < std::min(need, kJetFuel) - 0.05f) return;
        c.jetting = true; c.jetGlide = true; c.jetFor = 0.0f; c.jets++;
        bot->SetPhysics(2 /*PHYS_Falling*/);
        Logger::Log("chat-command", "[PvpBot] jet start pawn=%d jump to=(%.0f,%.0f,%.0f) dz=%.0f d2d=%.0f fuel=%.1f need=%.1f n=%d\n",
            c.pawnId, w.X, w.Y, w.Z, jz, h, c.fuel, need, c.jets);
        return;
    }
    if (bot->Physics != 1 /*PHYS_Walking*/) return;
    const float rise = w.Z - FeetZ(bot);
    const float dx = w.X - bot->Location.X, dy = w.Y - bot->Location.Y;
    const float d2d = std::sqrt(dx * dx + dy * dy);
    if (rise < kJetMinRise || d2d > kJetMax2D) return;
    // fuel budget: climb + the air-node chain that follows, at kJetSpeed;
    // short of it, refill first (full tank at most)
    float need = (rise + d2d) / kJetSpeed;
    FVector prevL = w;
    for (size_t i = c.routeIdx + 1; i < c.route.size() && IsAirNode((AActor*)c.route[i - 1]); i++) {
        const FVector& l = c.route[i]->Location;
        need += std::sqrt((l.X - prevL.X) * (l.X - prevL.X) + (l.Y - prevL.Y) * (l.Y - prevL.Y) +
                          (l.Z - prevL.Z) * (l.Z - prevL.Z)) / kJetSpeed;
        prevL = l;
    }
    if (c.fuel < std::min(need, kJetFuel) - 0.05f) return;
    c.jetting = true; c.jetFor = 0.0f; c.jets++;
    bot->SetPhysics(2 /*PHYS_Falling*/);
    const std::string nm = c.lastWaypoint->GetName();
    Logger::Log("chat-command", "[PvpBot] jet start pawn=%d to=%s rise=%.0f d2d=%.0f fuel=%.1f need=%.1f n=%d\n",
        c.pawnId, nm.c_str(), rise, d2d, c.fuel, need, c.jets);
}

// every frame
void JetTick(ATgPawn* bot, Chaser& c, float dt) {
    if (!c.jetting) {
        if (bot->Physics == 1 && c.fuel < kJetFuel) c.fuel = std::min(kJetFuel, c.fuel + kJetRefill * dt);
        return;
    }
    c.jetFor += dt;
    c.fuel -= dt;
    if (c.slamJet) {  // Concussion hop - straight up for kSlamJetFor s, then fall
        if (c.jetFor >= kSlamJetFor || c.fuel <= 0.0f) {
            c.jetting = false; c.slamJet = false;
            Logger::Log("chat-command", "[PvpBot] jet end pawn=%d why=slam hop for=%.2f fuel=%.1f rise=%.0f\n",
                c.pawnId, c.jetFor, c.fuel, bot->Location.Z - c.slamZ0);
            return;
        }
        if (bot->Physics != 2) bot->SetPhysics(2);
        bot->Velocity = FVector(0.0f, 0.0f, kSlamUp);
        return;
    }
    if (c.escapeJet) {  // Air escape - fixed heading, climb then glide
        const char* ew = nullptr;
        if (c.fuel <= 0.0f) ew = "fuel";
        else if (c.jetFor >= kEscapeFor) ew = "escaped";
        else if (c.jetFor > 0.3f && bot->Physics == 1) ew = "landed";
        if (ew) {
            c.jetting = false; c.escapeJet = false;
            Logger::Log("chat-command", "[PvpBot] jet end pawn=%d why=%s escape for=%.1f fuel=%.1f at=(%.0f,%.0f,%.0f)\n",
                c.pawnId, ew, c.jetFor, c.fuel, bot->Location.X, bot->Location.Y, bot->Location.Z);
            return;
        }
        FVector v;
        v.X = c.escapeDir.X * kEscapeH;
        v.Y = c.escapeDir.Y * kEscapeH;
        v.Z = c.jetFor < 0.8f ? kEscapeUp : 120.0f;
        if (bot->Physics != 2) bot->SetPhysics(2);
        bot->Velocity = v;
        return;
    }
    AActor* wp = c.lastWaypoint;
    const char* why = nullptr;
    FVector to = {0, 0, 0};
    if (!wp || !c.viaPath) why = "nowaypoint";
    else if (c.fuel <= 0.0f) why = "fuel";
    else if (c.jetFor > 0.3f && bot->Physics == 1) why = "landed";
    else if (c.jetGlide) {
        // Jump link: straight at the node (a bit above), down is fine
        to.X = wp->Location.X - bot->Location.X;
        to.Y = wp->Location.Y - bot->Location.Y;
        to.Z = wp->Location.Z + 60.0f - FeetZ(bot);
        // 2D-close but still below the node's deck ('arrived'
        // ~200 uu under (0,-1408,6459), fell, re-jetted 8x) -> keep climbing
        if (to.X * to.X + to.Y * to.Y < 120.0f * 120.0f && FeetZ(bot) >= wp->Location.Z - 50.0f) why = "arrived";
    } else {
        const bool air = IsAirNode(wp);
        to.X = wp->Location.X - bot->Location.X;
        to.Y = wp->Location.Y - bot->Location.Y;
        // ground node: aim a little above it and stop once level with it;
        // air node: fly through its centre
        to.Z = wp->Location.Z + (air ? 0.0f : 60.0f) - (air ? bot->Location.Z : FeetZ(bot));
        if (!air && to.Z < 0.0f) why = "above";
    }
    if (why) {
        c.jetting = false; c.jetGlide = false;
        Logger::Log("chat-command", "[PvpBot] jet end pawn=%d why=%s for=%.1f fuel=%.1f at=(%.0f,%.0f,%.0f)\n",
            c.pawnId, why, c.jetFor, c.fuel, bot->Location.X, bot->Location.Y, bot->Location.Z);
        // A jet can loop at an overhang (repeated fuel-out
        // below the ledge). 2 misses toward the same node -> plan around it.
        if (wp && (why[0] == 'f' || why[0] == 'l')) {
            if (wp == c.jetTo) c.jetMiss++; else { c.jetTo = wp; c.jetMiss = 1; }
            if (c.jetMiss >= 2) AvoidNode(bot, c, wp, "jet miss");
        } else if (why[0] == 'a') { c.jetTo = nullptr; c.jetMiss = 0; }
        return;
    }
    const float len = std::sqrt(to.X * to.X + to.Y * to.Y + to.Z * to.Z);
    if (len < 1.0f) return;
    FVector v;
    v.X = to.X / len * kJetSpeed;
    v.Y = to.Y / len * kJetSpeed;
    v.Z = to.Z / len * kJetSpeed;
    if (bot->Physics != 2) bot->SetPhysics(2);
    bot->Velocity = v;
}

} // namespace

// The native fire check (TargetInLOS) traces from a point that is
// blocked at ledges/door frames even when the bot clearly sees the foe's
// head, so a holding bot never fired. For a tracked
// bot aiming at its own foe: a clear world trace from its eye to the foe's
// eye or centre within 3000 uu counts as "in line of sight".
bool FireSight(ATgAIController* aic) {
    if (g_chasers.empty() || !aic) return false;
    auto it = g_chasers.find(aic);
    if (it == g_chasers.end()) return false;
    Chaser& c = it->second;
    APawn* bot = aic->Pawn;
    if (!bot || !c.foe || c.foe->Health <= 0 || bot->Health <= 0) return false;
    if (aic->Enemy != c.foe) return false;
    const float dx = bot->Location.X - c.foe->Location.X;
    const float dy = bot->Location.Y - c.foe->Location.Y;
    const float dz = bot->Location.Z - c.foe->Location.Z;
    if (dx * dx + dy * dy + dz * dz > 3000.0f * 3000.0f) return false;
    FVector eye = bot->Location; eye.Z += bot->BaseEyeHeight;
    FVector head = c.foe->Location; head.Z += c.foe->BaseEyeHeight;
    const FVector zero = {0, 0, 0};
    const bool ok = PbClear((AActor*)bot, head, eye) || PbClear((AActor*)bot, c.foe->Location, eye);
    if (ok && (c.fireLos++ % 20) == 0)
        Logger::Log("chat-command", "[PvpBot] fireLos pawn=%d n=%d dist=%.0f dz=%.0f foeHp=%d\n",
            c.pawnId, c.fireLos, std::sqrt(dx * dx + dy * dy + dz * dz), dz, c.foe->Health);
    return ok;
}


// Skill pacing for a tracked bot's fire check (native TargetInLOS
// result `sight`). Lower skill = slower first shot after the foe comes into
// view and shorter bursts with longer pauses. skill 0 -> 0.9 s reaction,
// 1.0 s burst / 1.2 s pause; 0.5 -> 0.55 / 2.0 / 0.7; 1 -> 0.2 / 3.0 / 0.2.
// An enemy Force Wall / force field stops shots like it does for a
// player. The AI LOS trace ignores force fields (TgAIController__LOSTrace), so
// a tracked PvP bot would empty its gun into the wall: the fire check holds
// while an enemy field (task force != the bot's, alive) crosses the eye->foe
// line, and TickFfWall walks the bot past the field's edge when it is within
// kFfNear. Field size = m_vExtent (else the deployable's collision radius,
// else 200 uu), treated as a disc around the field. Own-team fields: shots
// pass (no change). cs_settings pvpbot_ab_ffwall: 0 off, 1 hold fire only,
// 2 + re-route around it (default).
constexpr float kFfNear = 1200.0f, kFfFor = 3.0f, kFfAgain = 2.0f, kFfSide = 180.0f;
int FfWallKnob(float now) {
    static int v = 2;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_ffwall'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 2;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 2) nv = 2;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] ffwall knob pvpbot_ab_ffwall=%d\n", nv);
        v = nv;
    }
    return v;
}
float FieldRadius(ATgDeployable* d) {
    float r = 0.0f;
    if (d->IsA(ATgDeploy_ForceField::StaticClass())) {
        const FVector& e = ((ATgDeploy_ForceField*)d)->m_vExtent;
        r = e.X > e.Y ? e.X : e.Y;
    }
    if (r < 60.0f) {
        float cr = 0.0f, hh = 0.0f;
        TgProj_Deployable__SpawnDeployable::GetDeployableCollisionCylinder(d->r_nDeployableId, &cr, &hh);
        r = cr;
    }
    if (r < 60.0f) r = 200.0f;
    return r > 500.0f ? 500.0f : r;
}
ATgDeployable* EnemyFieldBetween(APawn* bot, const FVector& a, const FVector& b, float* outR) {
    ATgRepInfo_TaskForce* mine = TfOf(bot);
    if (!mine || TgProj_Deployable__SpawnDeployable::ForceFieldCount() == 0) return nullptr;
    std::vector<ATgDeployable*> ff;
    TgProj_Deployable__SpawnDeployable::LiveForceFields(ff);
    const float sx = b.X - a.X, sy = b.Y - a.Y, sl2 = sx * sx + sy * sy;
    if (sl2 < 1.0f) return nullptr;
    ATgDeployable* best = nullptr;
    float bestT = 2.0f;
    for (ATgDeployable* d : ff) {
        if (!d || d->bDeleteMe || d->m_bInDestroyedState || d->r_nHealth <= 0) continue;
        ATgRepInfo_TaskForce* tf = d->r_DRI ? d->r_DRI->r_TaskforceInfo : nullptr;
        if (!tf && d->r_DRI && d->r_DRI->r_InstigatorInfo) tf = d->r_DRI->r_InstigatorInfo->r_TaskForce;
        if (!tf || tf == mine) continue;
        const float r = FieldRadius(d);
        const float t = ((d->Location.X - a.X) * sx + (d->Location.Y - a.Y) * sy) / sl2;
        if (t <= 0.02f || t >= 0.98f || t >= bestT) continue;
        const float px = a.X + sx * t - d->Location.X, py = a.Y + sy * t - d->Location.Y;
        if (px * px + py * py > r * r) continue;
        const float z = a.Z + (b.Z - a.Z) * t;
        if (std::fabs(z - d->Location.Z) > r + 200.0f) continue;
        best = d; bestT = t; if (outR) *outR = r;
    }
    return best;
}
bool FfHold(ATgAIController* aic, Chaser& c) {
    APawn* bot = aic->Pawn;
    if (!bot || !bot->WorldInfo || !c.foe || c.foe->Health <= 0) return false;
    const float now = bot->WorldInfo->TimeSeconds;
    if (FfWallKnob(now) == 0) return false;
    FVector eye = bot->Location; eye.Z += bot->BaseEyeHeight;
    float r = 0.0f;
    ATgDeployable* w = EnemyFieldBetween(bot, eye, c.foe->Location, &r);
    if (!w) return false;
    c.ffBlockAt = now; c.ffHoldN++;
    if (now - c.ffHoldLogAt >= 2.0f) {
        c.ffHoldLogAt = now;
        const float dx = bot->Location.X - w->Location.X, dy = bot->Location.Y - w->Location.Y;
        Logger::Log("chat-command", "[PvpBot] ab ffwall hold pawn=%d dep=%d r=%.0f wallDist=%.0f foe=%.0f hp=%d n=%d\n",
            c.pawnId, w->r_nDeployableId, r, std::sqrt(dx * dx + dy * dy), std::sqrt(Dist2(bot, c.foe)), w->r_nHealth, c.ffHoldN);
    }
    return true;
}

bool FireAllowed(ATgAIController* aic, bool sight) {
    if (!sight || g_chasers.empty() || !aic) return sight;
    auto it = g_chasers.find(aic);
    if (it == g_chasers.end()) return sight;
    if (FfHold(aic, it->second)) return false;  // Enemy force field in the way
    if (it->second.skill < 0.0f) return sight;
    Chaser& c = it->second;
    APawn* bot = aic->Pawn;
    if (!bot || !bot->WorldInfo) return sight;
    const float now = bot->WorldInfo->TimeSeconds;
    const float s = c.skill;
    if (now - c.lastSightAt > 1.0f) { c.sightSince = now; c.burstAt = now; }  // fresh sighting
    c.lastSightAt = now;
    const float react = 0.9f - 0.7f * s;
    const float burst = 1.0f + 2.0f * s;
    const float pause = 1.2f - 1.0f * s;
    bool ok = true;
    if (now - c.sightSince < react) ok = false;
    else {
        const float cyc = std::fmod(now - c.sightSince - react, burst + pause);
        ok = cyc < burst;
    }
    if (!ok && (c.gated++ % 40) == 0)
        Logger::Log("chat-command", "[PvpBot] fire paced pawn=%d skill=%.2f n=%d\n", c.pawnId, s, c.gated);
    return ok;
}

// Same choke point as SuperAgent's outside march: the behavior engine calls
// SetWhatToDoNext on every action pick and SetMovement clears movement for the
// donor's Wait rows (dest 196), so rewrite the pick to run (238) at
// m_vTriggerLocation (dest 289) re-stamped to the foe each cycle.
// Retreat to heal. A bot under kRetreatBelow of max HP stops pressing
// its foe and falls back to the nearest live medic bot of its own team (the
// medic's WeakestTeammate pick then beams it) or, without one, to its own
// team's nearest start. At the start a slow resupply regen stands in for the
// spawn-room heal (kHomeRegen of max HP per second). Ends healed
// (>= kRetreatDone), on timeout, or when cornered by a weak foe (fight on).
constexpr float kRetreatBelow   = 0.35f;
constexpr float kRetreatDone    = 0.85f;
constexpr float kRetreatMax     = 35.0f;   // s
constexpr float kRetreatCool    = 12.0f;   // s between retreats
constexpr float kRetreatMedicR  = 3000.0f; // medic search radius
constexpr float kRetreatArrive  = 350.0f;  // "there" radius around medic / start
constexpr float kCorneredD      = 450.0f;  // foe this close and weaker -> fight on
constexpr float kHomeRegen      = 0.08f;   // of max HP per second at the start
int MaxHp(APawn* p) {
    const int mx = ((ATgPawn*)p)->r_nHealthMaximum > 0 ? ((ATgPawn*)p)->r_nHealthMaximum : p->HealthMax;
    return mx > 0 ? mx : 1;
}
APawn* NearestMedicBot(ATgPawn* bot, ATgAIController* self) {
    ATgRepInfo_TaskForce* mine = TfOf((APawn*)bot);
    if (!mine) return nullptr;
    APawn* best = nullptr; float bestD = kRetreatMedicR * kRetreatMedicR;
    for (auto& kv : g_chasers) {
        if (kv.first == self || !kv.second.medic || !kv.first || kv.first->bDeleteMe) continue;
        APawn* m = kv.first->Pawn;
        if (!m || m->bDeleteMe || m->Health <= 0 || ((ATgPawn*)m)->r_nPawnId != kv.second.pawnId) continue;
        if (TfOf(m) != mine) continue;
        const float d = Dist2((APawn*)bot, m);
        if (d < bestD) { bestD = d; best = m; }
    }
    return best;
}
bool NearestOwnStart(ATgPawn* bot, FVector& out) {
    ATgRepInfo_TaskForce* tf = TfOf((APawn*)bot);
    if (!tf) return false;
    ActorCache::CacheMapActors();
    ATgGame* game = (ATgGame*)Globals::Get().GGameInfo;
    const int prio = game ? game->s_nCurrentPriority : 0;
    float bestD = 1.0e18f; bool ok = false;
    for (ATgTeamPlayerStart* s : ActorCache::PlayerStarts) {
        if (!s || !s->bEnabled || s->m_nTaskForce != (int)tf->r_nTaskForce) continue;
        if (s->m_nPriority > 0 && s->m_nPriority != prio) continue;
        const float dx = s->Location.X - bot->Location.X, dy = s->Location.Y - bot->Location.Y,
                    dz = s->Location.Z - bot->Location.Z;
        const float d = dx * dx + dy * dy + dz * dz;
        if (d < bestD) { bestD = d; out = s->Location; ok = true; }
    }
    return ok;
}
void EndRetreat(ATgAIController* aic, ATgPawn* bot, Chaser& c, const char* why) {
    const float now = aic->WorldInfo ? aic->WorldInfo->TimeSeconds : 0.0f;
    Logger::Log("chat-command", "[PvpBot] retreat end pawn=%d hp=%d->%d/%d after=%.1fs why=%s (#%d)\n",
        c.pawnId, c.retreatHp0, bot->Health, MaxHp((APawn*)bot), now - c.retreatAt, why, c.retreats);
    c.retreat = false; c.retreatMedic = nullptr; c.retreatAt = now; c.homeFor = 0.0f;
    if (c.cloaked && c.cloakEscape) EndCloak(aic, bot, c, "escape end");
    c.route.clear(); c.routeGoal = nullptr;
    aic->m_bInterrupt = 1;
}
// Air escape. A retreating bot with a foe close by (seen, or < 700 uu)
// spends its jetpack to break contact: one 1.6 s burst up and away from the
// foe, bent toward where it is retreating to (medic / start); if that line is
// blocked, sideways either way, then straight away. Needs kEscapeMinFuel,
// 6 s between escapes, not when already home; higher skill reacts sooner.
constexpr float kEscapeFoeD    = 1400.0f;
constexpr float kEscapeMinFuel = 1.6f;
constexpr float kEscapeCool    = 6.0f;
void TryEscapeJet(ATgAIController* aic, ATgPawn* bot, Chaser& c, float now) {
    if (c.jetting || now - c.escapeAt < kEscapeCool) return;
    if (!c.foe || c.foe->bDeleteMe || c.foe->Health <= 0) return;
    const float fd2 = Dist2((APawn*)bot, c.foe);
    if (fd2 > kEscapeFoeD * kEscapeFoeD || fd2 < 1.0f) return;
    // retreating bots skip WantsChase, so c.lastSight is stale: trace eye to eye
    const FVector eb(bot->Location.X, bot->Location.Y, bot->Location.Z + 50.0f);
    const FVector ef(c.foe->Location.X, c.foe->Location.Y, c.foe->Location.Z + 50.0f);
    const bool seen = PbClear((AActor*)bot, ef, eb);
    const char* skip = nullptr;
    if (bot->Physics != 1) skip = "air";
    else if (c.fuel < kEscapeMinFuel) skip = "fuel";
    else if (!seen && fd2 > 700.0f * 700.0f) skip = "nosight";
    if (skip) {
        if (now - c.escDiagAt > 2.0f) {
            c.escDiagAt = now;
            Logger::Log("chat-command", "[PvpBot] escape skip pawn=%d why=%s foeD=%.0f fuel=%.1f phys=%d\n",
                c.pawnId, skip, std::sqrt(fd2), c.fuel, (int)bot->Physics);
        }
        return;
    }
    const FVector g = c.retreatMedic ? c.retreatMedic->Location : c.retreatTo;
    float gx = g.X - bot->Location.X, gy = g.Y - bot->Location.Y;
    const float gl = std::sqrt(gx * gx + gy * gy);
    if (!c.retreatMedic && gl < kRetreatArrive) return;  // home: stay for the resupply
    const float s = c.skill < 0.0f ? 0.5f : c.skill;
    if ((float)std::rand() / (float)RAND_MAX > 0.25f + 0.5f * s) return;
    float ax = bot->Location.X - c.foe->Location.X, ay = bot->Location.Y - c.foe->Location.Y;
    const float al = std::sqrt(ax * ax + ay * ay);
    if (al < 1.0f) return;
    ax /= al; ay /= al;
    if (gl > 1.0f) { gx /= gl; gy /= gl; } else { gx = 0.0f; gy = 0.0f; }
    float cand[4][2] = {{0.6f * ax + 0.4f * gx, 0.6f * ay + 0.4f * gy}, {-ay, ax}, {ay, -ax}, {ax, ay}};
    if (gx * -ay + gy * ax < 0.0f) { std::swap(cand[1][0], cand[2][0]); std::swap(cand[1][1], cand[2][1]); }  // goal side first
    const FVector up(bot->Location.X, bot->Location.Y, bot->Location.Z + 300.0f);
    if (!PbClear((AActor*)bot, up, bot->Location)) {
        Logger::Log("chat-command", "[PvpBot] escape blocked pawn=%d why=ceiling\n", c.pawnId);
        c.escapeAt = now - kEscapeCool + 2.0f;  // retry in 2 s
        return;
    }
    int pick = -1;
    for (int i = 0; i < 4 && pick < 0; i++) {
        float l = std::sqrt(cand[i][0] * cand[i][0] + cand[i][1] * cand[i][1]);
        if (l < 0.3f) continue;
        cand[i][0] /= l; cand[i][1] /= l;
        const FVector end(bot->Location.X + cand[i][0] * 600.0f, bot->Location.Y + cand[i][1] * 600.0f, bot->Location.Z + 420.0f);
        if (PbClear((AActor*)bot, end, bot->Location) && PbClear((AActor*)bot, end, up)) pick = i;
    }
    if (pick < 0) {
        Logger::Log("chat-command", "[PvpBot] escape blocked pawn=%d why=walls\n", c.pawnId);
        c.escapeAt = now - kEscapeCool + 2.0f;
        return;
    }
    c.escapeDir = FVector(cand[pick][0], cand[pick][1], 0.0f);
    c.jetting = true; c.escapeJet = true; c.jetGlide = false; c.jetFor = 0.0f; c.jets++;
    c.escapeAt = now; c.escapes++;
    bot->SetPhysics(2 /*PHYS_Falling*/);
    static const char* kDirName[4] = {"away+goal", "side", "side", "away"};
    Logger::Log("chat-command", "[PvpBot] escape start pawn=%d n=%d dir=%s foeD=%.0f sight=%d fuel=%.1f hp=%d at=(%.0f,%.0f,%.0f)\n",
        c.pawnId, c.escapes, kDirName[pick], std::sqrt(fd2), seen ? 1 : 0, c.fuel, bot->Health,
        bot->Location.X, bot->Location.Y, bot->Location.Z);
}
// true while retreating (caller skips heal/turret/chase and issues the run pick)
bool TickRetreat(ATgAIController* aic, ATgPawn* bot, Chaser& c, float dt) {
    const float now = aic->WorldInfo ? aic->WorldInfo->TimeSeconds : 0.0f;
    const int mx = MaxHp((APawn*)bot);
    const float r = (float)bot->Health / (float)mx;
    const bool foeUp = c.foe && !c.foe->bDeleteMe && c.foe->Health > 0;
    const bool cornered = foeUp && Dist2((APawn*)bot, c.foe) < kCorneredD * kCorneredD &&
                          (float)c.foe->Health / (float)MaxHp(c.foe) < r;
    if (!c.retreat) {
        if (r >= kRetreatBelow || cornered || now - c.retreatAt < kRetreatCool) return false;
        c.retreatMedic = NearestMedicBot(bot, aic);
        if (!c.retreatMedic && !NearestOwnStart(bot, c.retreatTo)) return false;
        c.retreat = true; c.retreatAt = now; c.retreatHp0 = bot->Health; c.homeFor = 0.0f;
        c.retreats++; c.route.clear(); c.routeGoal = nullptr;
        if (c.patient) EndHeal(aic, c, "retreat");
        if (c.cloaked && CloakKnob(now) < 2) EndCloak(aic, bot, c, "retreat");
        else if (c.cloaked) {  // The cloak becomes the escape cloak
            c.cloakEscape = true; c.cloakAt = now;
            Logger::Log("chat-command", "[PvpBot] cloak becomes escape pawn=%d power=%.0f\n", c.pawnId, bot->r_fCurrentPowerPool);
        }
        Logger::Log("chat-command", "[PvpBot] retreat start pawn=%d hp=%d/%d at=(%.0f,%.0f,%.0f) to=%s dist=%.0f (#%d)\n",
            c.pawnId, bot->Health, mx, bot->Location.X, bot->Location.Y, bot->Location.Z, c.retreatMedic ? "medic" : "start",
            c.retreatMedic ? std::sqrt(Dist2((APawn*)bot, c.retreatMedic))
                           : std::sqrt((bot->Location.X - c.retreatTo.X) * (bot->Location.X - c.retreatTo.X) +
                                       (bot->Location.Y - c.retreatTo.Y) * (bot->Location.Y - c.retreatTo.Y)),
            c.retreats);
    }
    if (r >= kRetreatDone) { EndRetreat(aic, bot, c, "healed"); return false; }
    if (now - c.retreatAt > kRetreatMax) { EndRetreat(aic, bot, c, "timeout"); return false; }
    if (cornered) { EndRetreat(aic, bot, c, "cornered"); return false; }
    if (c.retreatMedic && (c.retreatMedic->bDeleteMe || c.retreatMedic->Health <= 0)) {
        c.retreatMedic = nullptr;  // medic down: head home instead
        if (!NearestOwnStart(bot, c.retreatTo)) { EndRetreat(aic, bot, c, "medic lost"); return false; }
        Logger::Log("chat-command", "[PvpBot] retreat pawn=%d medic lost -> start\n", c.pawnId);
    }
    if (!c.retreatMedic) {
        const float dx = bot->Location.X - c.retreatTo.X, dy = bot->Location.Y - c.retreatTo.Y;
        if (dx * dx + dy * dy < kRetreatArrive * kRetreatArrive) {
            c.homeFor += dt;
            if (c.homeFor > 1.0f) {
                const int add = std::max(1, (int)(kHomeRegen * (float)mx * dt));
                bot->Health = std::min(mx, bot->Health + add);
            }
        }
    }
    if (bot->bIsWalking) bot->bIsWalking = 0;
    TryEscapeJet(aic, bot, c, now);
    MaybeStartJet(bot, c);
    if (((int)((now - c.retreatAt) * 2.0f)) % 6 == 0) {  // every ~3 s
        const FVector g = c.retreatMedic ? c.retreatMedic->Location : c.retreatTo;
        const float gx = bot->Location.X - g.X, gy = bot->Location.Y - g.Y;
        Logger::Log("chat-command", "[PvpBot] retreat pawn=%d dist=%.0f speed=%.0f hp=%d home=%.1f trig=(%.0f,%.0f) phys=%d\n",
            c.pawnId, std::sqrt(gx * gx + gy * gy), std::sqrt(bot->Velocity.X * bot->Velocity.X + bot->Velocity.Y * bot->Velocity.Y),
            bot->Health, c.homeFor, aic->m_vTriggerLocation.X, aic->m_vTriggerLocation.Y, (int)bot->Physics);
    }
    aic->eventSetWhatToDoNext(238, 289);
    return true;
}
// Where a retreating bot runs this pick (medic / start, via the path net).
FVector RetreatTarget(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    const FVector goal = c.retreatMedic ? c.retreatMedic->Location : c.retreatTo;
    const float dx = bot->Location.X - goal.X, dy = bot->Location.Y - goal.Y;
    if (dx * dx + dy * dy < kRetreatArrive * kRetreatArrive) { c.viaPath = false; return bot->Location; }  // there: hold
    if (c.retreatMedic && aic->ActorReachable((AActor*)c.retreatMedic)) { c.viaPath = false; return goal; }
    // own planner (engine searches return nothing for our bots) to the
    // path node nearest the goal; partial routes end nearest the goal; jets
    // via MaybeStartJet (TickRetreat) on raised waypoints.
    const int net = bot->Anchor ? bot->Anchor->NetworkID : -1;
    ANavigationPoint* nav = NearestLinkedNav((AActor*)bot, goal, net, nullptr, nullptr, nullptr);
    // Goal on another path network (Trafalgar koth) -> no
    // node of OUR net near it -> nav null -> straight run forever. Take the
    // goal's own node; OwnRouteNext then plans with jump links.
    if (!nav && net >= 0) nav = NearestLinkedNav((AActor*)bot, goal, -1, nullptr, nullptr, nullptr);
    AActor* next = nav ? OwnRouteNext(aic, bot, c, nav, &goal) : nullptr;
    if (!next) { c.viaPath = false; return goal; }
    c.viaPath = true;
    c.lastWaypoint = next;
    return next->Location;
}

// Objective play. With no foe in reach (none, or far + unseen) a bot
// heads for the nearest active, unlocked capture point / payload of the map:
// one its team does not own (capture / push) before one it owns (defend), and
// stands on it; fights there only when a foe is close or seen. Escort maps:
// the goal is the payload robot (r_AttachedActor). Proximity objectives only
// count AI pawns with s_bAllowAIBotInteraction - set on maps with PvP bots.
constexpr float kObjFoeFar   = 2000.0f;  // foe this far + unseen -> objective
constexpr float kObjHoldMax  = 400.0f;   // stand within min(radius*0.6, this) of the goal
bool IsCapturable(ATgMissionObjective* o) {
    if (!o || o->bDeleteMe) return false;
    return o->IsA(ATgMissionObjective_Proximity::StaticClass());
}
AActor* ObjectiveGoalActor(ATgMissionObjective* o) {
    if (o->IsA(ATgMissionObjective_Escort::StaticClass())) {
        AActor* r = ((ATgMissionObjective_Escort*)o)->r_AttachedActor;
        if (r && !r->bDeleteMe) return r;
    }
    AActor* la = ((ATgMissionObjective_Proximity*)o)->LocationActor;
    return (la && !la->bDeleteMe) ? la : (AActor*)o;
}
// The objectives a bot can play = the map-placed ones (ActorCache)
// plus the game's own list (GRI m_MissionObjectives). Point-rotation (koth)
// maps spawn their points from a pool at match start, so they exist only in
// the GRI list.
// OFF until bots can cross gaps between path networks:
// on Rot_Trafalgar_P the koth points sit on another network than the spawn
// decks; the bot reaches the deck edge, then walks off into the void every life.
constexpr bool kPlayPooledObjectives = true;  // On with jump links
std::vector<ATgMissionObjective*> BotObjectives(AWorldInfo* wi, bool pooled = true) {
    ActorCache::CacheMapActors();
    std::vector<ATgMissionObjective*> v;
    for (ATgMissionObjective* o : ActorCache::MissionObjectives)
        if (o && !o->bDeleteMe) v.push_back(o);
    AGameInfo* g = wi ? wi->Game : nullptr;
    ATgRepInfo_Game* gri = g ? (ATgRepInfo_Game*)g->GameReplicationInfo : nullptr;
    if (pooled && gri && gri->m_MissionObjectives.Data) {
        for (int i = 0; i < gri->m_MissionObjectives.Num(); i++) {
            ATgMissionObjective* o = gri->m_MissionObjectives.Data[i];
            if (!o || o->bDeleteMe) continue;
            if (std::find(v.begin(), v.end(), o) == v.end()) v.push_back(o);
        }
    }
    return v;
}
// lets bots count on a capture point; idempotent, done at scan + at pick
// (pool-spawned points can appear after the scan). Returns old flags.
void AllowBotCapture(ATgMissionObjective* o, int* allow, int* ignore) {
    ATgMissionObjective_Proximity* p = (ATgMissionObjective_Proximity*)o;
    if (allow) *allow = (int)p->s_bAllowAIBotInteraction;
    p->s_bAllowAIBotInteraction = 1;
    // the capture cylinder skips non-player pawns (ShouldIgnoreActor)
    // when m_bIgnoreNonPlayers is set - run bot on the point,
    // status stayed 7 until the player stepped on it
    if (p->s_CollisionProxy) {
        if (ignore) *ignore = (int)p->s_CollisionProxy->m_bIgnoreNonPlayers;
        p->s_CollisionProxy->m_bIgnoreNonPlayers = 0;
    }
}
void ObjectiveScanLog(AWorldInfo* wi) {  // once per map (again if the list grows)
    static void* s_world = nullptr;
    static size_t s_count = 0;
    if (!wi) return;
    std::vector<ATgMissionObjective*> objs = BotObjectives(wi);
    if (s_world == (void*)wi && objs.size() <= s_count) return;
    s_world = (void*)wi;
    s_count = objs.size();
    int n = 0;
    for (ATgMissionObjective* o : objs) {
        const std::string cls = o->Class ? o->Class->GetName() : "?";
        const bool cap = IsCapturable(o);
        int allow = -1, ignore = -1;
        if (cap) AllowBotCapture(o, &allow, &ignore);
        Logger::Log("chat-command",
            "[PvpBot] objective scan id=%d cls=%s active=%d locked=%d owner=%d status=%d prio=%d radius=%d "
            "aiAllow=%d->%d ignoreNonPlayers=%d->0 loc=(%.0f,%.0f,%.0f)\n",
            o->nObjectiveId, cls.c_str(), (int)o->r_bIsActive, (int)o->r_bIsLocked, o->r_nOwnerTaskForce,
            (int)o->r_eStatus, o->nPriority, o->m_fProximityRadius, allow, cap ? 1 : allow, ignore,
            o->Location.X, o->Location.Y, o->Location.Z);
        n++;
    }
    Logger::Log("chat-command", "[PvpBot] objective scan done count=%d\n", n);
}
ATgMissionObjective* PickObjective(ATgPawn* bot, bool* outCapture) {
    ATgRepInfo_TaskForce* tf = TfOf((APawn*)bot);
    if (!tf) return nullptr;
    ActorCache::CacheMapActors();
    ATgMissionObjective* bestCap = nullptr; float capD = 1.0e18f;
    ATgMissionObjective* bestDef = nullptr; float defD = 1.0e18f;
    for (ATgMissionObjective* o : BotObjectives(bot->WorldInfo, kPlayPooledObjectives)) {
        if (!IsCapturable(o) || !o->r_bIsActive || o->r_bIsLocked) continue;
        AllowBotCapture(o, nullptr, nullptr);
        AActor* g = ObjectiveGoalActor(o);
        const float dx = g->Location.X - bot->Location.X, dy = g->Location.Y - bot->Location.Y,
                    dz = g->Location.Z - bot->Location.Z;
        const float d = dx * dx + dy * dy + dz * dz;
        if (o->r_nOwnerTaskForce != (int)tf->r_nTaskForce) { if (d < capD) { capD = d; bestCap = o; } }
        else if (d < defD) { defD = d; bestDef = o; }
    }
    if (outCapture) *outCapture = bestCap != nullptr;
    return bestCap ? bestCap : bestDef;
}
float HoldRadius(ATgMissionObjective* o) {
    // m_fProximityRadius reads 14 on MissileComplex (not uu) -> fixed hold
    // spot spread by pawn id so a team does not stack on one pixel
    const float r = o->m_fProximityRadius > 100 ? 0.6f * (float)o->m_fProximityRadius : kObjHoldMax;
    return std::min(r, kObjHoldMax);
}
void EndObjective(ATgAIController* aic, Chaser& c, const char* why) {
    Logger::Log("chat-command", "[PvpBot] objective end pawn=%d id=%d why=%s\n",
        c.pawnId, c.objective ? c.objective->nObjectiveId : -1, why);
    c.objective = nullptr; c.objOn = false; c.route.clear(); c.routeGoal = nullptr;
    c.rally = false;
    aic->m_bInterrupt = 1;
}
// true while the bot plays the objective (caller skips the chase).
// Group up instead of trickling in. A bot about to CAPTURE a point
// (within kRallyR, not yet on it) with nobody of its team on the point or near
// it waits while a teammate bot heading for the same point is still behind;
// it goes when that teammate is within kRallyNear, or after kRallyMax s.
// Once per objective (no re-waiting after a death or a fight).
constexpr float kRallyR    = 1800.0f;  // start waiting this close to the goal
constexpr float kRallyNear = 900.0f;   // teammate this close = grouped
constexpr float kRallyBack = 5000.0f;  // only wait for teammates this close to the goal
constexpr float kRallyMax  = 10.0f;
// counts: nNear = team pawns (bots or players) within kRallyNear of the bot or
// on the point; behind = teammate bots playing o, farther from the goal.
void RallyCount(ATgAIController* self, ATgPawn* bot, ATgMissionObjective* o, AActor* g, float gd,
                int* nNear, int* behind, float* bestBehind) {
    *nNear = 0; *behind = 0; *bestBehind = 1.0e9f;
    ATgRepInfo_TaskForce* mine = TfOf((APawn*)bot);
    if (!mine || !bot->WorldInfo) return;
    const float hold = HoldRadius(o) + 150.0f;
    for (APawn* q = bot->WorldInfo->PawnList; q; q = q->NextPawn) {
        if (q == (APawn*)bot || q->bDeleteMe || q->Health <= 0 || TfOf(q) != mine) continue;
        const float qx = q->Location.X - g->Location.X, qy = q->Location.Y - g->Location.Y;
        const float qg = std::sqrt(qx * qx + qy * qy);
        if (qg < hold || Dist2((APawn*)bot, q) < kRallyNear * kRallyNear) { (*nNear)++; continue; }
    }
    for (auto& kv : g_chasers) {
        if (kv.first == self || !kv.first || kv.first->bDeleteMe || kv.second.objective != o) continue;
        APawn* m = kv.first->Pawn;
        if (!m || m->bDeleteMe || m->Health <= 0 || ((ATgPawn*)m)->r_nPawnId != kv.second.pawnId) continue;
        if (TfOf(m) != mine || kv.second.retreat) continue;
        const float mx = m->Location.X - g->Location.X, my = m->Location.Y - g->Location.Y;
        const float mg = std::sqrt(mx * mx + my * my);
        if (mg <= gd || mg > kRallyBack) continue;
        if (Dist2((APawn*)bot, m) < kRallyNear * kRallyNear) continue;  // already counted nNear
        (*behind)++;
        const float d = std::sqrt(Dist2((APawn*)bot, m));
        if (d < *bestBehind) *bestBehind = d;
    }
}
void TickRally(ATgAIController* aic, ATgPawn* bot, Chaser& c, ATgMissionObjective* o, AActor* g,
               float gd, bool cap, bool on, float dt) {
    if (c.rallyDone && !c.rally) return;
    if (!c.rally) {
        if (!cap || on || gd > kRallyR) return;
        int nNear = 0, behind = 0; float bd = 0.0f;
        RallyCount(aic, bot, o, g, gd, &nNear, &behind, &bd);
        c.rallyDone = true;  // decide once on reaching the rally ring
        if (nNear > 0 || behind == 0) {  // Log the no-wait decision too (smoke/evidence)
            Logger::Log("chat-command", "[PvpBot] rally skip pawn=%d id=%d why=%s dist=%.0f nNear=%d behind=%d\n",
                c.pawnId, o->nObjectiveId, nNear > 0 ? "grouped" : "no mates", gd, nNear, behind);
            return;
        }
        c.rally = true; c.rallyFor = 0.0f; c.route.clear(); c.routeGoal = nullptr;
        Logger::Log("chat-command", "[PvpBot] rally wait pawn=%d id=%d dist=%.0f behind=%d mate=%.0f\n",
            c.pawnId, o->nObjectiveId, gd, behind, bd);
        aic->m_bInterrupt = 1;
        return;
    }
    c.rallyFor += dt;
    int nNear = 0, behind = 0; float bd = 0.0f;
    RallyCount(aic, bot, o, g, gd, &nNear, &behind, &bd);
    const char* why = gd > kRallyR + 600.0f ? "left ring" :  // respawned / pushed away
                      cap ? (nNear > 0 ? "grouped" : behind == 0 ? "no mates" : c.rallyFor >= kRallyMax ? "timeout" : nullptr)
                          : "point ours";
    if (!why) return;
    c.rally = false; c.route.clear(); c.routeGoal = nullptr;
    Logger::Log("chat-command", "[PvpBot] rally go pawn=%d id=%d why=%s waited=%.1f nNear=%d\n",
        c.pawnId, o->nObjectiveId, why, c.rallyFor, nNear);
    aic->m_bInterrupt = 1;
}
// objective-run watchdog (every 0.5 s, not on the point / rallying).
// (a) standing still >= 3 s while it should run (e.g. a 2nd
// bot on the shared start stuck at speed=0 with a good route) -> log
// why + hop toward the waypoint; 3rd hop also avoids that waypoint.
// (b) no 200 uu of progress toward the goal in 15 s -> avoid the waypoint.
void TickUnstick(ATgPawn* bot, Chaser& c, float gd, bool on, float dt) {
    if (on || c.rally || c.jetting) { c.stillFor = 0.0f; c.progClock = 0.0f; c.progBest = gd; return; }
    const float sp = std::sqrt(bot->Velocity.X * bot->Velocity.X + bot->Velocity.Y * bot->Velocity.Y);
    c.stillFor = sp < 20.0f ? c.stillFor + dt : 0.0f;
    if (gd < c.progBest - 200.0f) { c.progBest = gd; c.progClock = 0.0f; } else c.progClock += dt;
    if (c.stillFor >= 3.0f) {
        c.stillFor = 0.0f; c.nudges++;
        AActor* wp = c.viaPath ? c.lastWaypoint : nullptr;
        if (!wp && !c.route.empty() && c.routeIdx < c.route.size()) wp = (AActor*)c.route[c.routeIdx];
        if (!c.viaPath && !c.noDirect) {  // Straight run stalled
            c.noDirect = true; c.routeAt = -100.0f;
            Logger::Log("chat-command", "[PvpBot] unstick nodirect pawn=%d route=%d\n", c.pawnId, (int)c.route.size());
        }
        const FVector to = wp ? wp->Location : (c.objective ? ObjectiveGoalActor(c.objective)->Location : bot->Location);
        Logger::Log("chat-command",
            "[PvpBot] unstick pawn=%d n=%d phys=%d via=%d idx=%d/%d moves=%d walking=%d gs=%.0f base=%d to=(%.0f,%.0f,%.0f) bot=(%.0f,%.0f,%.0f)\n",
            c.pawnId, c.nudges, (int)bot->Physics, (int)c.viaPath, (int)c.routeIdx, (int)c.route.size(), c.moveCalls,
            (int)bot->bIsWalking, bot->GroundSpeed, bot->Base ? 1 : 0, to.X, to.Y, to.Z,
            bot->Location.X, bot->Location.Y, bot->Location.Z);
        // Run 014223 - a bot spawned on the slot-offset spot moved 0 uu
        // even after hops (embedded at the start). Hop moved nothing -> warp
        // onto the waypoint node (first route node, next to the start).
        const float hx = bot->Location.X - c.hopFrom.X, hy = bot->Location.Y - c.hopFrom.Y, hz = bot->Location.Z - c.hopFrom.Z;
        // No route + no waypoint (start=none) -> warp target =
        // nearest linked node within 6000 uu on the goal's network (else any)
        if (!wp && c.nudges > 1 && c.objective && c.warps < 3) {
            const FVector& at = bot->Location;
            AActor* ga = ObjectiveGoalActor(c.objective);
            ANavigationPoint* gn = NearestLinkedNav(ga, ga->Location, -1, nullptr, nullptr, nullptr);
            wp = (AActor*)NearestLinkedNav((AActor*)bot, at, gn ? gn->NetworkID : -1, nullptr, nullptr, nullptr, 6000.0f);
            if (!wp) wp = (AActor*)NearestLinkedNav((AActor*)bot, at, -1, nullptr, nullptr, nullptr, 6000.0f);
            Logger::Log("chat-command", "[PvpBot] unstick noroute pawn=%d node=%d\n", c.pawnId, wp ? 1 : 0);
        }
        if (c.nudges > 1 && wp && hx * hx + hy * hy + hz * hz < 30.0f * 30.0f && c.warps < 3) {
            FVector t = wp->Location; t.Z += 60.0f;
            const bool ok = bot->SetLocation(t);
            c.warps++;
            Logger::Log("chat-command", "[PvpBot] unstick warp pawn=%d ok=%d n=%d to=(%.0f,%.0f,%.0f)\n",
                c.pawnId, (int)ok, c.warps, t.X, t.Y, t.Z);
            c.hopFrom = bot->Location; c.routeAt = -100.0f;
            return;
        }
        c.hopFrom = bot->Location;
        const float dx = to.X - bot->Location.X, dy = to.Y - bot->Location.Y;
        const float l = std::sqrt(dx * dx + dy * dy);
        if (l > 1.0f) {
            FVector v; v.X = dx / l * 400.0f; v.Y = dy / l * 400.0f; v.Z = 350.0f;
            bot->SetPhysics(2 /*PHYS_Falling*/);
            bot->Velocity = v;
        }
        if (wp && c.nudges % 3 == 0) AvoidNode(bot, c, wp, "still");
    }
    // Rot_Trafalgar_P: straight run at speed 289 but no
    // progress for 100 s ('reachable' false positive across a deck gap; the
    // still-check never fires while moving) -> after 6 s follow the route.
    if (c.progClock >= 6.0f && !c.viaPath && !c.noDirect) {
        c.noDirect = true; c.routeAt = -100.0f; c.progClock = 0.0f; c.progBest = gd;
        Logger::Log("chat-command", "[PvpBot] unstick nodirect pawn=%d why=noprogress gd=%.0f\n", c.pawnId, gd);
        return;
    }
    if (c.progClock >= 15.0f) {
        c.progClock = 0.0f; c.progBest = gd;
        if (c.viaPath && c.lastWaypoint) AvoidNode(bot, c, c.lastWaypoint, "no progress");
    }
}

// chase-mode watchdog. A bot frozen on its spawn spot
// (slot offset) the whole chase - TickUnstick only ran in objective mode.
// Chasing (run wanted) but speed < 20 for 3 s -> hop toward the waypoint;
// a hop that moved < 30 uu -> warp onto the waypoint / nearest linked node
// (max 3 warps per life, shared with objective mode).
void TickChaseUnstick(ATgPawn* bot, Chaser& c, float dt) {
    const float now = bot->WorldInfo ? bot->WorldInfo->TimeSeconds : 0.0f;
    if (now - c.chaseTickAt > 1.0f) c.chaseStill = 0.0f;  // chase run was interrupted (hold/strafe/objective)
    c.chaseTickAt = now;
    if (c.jetting || c.backoff || c.scoped || !c.foe) { c.chaseStill = 0.0f; return; }
    const float sp = std::sqrt(bot->Velocity.X * bot->Velocity.X + bot->Velocity.Y * bot->Velocity.Y);
    c.chaseStill = sp < 20.0f ? c.chaseStill + dt : 0.0f;
    if (c.chaseStill < 3.0f) return;
    c.chaseStill = 0.0f; c.chaseNudges++;
    AActor* wp = c.viaPath ? c.lastWaypoint : nullptr;
    if (!wp && !c.route.empty() && c.routeIdx < c.route.size()) wp = (AActor*)c.route[c.routeIdx];
    const float hx = bot->Location.X - c.hopFrom.X, hy = bot->Location.Y - c.hopFrom.Y, hz = bot->Location.Z - c.hopFrom.Z;
    const bool hopFailed = c.chaseNudges > 1 && hx * hx + hy * hy + hz * hz < 30.0f * 30.0f;
    Logger::Log("chat-command",
        "[PvpBot] unstick chase pawn=%d n=%d phys=%d via=%d wp=%d hopFailed=%d bot=(%.0f,%.0f,%.0f)\n",
        c.pawnId, c.chaseNudges, (int)bot->Physics, (int)c.viaPath, wp ? 1 : 0, (int)hopFailed,
        bot->Location.X, bot->Location.Y, bot->Location.Z);
    if (hopFailed && c.warps < 3) {
        AActor* to = wp;
        if (!to) {
            ANavigationPoint* fn = FoeNav(c, bot);
            to = (AActor*)NearestLinkedNav((AActor*)bot, bot->Location, fn ? fn->NetworkID : -1, nullptr, nullptr, nullptr, 6000.0f);
            if (!to) to = (AActor*)NearestLinkedNav((AActor*)bot, bot->Location, -1, nullptr, nullptr, nullptr, 6000.0f);
        }
        if (to) {
            FVector t = to->Location; t.Z += 60.0f;
            const bool ok = bot->SetLocation(t);
            c.warps++;
            Logger::Log("chat-command", "[PvpBot] unstick chase warp pawn=%d ok=%d n=%d to=(%.0f,%.0f,%.0f)\n",
                c.pawnId, (int)ok, c.warps, t.X, t.Y, t.Z);
            c.hopFrom = bot->Location; c.routeAt = -100.0f;
            return;
        }
    }
    c.hopFrom = bot->Location;
    const FVector to = wp ? wp->Location : c.foe->Location;
    const float dx = to.X - bot->Location.X, dy = to.Y - bot->Location.Y;
    const float l = std::sqrt(dx * dx + dy * dy);
    if (l > 1.0f) {
        FVector v; v.X = dx / l * 400.0f; v.Y = dy / l * 400.0f; v.Z = 350.0f;
        bot->SetPhysics(2 /*PHYS_Falling*/);
        bot->Velocity = v;
    }
    if (wp && c.chaseNudges % 3 == 0) AvoidNode(bot, c, wp, "still chase");
}

// AvA defender cargo carried by bots. A hex held by a service (bot) agency
// sends its defender cargo (AVA_CARGO side bot_carriers); with no foe in sight a
// defender bot runs to the nearest free pack (< 5000 uu), picks it up (= a player
// pickup: the item counts as used), plays the objective as usual and, once on or
// near the objective (< 700 uu), stops 0.5 s (tell) and deploys it at its feet via
// the player deploy path. No objective after 20 s carrying -> deploys where it is.
// A foe in sight aborts the pickup run (fight first); carried packs are kept.
// cs_settings pvpbot_ab_cargo: 0 off, 1 on (default).
constexpr float kCargoFind = 5000.0f, kCargoTake = 170.0f, kCargoNear = 700.0f, kCargoWind = 0.5f;
int CargoKnob(float now) {
    static int v = 1;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_cargo'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 1;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 1) nv = 1;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] cargo knob pvpbot_ab_cargo=%d\n", nv);
        v = nv;
    }
    return v;
}
void EndCargoRun(Chaser& c, const char* why) {
    if (!c.cargoPack) return;
    Logger::Log("chat-command", "[PvpBot] ab cargo abort pawn=%d why=%s\n", c.pawnId, why);
    c.cargoPack = nullptr;
    AvaCargoDll::BotReleasePack(c.pawnId);
}
// Bots step out of poison like a player. A bot that newly carries a
// poison effect group (category 303 - Poison Grenade, Poison Aura, Poison
// Injector, Sticky Poison Mine) runs kPoisonStep uu away from where it was hit
// (away from the nearest seen foe within 1500 uu, else backwards; the first of
// 7 directions that is clear with floor under it), at most kPoisonFor s, still
// shooting. Again only if still poisoned kPoisonAgain s later and back near the
// spot. Not while retreating. cs_settings pvpbot_ab_poison: 0 off, 1 on (default).
constexpr int   kPoisonCat = 303;
constexpr float kPoisonStep = 400.0f, kPoisonFor = 2.5f, kPoisonAgain = 4.0f;
int PoisonKnob(float now) {
    static int v = 1;
    static float readAt = -1000.0f;
    if (now >= readAt && now - readAt < 60.0f) return v;
    readAt = now;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'pvpbot_ab_poison'", -1, &st, nullptr) == SQLITE_OK) {
        int nv = 1;
        if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL) nv = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (nv < 0 || nv > 1) nv = 1;
        if (nv != v) Logger::Log("chat-command", "[PvpBot] poison knob pvpbot_ab_poison=%d\n", nv);
        v = nv;
    }
    return v;
}

bool TickPoison(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!aic->WorldInfo) return false;
    const float now = aic->WorldInfo->TimeSeconds;
    const bool pois = HasCategory(bot, kPoisonCat);
    const bool fresh = pois && !c.poisonWas;
    c.poisonWas = pois;
    if (c.poisonOn) {
        const float dx = bot->Location.X - c.poisonTo.X, dy = bot->Location.Y - c.poisonTo.Y;
        const char* end = dx * dx + dy * dy < 90.0f * 90.0f ? "there"
                        : now - c.poisonAt > kPoisonFor ? "time" : c.retreat ? "retreat" : nullptr;
        if (!end) return true;
        c.poisonOn = false; c.poisonEndAt = now;
        const float mx = bot->Location.X - c.poisonFrom.X, my = bot->Location.Y - c.poisonFrom.Y;
        Logger::Log("chat-command", "[PvpBot] ab poison end pawn=%d why=%s moved=%.0f after=%.1f poisoned=%d hp=%d\n",
            c.pawnId, end, std::sqrt(mx * mx + my * my), now - c.poisonAt, (int)pois, bot->Health);
        return false;
    }
    if (!pois || c.retreat || bot->Physics != 1 /*PHYS_Walking*/) return false;
    const float bx = bot->Location.X - c.poisonFrom.X, by = bot->Location.Y - c.poisonFrom.Y;
    const bool again = !fresh && now - c.poisonEndAt >= kPoisonAgain && bx * bx + by * by < 300.0f * 300.0f;
    if (!fresh && !again) return false;
    if (PoisonKnob(now) == 0) return false;
    APawn* src = c.foe && c.lastSight && PawnAlive(c.foe, (APawn*)bot) &&
                 Dist2((APawn*)bot, c.foe) < 1500.0f * 1500.0f ? c.foe : nullptr;
    float ax, ay;
    if (src) { ax = bot->Location.X - src->Location.X; ay = bot->Location.Y - src->Location.Y; }
    else {
        const float yaw = (float)bot->Rotation.Yaw * 6.2831853f / 65536.0f;
        ax = -std::cos(yaw); ay = -std::sin(yaw);
    }
    const float al = std::sqrt(ax * ax + ay * ay);
    if (al < 1.0f) { ax = 1.0f; ay = 0.0f; } else { ax /= al; ay /= al; }
    const float h = bot->CylinderComponent ? bot->CylinderComponent->CollisionHeight : 90.0f;
    static const int kTurns[7] = {0, 45, -45, 90, -90, 135, -135};
    FVector to; int turn = 999;
    for (int k = 0; k < 7 && turn == 999; k++) {
        const float a = kTurns[k] * 0.0174533f, ca = std::cos(a), sa = std::sin(a);
        FVector t = bot->Location;
        t.X += (ax * ca - ay * sa) * kPoisonStep;
        t.Y += (ax * sa + ay * ca) * kPoisonStep;
        if (!PbClear((AActor*)bot, t, bot->Location)) continue;  // wall
        FVector down = t; down.Z -= h + 120.0f;
        if (PbClear((AActor*)bot, down, t)) continue;  // no floor: ledge
        to = t; turn = kTurns[k];
    }
    c.poisonFrom = bot->Location;
    if (turn == 999) {
        c.poisonEndAt = now;
        Logger::Log("chat-command", "[PvpBot] ab poison stuck pawn=%d why=%s at=(%.0f,%.0f,%.0f) hp=%d\n",
            c.pawnId, fresh ? "fresh" : "again", bot->Location.X, bot->Location.Y, bot->Location.Z, bot->Health);
        return false;
    }
    c.poisonOn = true; c.poisonAt = now; c.poisonTo = to; c.poisonN++;
    aic->eventSetWhatToDoNext(238, 289);  // OverrideMovement runs it to poisonTo
    Logger::Log("chat-command", "[PvpBot] ab poison step pawn=%d why=%s src=%s turn=%d at=(%.0f,%.0f,%.0f) to=(%.0f,%.0f) hp=%d n=%d\n",
        c.pawnId, fresh ? "fresh" : "again", src ? "foe" : "back", turn, bot->Location.X, bot->Location.Y, bot->Location.Z,
        to.X, to.Y, bot->Health, c.poisonN);
    return true;
}

// Blocked by an enemy force field (FfHold < 0.6 s ago) within kFfNear
// -> walk to the nearer clear side of it (field centre +- (r + kFfSide)
// across the line to the foe, floor under it), at most kFfFor s, then fire
// resumes by itself once the line is clear (a foe right behind / inside the
// field: the bot circles to its edge). Not while retreating. Logs `ab ffwall route|end|stuck`.
bool TickFfWall(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!aic->WorldInfo) return false;
    const float now = aic->WorldInfo->TimeSeconds;
    if (c.ffOn) {
        const float dx = bot->Location.X - c.ffTo.X, dy = bot->Location.Y - c.ffTo.Y;
        const char* end = dx * dx + dy * dy < 90.0f * 90.0f ? "there"
                        : now - c.ffAt > kFfFor ? "time" : c.retreat ? "retreat"
                        : (!c.foe || c.foe->Health <= 0) ? "foe" : nullptr;
        if (!end) return true;
        c.ffOn = false; c.ffEndAt = now;
        FVector eye = bot->Location; eye.Z += bot->BaseEyeHeight;
        const bool clear = c.foe && c.foe->Health > 0 && !EnemyFieldBetween((APawn*)bot, eye, c.foe->Location, nullptr);
        const float mx = bot->Location.X - c.ffFrom.X, my = bot->Location.Y - c.ffFrom.Y;
        Logger::Log("chat-command", "[PvpBot] ab ffwall end pawn=%d why=%s moved=%.0f after=%.1f clear=%d hp=%d\n",
            c.pawnId, end, std::sqrt(mx * mx + my * my), now - c.ffAt, (int)clear, bot->Health);
        return false;
    }
    if (now - c.ffBlockAt > 0.6f || now - c.ffEndAt < kFfAgain) return false;
    if (FfWallKnob(now) < 2 || c.retreat || bot->Physics != 1 /*PHYS_Walking*/) return false;
    if (!c.foe || !PawnAlive(c.foe, (APawn*)bot)) return false;
    FVector eye = bot->Location; eye.Z += bot->BaseEyeHeight;
    float r = 200.0f;
    ATgDeployable* w = EnemyFieldBetween((APawn*)bot, eye, c.foe->Location, &r);
    if (!w) return false;
    const float wx = w->Location.X - bot->Location.X, wy = w->Location.Y - bot->Location.Y;
    if (wx * wx + wy * wy > kFfNear * kFfNear) return false;  // far: just hold fire
    float ux = c.foe->Location.X - bot->Location.X, uy = c.foe->Location.Y - bot->Location.Y;
    const float ul = std::sqrt(ux * ux + uy * uy);
    if (ul < 1.0f) return false;
    ux /= ul; uy /= ul;
    const float h = bot->CylinderComponent ? bot->CylinderComponent->CollisionHeight : 90.0f;
    const float off = r + kFfSide;
    FVector cand[2];
    for (int k = 0; k < 2; k++) {
        const float sgn = k ? -1.0f : 1.0f;
        cand[k] = w->Location;
        cand[k].X += -uy * sgn * off; cand[k].Y += ux * sgn * off; cand[k].Z = bot->Location.Z;
    }
    auto d2 = [&](const FVector& v) { const float x = v.X - bot->Location.X, y = v.Y - bot->Location.Y; return x * x + y * y; };
    if (d2(cand[1]) < d2(cand[0])) { FVector t = cand[0]; cand[0] = cand[1]; cand[1] = t; }
    int pick = -1;
    for (int k = 0; k < 2 && pick < 0; k++) {
        if (!PbClear((AActor*)bot, cand[k], bot->Location)) continue;  // world wall
        FVector down = cand[k]; down.Z -= h + 120.0f;
        if (PbClear((AActor*)bot, down, cand[k])) continue;  // no floor: ledge
        pick = k;
    }
    c.ffFrom = bot->Location;
    if (pick < 0) {
        c.ffEndAt = now;
        Logger::Log("chat-command", "[PvpBot] ab ffwall stuck pawn=%d dep=%d r=%.0f at=(%.0f,%.0f,%.0f) hp=%d\n",
            c.pawnId, w->r_nDeployableId, r, bot->Location.X, bot->Location.Y, bot->Location.Z, bot->Health);
        return false;
    }
    c.ffOn = true; c.ffAt = now; c.ffTo = cand[pick]; c.ffN++;
    aic->eventSetWhatToDoNext(238, 289);  // OverrideMovement runs it to ffTo
    Logger::Log("chat-command", "[PvpBot] ab ffwall route pawn=%d dep=%d r=%.0f side=%d wallDist=%.0f at=(%.0f,%.0f,%.0f) to=(%.0f,%.0f) hp=%d n=%d\n",
        c.pawnId, w->r_nDeployableId, r, pick, std::sqrt(wx * wx + wy * wy), bot->Location.X, bot->Location.Y, bot->Location.Z,
        c.ffTo.X, c.ffTo.Y, bot->Health, c.ffN);
    return true;
}

bool TickCargo(ATgAIController* aic, ATgPawn* bot, Chaser& c, bool hadFoe) {
    const int ctf = AvaCargoDll::BotCarrierTf();
    if (!ctf || !aic->WorldInfo) { c.cargoPack = nullptr; c.cargoDep = 0; return false; }
    ATgRepInfo_TaskForce* tf = TfOf((APawn*)bot);
    if (!tf || (int)tf->r_nTaskForce != ctf) return false;
    const float now = aic->WorldInfo->TimeSeconds;
    if (c.cargoDep) {  // carrying
        if (c.cargoWind > 0.0f) {  // tell running
            aic->Enemy = nullptr;
            bot->Acceleration = FVector(0, 0, 0);
            if (now < c.cargoWind) return true;
            c.cargoWind = 0.0f;
            const float yaw = bot->Rotation.Yaw * 3.14159265f / 32768.0f;
            FVector start = {bot->Location.X + std::cos(yaw) * 120.0f, bot->Location.Y + std::sin(yaw) * 120.0f, bot->Location.Z + 60.0f};
            FVector end = {start.X, start.Y, start.Z - 700.0f};
            FVector spot = {0, 0, 0}, n = {0, 0, 1};
            FTraceHitInfo hi;
            const bool ground = bot->Trace(end, start, 0, FVector(0, 0, 0), 0, &spot, &n, &hi) != nullptr;
            if (!ground) {
                spot = bot->Location; n = FVector(0, 0, 1);
                if (bot->CylinderComponent) spot.Z -= bot->CylinderComponent->CollisionHeight;
            }
            ATgDeployable* d = TgProj_Deployable__SpawnDeployable::SpawnDeployableActor(bot, c.cargoDep, spot, n, nullptr, nullptr);
            if (d) c.cargoN++;
            Logger::Log("chat-command", "[PvpBot] ab cargo use pawn=%d ok=%d dep=%d ground=%d at=(%.0f,%.0f,%.0f) obj=%d hp=%d n=%d\n",
                c.pawnId, (int)(d != nullptr), c.cargoDep, (int)ground, spot.X, spot.Y, spot.Z,
                c.objective ? c.objective->nObjectiveId : -1, d ? d->r_nHealth : 0, c.cargoN);
            c.cargoDep = 0;
            if (hadFoe && c.foe) aic->Enemy = c.foe;
            return false;
        }
        bool place = now - c.cargoSince > 20.0f && !c.objective;
        const char* why = "carried";
        if (c.objective) {
            AActor* g = ObjectiveGoalActor(c.objective);
            const float gx = bot->Location.X - g->Location.X, gy = bot->Location.Y - g->Location.Y;
            if (c.objOn || gx * gx + gy * gy < kCargoNear * kCargoNear) { place = true; why = "objective"; }
        }
        if (!place) return false;  // keep playing; objective play walks it there
        c.cargoWind = now + kCargoWind;
        ATgDevice* cur = (ATgDevice*)bot->Weapon;
        if (cur && bot->IsFiring()) { bot->StopAction(cur, 0); bot->StopAction(cur, 1); }
        aic->Enemy = nullptr;
        bot->Acceleration = FVector(0, 0, 0);
        Logger::Log("chat-command", "[PvpBot] ab cargo windup pawn=%d why=%s dep=%d carried=%.1f\n", c.pawnId, why, c.cargoDep, now - c.cargoSince);
        return true;
    }
    if (c.cargoPack) {  // pickup run
        if (!AvaCargoDll::BotPackLive(c.cargoPack)) { c.cargoPack = nullptr; Logger::Log("chat-command", "[PvpBot] ab cargo gone pawn=%d\n", c.pawnId); return false; }
        if (hadFoe && c.lastSight) { EndCargoRun(c, "foe seen"); return false; }
        if (now - c.cargoRunAt > 40.0f) { EndCargoRun(c, "stalled"); c.cargoLookAt = now + 20.0f; return false; }
        const float dx = bot->Location.X - c.cargoAt.X, dy = bot->Location.Y - c.cargoAt.Y;
        if (dx * dx + dy * dy < kCargoTake * kCargoTake) {
            const int dep = AvaCargoDll::BotTakePack(c.cargoPack);
            c.cargoPack = nullptr;
            Logger::Log("chat-command", "[PvpBot] ab cargo pickup pawn=%d ok=%d dep=%d\n", c.pawnId, (int)(dep > 0), dep);
            if (dep > 0) { c.cargoDep = dep; c.cargoSince = now; }
            return false;
        }
        if (bot->bIsWalking) bot->bIsWalking = 0;
        aic->eventSetWhatToDoNext(238, 289);  // OverrideMovement aims it at the pack
        return true;
    }
    if (hadFoe && c.lastSight) return false;
    if (now - c.cargoLookAt < 2.0f) return false;
    c.cargoLookAt = now;
    if (CargoKnob(now) == 0) return false;
    ATgDeployable* p = AvaCargoDll::BotClaimPack(ctf, bot->Location, kCargoFind, c.pawnId, now);
    if (!p) return false;
    c.cargoPack = p; c.cargoAt = p->Location; c.cargoRunAt = now; c.route.clear(); c.routeGoal = nullptr;
    if (c.objective) EndObjective(aic, c, "cargo");
    Logger::Log("chat-command", "[PvpBot] ab cargo run pawn=%d dep=%d dist=%.0f\n", c.pawnId, p->r_nDeployableId,
        std::sqrt((bot->Location.X - p->Location.X) * (bot->Location.X - p->Location.X) + (bot->Location.Y - p->Location.Y) * (bot->Location.Y - p->Location.Y)));
    if (bot->bIsWalking) bot->bIsWalking = 0;
    aic->eventSetWhatToDoNext(238, 289);
    return true;
}
bool TickObjective(ATgAIController* aic, ATgPawn* bot, Chaser& c, bool hadFoe, float dt) {
    ObjectiveScanLog(bot->WorldInfo);
    const float fd2 = hadFoe ? Dist2((APawn*)bot, c.foe) : 1.0e18f;
    bool cap = false;
    ATgMissionObjective* o = PickObjective(bot, &cap);
    if (!o) { if (c.objective) EndObjective(aic, c, "none"); return false; }
    AActor* g = ObjectiveGoalActor(o);
    const float gx = bot->Location.X - g->Location.X, gy = bot->Location.Y - g->Location.Y;
    const float gd = std::sqrt(gx * gx + gy * gy);
    const bool on = gd < HoldRadius(o) + 150.0f;
    // fight instead: foe seen, or close (closer still when we hold the point)
    const float fightR = on ? kChaseFar : kObjFoeFar;
    if (hadFoe && (c.lastSight || fd2 < fightR * fightR)) {
        if (c.objective) EndObjective(aic, c, c.lastSight ? "foe seen" : "foe near");
        return false;
    }
    if (o != c.objective) {
        c.objective = o; c.objOn = false; c.objClock = 0.0f; c.route.clear(); c.routeGoal = nullptr;
        c.rally = false; c.rallyDone = false;
        c.progBest = 1.0e18f; c.progClock = 0.0f; c.stillFor = 0.0f;
        c.noDirect = false;
        const std::string nm = g->GetName();
        Logger::Log("chat-command", "[PvpBot] objective start pawn=%d id=%d mode=%s goal=%s dist=%.0f owner=%d\n",
            c.pawnId, o->nObjectiveId, cap ? "capture" : "defend", nm.c_str(), gd, o->r_nOwnerTaskForce);
    }
    if (on != c.objOn) {
        c.objOn = on;
        if (on) Logger::Log("chat-command", "[PvpBot] objective arrive pawn=%d id=%d dist=%.0f\n",
                            c.pawnId, o->nObjectiveId, gd);
    }
    c.objClock += dt;
    if (c.objClock >= 6.0f) {
        c.objClock = 0.0f;
        Logger::Log("chat-command",
            "[PvpBot] objective pawn=%d id=%d on=%d dist=%.0f owner=%d status=%d capT=%.1f/%.1f mCap=%.1f speed=%.0f\n",
            c.pawnId, o->nObjectiveId, (int)on, gd, o->r_nOwnerTaskForce, (int)o->r_eStatus,
            o->r_fCurrCaptureTime, o->m_fTimeToCapture, o->m_fCurrCaptureTime,
            std::sqrt(bot->Velocity.X * bot->Velocity.X + bot->Velocity.Y * bot->Velocity.Y));
    }
    TickRally(aic, bot, c, o, g, gd, cap, on, dt);
    TickUnstick(bot, c, gd, on, dt);
    if (bot->bIsWalking) bot->bIsWalking = 0;
    if (!on && !c.rally) MaybeStartJet(bot, c);
    aic->eventSetWhatToDoNext(238, 289);
    return true;
}
FVector ObjectiveTarget(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    AActor* g = ObjectiveGoalActor(c.objective);
    const FVector goal = g->Location;
    if (c.rally) { c.viaPath = false; return bot->Location; }  // Hold for teammates
    const float dx = bot->Location.X - goal.X, dy = bot->Location.Y - goal.Y;
    if (dx * dx + dy * dy < HoldRadius(c.objective) * HoldRadius(c.objective)) {
        c.viaPath = false; return bot->Location;  // on the point: hold
    }
    // Push_Dust_P attacker start: ActorReachable said yes
    // for the payload robot 4300 uu away behind walls, bot pressed into the
    // spawn wall for 50 s; after a stall (noDirect) only the route counts
    if (!c.noDirect && aic->ActorReachable(g)) { c.viaPath = false; return goal; }
    const int net = bot->Anchor ? bot->Anchor->NetworkID : -1;
    ANavigationPoint* nav = NearestLinkedNav((AActor*)bot, goal, net, nullptr, nullptr, nullptr);
    // Goal on another path network (Trafalgar koth) -> no
    // node of OUR net near it -> nav null -> straight run forever. Take the
    // goal's own node; OwnRouteNext then plans with jump links.
    if (!nav && net >= 0) nav = NearestLinkedNav((AActor*)bot, goal, -1, nullptr, nullptr, nullptr);
    AActor* next = nav ? OwnRouteNext(aic, bot, c, nav, &goal) : nullptr;
    if (!next) { c.viaPath = false; return goal; }
    c.viaPath = true;
    c.lastWaypoint = next;
    return next->Location;
}

// Pickup run - direct when reachable, else the nav route (same as ObjectiveTarget)
FVector CargoTarget(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    const FVector goal = c.cargoAt;
    if (aic->ActorReachable((AActor*)c.cargoPack)) { c.viaPath = false; return goal; }
    const int net = bot->Anchor ? bot->Anchor->NetworkID : -1;
    ANavigationPoint* nav = NearestLinkedNav((AActor*)bot, goal, net, nullptr, nullptr, nullptr);
    if (!nav && net >= 0) nav = NearestLinkedNav((AActor*)bot, goal, -1, nullptr, nullptr, nullptr);
    AActor* next = nav ? OwnRouteNext(aic, bot, c, nav, &goal) : nullptr;
    if (!next) { c.viaPath = false; return goal; }
    c.viaPath = true;
    c.lastWaypoint = next;
    return next->Location;
}

// Strafing + dodge. Holding range with the foe in sight the donor
// behaviour stood still (an easy target). Players side-step: a point
// 180-300 uu to the side of the foe line (plus 120 uu in/out to stay at
// 600-1200 uu), sides flip at random, low skill pauses more. The spot must be
// clear at body height and have floor under it (never steps off a ledge).
// Hit while on the ground -> skill-based chance of a sideways hop.
float Rand01() { return (float)std::rand() / (float)RAND_MAX; }

bool SideSpot(ATgPawn* bot, Chaser& c, int side, float step, float fwd, FVector& out) {
    const float fx = c.foe->Location.X - bot->Location.X, fy = c.foe->Location.Y - bot->Location.Y;
    const float l = std::sqrt(fx * fx + fy * fy);
    if (l < 1.0f) return false;
    const float ux = fx / l, uy = fy / l;
    FVector t = bot->Location;
    t.X += -uy * side * step + ux * fwd;
    t.Y +=  ux * side * step + uy * fwd;
    if (!PbClear((AActor*)bot, t, bot->Location)) return false;  // wall / pillar
    const float h = bot->CylinderComponent ? bot->CylinderComponent->CollisionHeight : 90.0f;
    FVector down = t; down.Z -= h + 120.0f;
    if (PbClear((AActor*)bot, down, t)) return false;  // no floor: ledge
    out = t;
    return true;
}

bool TickStrafe(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!aic->WorldInfo || !c.foe || !c.lastSight || bot->Physics != 1 /*PHYS_Walking*/ || c.jetting ||
        c.scoped /* a sniper holds still in scope */) {
        c.strafeOn = false;
        return false;
    }
    const float now = aic->WorldInfo->TimeSeconds;
    if (now < c.strafeUntil) return c.strafeOn;
    const float s = c.skill < 0.0f ? 0.5f : c.skill;
    if (c.strafeOn && Rand01() < 0.45f - 0.35f * s) {  // pause (easy bots stand more)
        c.strafeOn = false;
        c.strafeUntil = now + 0.5f + 0.8f * (1.0f - s);
        return false;
    }
    const float fx = c.foe->Location.X - bot->Location.X, fy = c.foe->Location.Y - bot->Location.Y;
    const float d = std::sqrt(fx * fx + fy * fy);
    const float fwd = d > 1200.0f ? 120.0f : (d < 600.0f ? -120.0f : 0.0f);
    const float step = 180.0f + 120.0f * Rand01();
    int side = Rand01() < 0.7f ? -c.strafeSide : c.strafeSide;
    FVector t;
    if (!SideSpot(bot, c, side, step, fwd, t)) {
        side = -side;
        if (!SideSpot(bot, c, side, step, fwd, t)) {
            c.strafeOn = false; c.strafeBlocked++;
            c.strafeUntil = now + 1.0f;
            return false;
        }
    }
    c.strafeSide = side; c.strafeTo = t; c.strafeOn = true;
    c.strafeUntil = now + 0.5f + 0.6f * Rand01() + 0.4f * (1.0f - s);
    if (c.strafes++ % 4 == 0)
        Logger::Log("chat-command", "[PvpBot] strafe pawn=%d n=%d side=%d dist=%.0f blocked=%d skill=%.2f\n",
            c.pawnId, c.strafes, side, d, c.strafeBlocked, s);
    return true;
}

// cover-to-cover. Holding range with sight and getting hit -> run to
// the nearest linked path node 250-900 uu away that the foe's eyes can't see
// (straight walkable line from the bot, about the same height, not toward
// the foe), hide 1.2-3 s (easy bots longer), then step back out (strafe
// resumes and re-acquires sight). Ends why=peek|timeout|foe|chase|melee.
void EndCover(Chaser& c, const char* why, float now) {
    if (!c.coverOn) return;
    c.coverOn = false; c.coverHide = false; c.coverEndAt = now;
    Logger::Log("chat-command", "[PvpBot] cover end pawn=%d n=%d why=%s\n", c.pawnId, c.covers, why);
}

bool PickCoverNode(ATgPawn* bot, Chaser& c, float* outD, FVector& outP, int* nCand, int* nSeen) {
    const FVector b = bot->Location, f = c.foe->Location;
    const float fdx = f.X - b.X, fdy = f.Y - b.Y;
    const float fd = std::sqrt(fdx * fdx + fdy * fdy);
    if (fd < 1.0f || !bot->WorldInfo) return false;
    const int net = bot->Anchor ? bot->Anchor->NetworkID : -1;
    std::vector<std::pair<float, ANavigationPoint*>> cand;
    int n = 0;
    for (ANavigationPoint* nav = bot->WorldInfo->NavigationPointList; nav && n < 20000; nav = nav->nextNavigationPoint, n++) {
        if (nav->bDeleteMe || nav->PathList.Num() == 0) continue;
        if (net >= 0 && nav->NetworkID != net) continue;
        if (IsAirNode((AActor*)nav)) continue;
        if (std::find(c.avoid.begin(), c.avoid.end(), nav) != c.avoid.end()) continue;
        const float rx = nav->Location.X - b.X, ry = nav->Location.Y - b.Y, rz = nav->Location.Z - b.Z;
        const float d2 = rx * rx + ry * ry;
        if (d2 < 250.0f * 250.0f || d2 > 900.0f * 900.0f || std::fabs(rz) > 160.0f) continue;
        const float along = (rx * fdx + ry * fdy) / fd;
        if (along > 0.35f * std::sqrt(d2)) continue;  // not toward the foe
        cand.push_back(std::make_pair(d2, nav));
    }
    std::sort(cand.begin(), cand.end(), [](const std::pair<float, ANavigationPoint*>& x, const std::pair<float, ANavigationPoint*>& y) { return x.first < y.first; });
    FVector feye = f; feye.Z += c.foe->BaseEyeHeight;
    FVector beye = b; beye.Z += 40.0f;
    if (nCand) *nCand = (int)cand.size();
    for (size_t i = 0; i < cand.size() && i < 12; i++) {
        FVector e = cand[i].second->Location; e.Z += 50.0f;
        if (PbClear((AActor*)bot, e, feye)) { if (nSeen) (*nSeen)++; continue; }  // foe sees it: no cover
        if (!PbClear((AActor*)bot, e, beye)) continue; // can't run straight there
        if (outD) *outD = std::sqrt(cand[i].first);
        outP = cand[i].second->Location;
        return true;
    }
    // Path nodes are sparse - sample floor spots round the bot (16 dirs x 3 radii)
    // that are not toward the foe, reachable in a straight line, with floor, out of the foe's sight.
    const float h = bot->CylinderComponent ? bot->CylinderComponent->CollisionHeight : 90.0f;
    const float radii[3] = {350.0f, 550.0f, 750.0f};
    for (int ri = 0; ri < 3; ri++) {
        const int a0 = (int)(Rand01() * 16.0f);
        for (int k = 0; k < 16; k++) {
            const float ang = (float)((a0 + k) % 16) * 0.3926991f;
            const float dx = std::cos(ang), dy = std::sin(ang);
            if (dx * fdx / fd + dy * fdy / fd > 0.35f) continue;  // not toward the foe
            FVector t = b; t.X += dx * radii[ri]; t.Y += dy * radii[ri];
            if (!PbClear((AActor*)bot, t, b)) continue;              // wall in the way
            // Real floor under the spot (a ramp may put t inside the floor)
            FVector top = t; top.Z += h + 60.0f;
            if (!PbClear((AActor*)bot, top, t)) continue;            // low ceiling / inside geometry
            FVector down = t; down.Z -= h + 120.0f;
            FVector hl, hn; FTraceHitInfo hi; std::memset(&hi, 0, sizeof(hi));
            if (!((AActor*)bot)->Trace(down, top, 0, FVector(0, 0, 0), 0, &hl, &hn, &hi)) continue;  // no floor: ledge
            if (hn.Z < 0.7f) continue;                               // steep / wall face
            t.Z = hl.Z + h;                                          // pawn centre standing there
            if (std::fabs(t.Z - b.Z) > 160.0f) continue;
            FVector e = t; e.Z += 40.0f;  // same height as the cover hide check
            if (PbClear((AActor*)bot, e, feye)) { if (nSeen) (*nSeen)++; continue; }
            if (outD) *outD = radii[ri];
            outP = t;
            return true;
        }
    }
    return false;
}

// true = cover owns the movement this tick (OverrideMovement uses coverTo)
bool TickCover(ATgAIController* aic, ATgPawn* bot, Chaser& c, bool wants) {
    if (!aic->WorldInfo) return false;
    const float now = aic->WorldInfo->TimeSeconds;
    if (c.coverOn) {
        if (!c.foe || c.foe != c.coverFoe) { EndCover(c, "foe", now); return false; }
        if (wants) { EndCover(c, "chase", now); return false; }
        if (c.meleeOn) { EndCover(c, "melee", now); return false; }
        if (!c.coverHide) {
            const float dx = bot->Location.X - c.coverTo.X, dy = bot->Location.Y - c.coverTo.Y;
            // Hide as soon as the foe's eyes lose the bot on the way (spot picked at start may be off)
            const bool hid = !PbClear((AActor*)bot, FVector(bot->Location.X, bot->Location.Y, bot->Location.Z + 40.0f),
                FVector(c.foe->Location.X, c.foe->Location.Y, c.foe->Location.Z + c.foe->BaseEyeHeight));
            const bool there = dx * dx + dy * dy < 90.0f * 90.0f;
            if (there || (hid && now - c.coverAt > 0.3f)) {
                const float s = c.skill < 0.0f ? 0.5f : c.skill;
                c.coverHide = true;
                c.coverUntil = now + 1.2f + 1.0f * Rand01() + 0.8f * (1.0f - s);
                if (!there) c.coverTo = bot->Location;  // hold here
                {
                    const float mx = c.foe->Location.X - c.coverFoeAt.X, my = c.foe->Location.Y - c.coverFoeAt.Y, mz = c.foe->Location.Z - c.coverFoeAt.Z;
                    FVector fe = c.foe->Location; fe.Z += c.foe->BaseEyeHeight;
                    FVector sp = c.coverTo; sp.Z += 40.0f;
                    Logger::Log("chat-command", "[PvpBot] cover hide pawn=%d n=%d took=%.1f hidden=%d there=%d foeMoved=%.0f spotHid=%d dz=%.0f\n", c.pawnId, c.covers,
                        now - c.coverAt, (int)hid, (int)there, std::sqrt(mx * mx + my * my + mz * mz),
                        (int)!PbClear((AActor*)bot, sp, fe), bot->Location.Z - c.coverTo.Z);
                }
            } else if (now > c.coverUntil) { EndCover(c, "timeout", now); return false; }
        } else if (now > c.coverUntil) {
            EndCover(c, "peek", now);
            c.strafeUntil = -100.0f;  // step straight back out
            return false;
        }
        return true;
    }
    if (wants || c.meleeOn || c.scoped || c.retreat || c.jetting || !c.foe || !c.lastSight || bot->Physics != 1) return false;
    if (now - c.lastHitAt > 1.5f || now - c.coverEndAt < 7.0f) return false;
    const float s = c.skill < 0.0f ? 0.5f : c.skill;
    if (s < 0.25f) return false;
    if ((float)bot->Health > 0.8f * (float)MaxHp((APawn*)bot)) return false;
    if (Rand01() > 0.3f + 0.5f * s) { c.coverEndAt = now - 5.0f; return false; }  // not this time; retry in 2 s
    float d = 0.0f;
    FVector spot = {0, 0, 0};
    int nCand = 0, nSeen = 0;
    const bool got = PickCoverNode(bot, c, &d, spot, &nCand, &nSeen);
    c.covers++;
    if (!got) {
        c.coverEndAt = now - 4.0f;
        if (c.covers % 3 == 1) Logger::Log("chat-command", "[PvpBot] cover skip pawn=%d n=%d why=nonode nodes=%d seen=%d\n", c.pawnId, c.covers, nCand, nSeen);
        return false;
    }
    c.coverOn = true; c.coverHide = false; c.coverFoe = c.foe; c.coverAt = now; c.coverFoeAt = c.foe->Location;
    c.coverTo = spot; c.coverUntil = now + 3.0f;
    c.strafeOn = false;
    const float ex = c.foe->Location.X - bot->Location.X, ey = c.foe->Location.Y - bot->Location.Y;
    Logger::Log("chat-command", "[PvpBot] cover start pawn=%d n=%d d=%.0f foeD=%.0f hp=%d skill=%.2f at=(%.0f,%.0f,%.0f)\n",
        c.pawnId, c.covers, d, std::sqrt(ex * ex + ey * ey), bot->Health, s, bot->Location.X, bot->Location.Y, bot->Location.Z);
    return true;
}

void TickDodge(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!aic->WorldInfo || !c.foe) return;
    const float now = aic->WorldInfo->TimeSeconds;
    const int hp = bot->Health, prev = c.hpPrev;
    const bool fresh = now - c.hpAt < 1.0f;
    c.hpPrev = hp; c.hpAt = now;
    if (!fresh || prev <= 0 || hp >= prev) return;
    c.lastHitAt = now;  // Cover trigger
    if (now - c.dodgeAt < 3.0f || bot->Physics != 1 || c.jetting) return;
    if ((float)(prev - hp) < 0.05f * (float)MaxHp((APawn*)bot)) return;
    const float s = c.skill < 0.0f ? 0.5f : c.skill;
    if (Rand01() > 0.2f + 0.5f * s) return;
    int side = Rand01() < 0.5f ? -1 : 1;
    FVector t;
    if (!SideSpot(bot, c, side, 260.0f, 0.0f, t)) {
        side = -side;
        if (!SideSpot(bot, c, side, 260.0f, 0.0f, t)) return;
    }
    const float vx = t.X - bot->Location.X, vy = t.Y - bot->Location.Y;
    const float l = std::sqrt(vx * vx + vy * vy);
    if (l < 1.0f) return;
    FVector v; v.X = vx / l * 420.0f; v.Y = vy / l * 420.0f; v.Z = 330.0f;
    bot->SetPhysics(2 /*PHYS_Falling*/);
    bot->Velocity = v;
    c.dodgeAt = now; c.dodges++;
    c.strafeSide = side;
    Logger::Log("chat-command", "[PvpBot] dodge pawn=%d n=%d side=%d hp=%d->%d skill=%.2f\n",
        c.pawnId, c.dodges, side, prev, hp, s);
}

// Weapon by range. Every class kit carries a melee device (equip
// point 1) and a ranged one (point 2); the donor behaviour only ever used
// the gun. A foe inside kMeleeIn with sight -> switch to melee and run into
// him (swing inside kMeleeHit); past kMeleeOut / foe lost -> back to the gun.
// Switching uses the medic-beam route (stop, StopDevices, SetActiveWeapon,
// retried each tick; a refused try drops Enemy for the next one).
constexpr float kMeleeIn  = 300.0f;
constexpr float kMeleeOut = 520.0f;
constexpr float kMeleeHit = 230.0f;

bool WantDevice(ATgAIController* aic, ATgPawn* bot, Chaser& c, int point, const char* why) {
    ATgDevice* want = bot->m_EquippedDevices[point];
    if (!want) return false;
    ATgDevice* cur = (ATgDevice*)bot->Weapon;
    const float now = aic->WorldInfo ? aic->WorldInfo->TimeSeconds : 0.0f;
    if (cur == want) {
        if (c.swFailSince > 0.0f) c.swFailSince = -100.0f;
        if (!aic->Enemy && c.foe && PawnAlive(c.foe, bot)) {
            aic->Enemy = c.foe;
            aic->SetTargetActor((AActor*)c.foe);
            aic->m_bInterrupt = 1;
        }
        return true;
    }
    if (cur) { bot->StopAction(cur, 0); bot->StopAction(cur, 1); }
    aic->StopDevices();
    const bool sw = bot->eventSetActiveWeapon((AWeapon*)want, 0, 1);
    const bool now_ok = bot->Weapon == (AWeapon*)want;
    if (now_ok) {
        c.swaps++;
        c.swFailSince = -100.0f;
    } else {
        c.swFail++;
        if (c.swFailSince < 0.0f) c.swFailSince = now;
        aic->Enemy = nullptr;  // heal path: the switch takes once the AI stops firing
    }
    if (now_ok || c.swFail % 6 == 1)
        Logger::Log("chat-command", "[PvpBot] weapon pawn=%d to=%s(%d) why=%s sw=%d ok=%d swaps=%d fail=%d dist=%.0f loc=(%.0f,%.0f,%.0f)\n",
            c.pawnId, point == 1 ? "melee" : "ranged", want->r_nDeviceId, why, (int)sw, (int)now_ok,
            c.swaps, c.swFail, c.foe ? std::sqrt(Dist2((APawn*)bot, c.foe)) : -1.0f,
            bot->Location.X, bot->Location.Y, bot->Location.Z);
    return now_ok;
}

// Sniper scope = the replicated pawn flag the player's
// ServerUpdateSnipeScopeMode sets; on while holding range with sight and the
// rifle (point 2) wanted, off for chase / melee / cloak / heal / objective.
void TickScope(ATgAIController* aic, ATgPawn* bot, Chaser& c, bool hadFoe) {
    if (!c.sniper) return;
    const bool want = hadFoe && c.foe && !c.chasing && c.lastSight && !c.meleeOn &&
        !c.cloaked && !c.patient && !c.retreat && !c.objective && !c.jetting &&
        bot->Physics == 1 /*PHYS_Walking*/;
    if (want == c.scoped) {
        if (want && (c.scopeDiag++ % 2) == 0) {  // ~1 s while scoped
            ATgDevice* w = (ATgDevice*)bot->Weapon;
            Logger::Log("chat-command", "[PvpBot] snipe pawn=%d aim=%d scope=%d accOff=%.4f firing=%d foeHp=%d dist=%.0f\n",
                c.pawnId, (int)bot->r_bAimingMode, (int)bot->r_bIsInSnipeScope,
                w ? w->eventAmountCurrentlyOffOfTargetAccuracy(w->CurrentFireMode) : -1.0f,
                (int)bot->IsFiring(), c.foe ? c.foe->Health : -1, std::sqrt(Dist2((APawn*)bot, c.foe)));
        }
        return;
    }
    c.scoped = want;
    // the player's own server calls (aim mode = Ballista m_bRequireAimMode gate)
    bot->ServerUpdateAimingMode(want ? 1 : 0);
    bot->ServerUpdateSnipeScopeMode(want ? 1 : 0);
    bot->r_bAimingMode = want ? 1 : 0;
    bot->r_bIsInSnipeScope = want ? 1 : 0;
    if (want) ++c.scopes;
    Logger::Log("chat-command", "[PvpBot] scope %s pawn=%d n=%d dist=%.0f weapon=%d\n",
        want ? "in" : "out", c.pawnId, c.scopes,
        c.foe ? std::sqrt(Dist2((APawn*)bot, c.foe)) : -1.0f,
        bot->Weapon ? ((ATgDevice*)bot->Weapon)->r_nDeviceId : 0);
}

void TickWeapon(ATgAIController* aic, ATgPawn* bot, Chaser& c) {
    if (!aic->WorldInfo || c.cloaked || c.patient) return;
    const float now = aic->WorldInfo->TimeSeconds;
    const bool foeUp = c.foe && PawnAlive(c.foe, bot);
    const float d = foeUp ? std::sqrt(Dist2((APawn*)bot, c.foe)) : 1.0e9f;
    bool want = c.meleeOn;
    if (!foeUp || d > kMeleeOut) want = false;
    else if (d < kMeleeIn && c.lastSight && std::fabs(bot->Location.Z - c.foe->Location.Z) < 160.0f) want = true;
    // a switch that keeps getting refused for 3 s: stay on the gun 5 s
    if (want && c.swFailSince > 0.0f && now - c.swFailSince > 3.0f) {
        want = false;
        c.weaponAt = now + 5.0f;
    }
    if (want != c.meleeOn && (want ? now >= c.weaponAt : true)) {
        c.meleeOn = want;
        c.weaponAt = now;
        c.swFailSince = -100.0f;
        c.strafeOn = false;
    }
    if (c.meleeOn && c.scoped) {  // A scoped rifle refuses the switch - drop the scope first
        c.scoped = false;
        bot->ServerUpdateAimingMode(0);
        bot->ServerUpdateSnipeScopeMode(0);
        bot->r_bAimingMode = 0;
        bot->r_bIsInSnipeScope = 0;
        Logger::Log("chat-command", "[PvpBot] scope out pawn=%d n=%d why=melee\n", c.pawnId, c.scopes);
    }
    const bool held = WantDevice(aic, bot, c, c.meleeOn ? 1 : 2, c.meleeOn ? "close" : "range");
    if (!c.meleeOn || !held) return;
    // swing: the behaviour's fire check is tuned for the gun - press it too
    ATgDevice* m = (ATgDevice*)bot->Weapon;
    if (d < kMeleeHit && !bot->IsFiring()) {
        bool ok = bot->StartAction(m, 0);
        if (!ok) ok = bot->StartAction(m, 1);
        if (ok && c.swings++ % 5 == 0)
            Logger::Log("chat-command", "[PvpBot] melee swing pawn=%d n=%d dist=%.0f foeHp=%d\n",
                c.pawnId, c.swings, d, c.foe->Health);
    } else if (d >= kMeleeHit && bot->IsFiring()) {
        bot->StopAction(m, 0); bot->StopAction(m, 1);
    }
}

void OverrideMovement(ATgAIController* aic, int* nMovementCode, int* nMoveDestination) {
    if (g_chasers.empty() || !aic || !nMovementCode || !nMoveDestination) return;
    auto it = g_chasers.find(aic);
    if (it == g_chasers.end()) return;
    ATgPawn* bot = (ATgPawn*)aic->Pawn;
    if (!bot || bot->Health <= 0 || bot->r_nPawnId != it->second.pawnId) {
        g_chasers.erase(it);
        return;
    }
    if (it->second.medHold > 0.0f) {  // Stay on its Medical Station while it heals
        aic->m_vTriggerLocation = it->second.medSpot;
        *nMovementCode    = 238;
        *nMoveDestination = 289;
        return;
    }
    if (it->second.retreat) {  // Falling back to heal
        aic->m_vTriggerLocation = RetreatTarget(aic, bot, it->second);
        *nMovementCode    = 238;
        *nMoveDestination = 289;
        return;
    }
    if (it->second.poisonOn) {  // Stepping out of poison
        aic->m_vTriggerLocation = it->second.poisonTo;
        *nMovementCode    = 238;
        *nMoveDestination = 289;
        return;
    }
    if (it->second.ffOn) {  // Around an enemy force field
        aic->m_vTriggerLocation = it->second.ffTo;
        *nMovementCode    = 238;
        *nMoveDestination = 289;
        return;
    }
    if (it->second.patient) {  // Medic busy - run to the patient or hold
        if (!it->second.healRun) return;
        aic->m_vTriggerLocation = it->second.patient->Location;
        *nMovementCode    = 238;
        *nMoveDestination = 289;
        return;
    }
    if (it->second.cargoPack && AvaCargoDll::BotPackLive(it->second.cargoPack)) {  // Run to the AvA cargo pack
        aic->m_vTriggerLocation = CargoTarget(aic, bot, it->second);
        *nMovementCode    = 238;
        *nMoveDestination = 289;
        return;
    }
    if (it->second.objective) {  // Playing the objective
        it->second.moveCalls++;  // Diag
        aic->m_vTriggerLocation = ObjectiveTarget(aic, bot, it->second);
        *nMovementCode    = 238;
        *nMoveDestination = 289;
        return;
    }
    if (!RefreshFoe(aic, bot, it->second)) return;
    if (it->second.meleeOn) {  // Melee - run into the foe
        aic->m_vTriggerLocation = it->second.foe->Location;
        *nMovementCode    = 238;
        *nMoveDestination = 289;
        return;
    }
    if (!WantsChase(aic, bot, it->second)) {
        if (it->second.coverOn) {  // To cover / hold in cover
            aic->m_vTriggerLocation = it->second.coverHide ? bot->Location : it->second.coverTo;
            *nMovementCode    = 238;
            *nMoveDestination = 289;
            return;
        }
        if (it->second.scoped) {  // Scoped sniper stands its ground
            aic->m_vTriggerLocation = bot->Location;
            *nMovementCode    = 238;
            *nMoveDestination = 289;
            return;
        }
        if (it->second.strafeOn) {  // side-step while holding
            aic->m_vTriggerLocation = it->second.strafeTo;
            *nMovementCode    = 238;
            *nMoveDestination = 289;
        }
        return;
    }
    it->second.strafeOn = false;
    if (*nMovementCode != 238 || *nMoveDestination != 289) {
        if (it->second.rewrites++ % 20 == 0)
            Logger::Log("chat-command", "[PvpBot] chase rewrite pawn=%d pick %d/%d -> 238/289 (#%d)\n",
                it->second.pawnId, *nMovementCode, *nMoveDestination, it->second.rewrites);
    }
    aic->m_vTriggerLocation = ChaseTarget(aic, bot, it->second);
    *nMovementCode    = 238;
    *nMoveDestination = 289;
}

// Dressed bots that come back after death (see RegisterRespawn).
struct RespawnSlot {
    int    slot;
    int    botId;
    std::string cls;
    ATgRepInfo_TaskForce* tf;
    bool   enemySide;
    FVector loc;
    FRotator rot;
    int    pawnId;          // current body (r_nPawnId)
    AController* foeCtrl;
    void*  world;           // WorldInfo at registration (map change -> drop)
    float  deadFor = -1.0f; // <0 = alive
    FVector lastLoc = {0, 0, 0}; // Last seen alive (died line)
    int    respawns = 0;
    DifficultyScalar scalar;  // same HP/damage scaling as the first spawn
    std::string tag;          // Owner tag of a task-force bot ("" = dev bot)
    bool   leave = false;     // Drop at the next death instead of respawning
    float  skill = kDefaultSkill;  // fire pacing 0..1
};
std::vector<RespawnSlot> g_respawn;
int g_nextSlot = 1;
constexpr float kRespawnDelay = 10.0f;  // seconds dead before the bot returns
constexpr int   kMaxRespawns  = 50;     // per slot (dev safety cap)

AWorldInfo* CurWorld() {
    AActor* g = (AActor*)Globals::Get().GGameInfo;
    return g ? g->WorldInfo : nullptr;
}

APawn* LivePawnById(AWorldInfo* wi, int pawnId) {
    for (APawn* q = wi ? wi->PawnList : nullptr; q; q = q->NextPawn) {
        if (!q->bDeleteMe && q->Health > 0 && ((ATgPawn*)q)->r_nPawnId == pawnId) return q;
    }
    return nullptr;
}

bool ControllerPresent(AWorldInfo* wi, AController* c) {
    for (AController* q = wi ? wi->ControllerList : nullptr; q; q = q->NextController) {
        if (q == c) return !q->bDeleteMe;
    }
    return false;
}

// Respawn at the bot's own team spawn points (same filter as the
// player picker TgFindPlayerStart: enabled, task force match or 0, priority
// <= 0 or current), round-robin across the eligible ones. Small per-slot
// offset so several bots on one start do not stack. false = none (keep spot).
int g_startRr = 0;
bool PickTeamStart(int taskForce, int slot, FVector& loc, FRotator& rot, int& mapObj) {
    ActorCache::CacheMapActors();
    ATgGame* game = (ATgGame*)Globals::Get().GGameInfo;
    const int prio = game ? game->s_nCurrentPriority : 0;
    std::vector<ATgTeamPlayerStart*> ok;
    for (ATgTeamPlayerStart* s : ActorCache::PlayerStarts) {
        if (!s || !s->bEnabled) continue;
        if (s->m_nTaskForce != 0 && s->m_nTaskForce != taskForce) continue;
        if (s->m_nPriority > 0 && s->m_nPriority != prio) continue;
        ok.push_back(s);
    }
    if (ok.empty()) return false;
    ATgTeamPlayerStart* s = ok[(size_t)(g_startRr++) % ok.size()];
    const float off = 60.0f * (float)(slot % 4);
    loc = s->Location;
    loc.X += (slot & 1) ? off : -off;
    loc.Y += (slot & 2) ? off : -off;
    rot = s->Rotation;
    mapObj = s->m_nMapObjectId;
    return true;
}

void TickRespawn(float dt) {
    static float s_clock = 0.0f;
    s_clock += dt;
    if (s_clock < 0.5f || g_respawn.empty()) return;
    const float step = s_clock;
    s_clock = 0.0f;
    AWorldInfo* wi = CurWorld();
    for (size_t i = 0; i < g_respawn.size();) {
        RespawnSlot& r = g_respawn[i];
        const char* drop = nullptr;
        if (!wi || (void*)wi != r.world) drop = "map changed";
        else if (r.tf != GTeamsData.Attackers && r.tf != GTeamsData.Defenders) drop = "task force gone";
        else if (r.foeCtrl && !ControllerPresent(wi, r.foeCtrl)) drop = "foe left";
        else if (r.leave && !LivePawnById(wi, r.pawnId)) drop = "left (removed)";
        if (drop) {
            Logger::Log("chat-command", "[PvpBot] respawn slot=%d dropped: %s\n", r.slot, drop);
            BotLooks::Release(r.slot);
            g_respawn.erase(g_respawn.begin() + i);
            continue;
        }
        if (APawn* lp = LivePawnById(wi, r.pawnId)) { r.lastLoc = lp->Location; r.deadFor = -1.0f; ++i; continue; }
        if (r.deadFor < 0.0f) {
            r.deadFor = 0.0f;
            Logger::Log("chat-command", "[PvpBot] died slot=%d pawn=%d class=%s respawn in %.0fs last=(%.0f,%.0f,%.0f) killZ=%.0f\n",
                r.slot, r.pawnId, r.cls.c_str(), kRespawnDelay, r.lastLoc.X, r.lastLoc.Y, r.lastLoc.Z, wi ? wi->KillZ : 0.0f);
        }
        r.deadFor += step;
        APawn* foe = r.foeCtrl ? r.foeCtrl->Pawn : nullptr;
        const bool foeUp = foe && !foe->bDeleteMe && foe->Health > 0;
        // Enemy bots wait for their quarry to be alive; friend bots just come back.
        if (r.deadFor < kRespawnDelay || (r.enemySide && r.foeCtrl && !foeUp)) { ++i; continue; }
        if (++r.respawns > kMaxRespawns && r.tag.empty()) {
            Logger::Log("chat-command", "[PvpBot] respawn slot=%d dropped: cap %d\n", r.slot, kMaxRespawns);
            BotLooks::Release(r.slot);
            g_respawn.erase(g_respawn.begin() + i);
            continue;
        }
        FVector at = r.loc;
        FRotator facing = r.rot;
        int startObj = 0;
        const bool teamStart = PickTeamStart((int)r.tf->r_nTaskForce, r.slot, at, facing, startObj);
        g_dressKey = r.slot;
        g_dressSkill = r.skill;
        ATgPawn* bot = TgPlayerActions::SpawnBotCmd::SpawnDressedAt(
            r.botId, at, facing, r.tf, r.enemySide, r.cls,
            (r.enemySide && foeUp) ? (ATgPawn*)foe : nullptr, r.scalar);
        g_dressKey = 0;
        g_dressSkill = kDefaultSkill;
        if (!bot) {
            // Spot blocked or spawn refused: retry next round.
            r.deadFor = kRespawnDelay - 2.0f;
            Logger::Log("chat-command", "[PvpBot] respawn slot=%d failed (try %d)\n", r.slot, r.respawns);
            ++i;
            continue;
        }
        r.pawnId = bot->r_nPawnId;
        r.deadFor = -1.0f;
        Logger::Log("chat-command",
            "[PvpBot] respawned slot=%d class=%s side=%s pawn=%d hp=%d tf=%d at (%.0f,%.0f,%.0f) start=%s/%d n=%d\n",
            r.slot, r.cls.c_str(), r.enemySide ? "enemy" : "friend", r.pawnId, bot->Health,
            (int)r.tf->r_nTaskForce, at.X, at.Y, at.Z, teamStart ? "team" : "first-spot",
            startObj, r.respawns);
        ++i;
    }
}

// A foe inside its spawn room (team-only shield/wall) is close but
// never visible; the straight chase then presses into the wall forever.
// No sight + foe within kBlockNear + under kBlockMove of progress for
// kBlockAfter s -> hold where we are (behavior owns movement) for kBlockHold s,
// then retry. Sight ends the hold at once. Returns true while holding.
constexpr float kBlockNear  = 700.0f;
constexpr float kBlockMove  = 40.0f;
constexpr float kBlockAfter = 4.0f;
constexpr float kBlockHold  = 6.0f;
bool TickBlocked(ATgAIController* aic, ATgPawn* bot, Chaser& c, float dt) {
    const FVector& b = bot->Location;
    if (c.lastSight) {
        if (c.blockedFor >= 0.0f)
            Logger::Log("chat-command", "[PvpBot] unblocked pawn=%d why=sight\n", c.pawnId);
        c.blockedFor = -1.0f;
        c.stuckClock = 0.0f;
        c.backoff = false;
        return false;
    }
    if (c.blockedFor >= 0.0f) {
        c.blockedFor += dt;
        if (c.backoff) {  // run back to where we last saw the foe, then hold
            const float bx = b.X - c.sightSpot.X, by = b.Y - c.sightSpot.Y;
            if (bx * bx + by * by > 80.0f * 80.0f && c.blockedFor < kBlockHold) return false;
            c.backoff = false;
            c.blockedFor = 0.0f;
            Logger::Log("chat-command", "[PvpBot] backoff done pawn=%d at=(%.0f,%.0f,%.0f)\n",
                c.pawnId, b.X, b.Y, b.Z);
            bot->Velocity.X = bot->Velocity.Y = 0.0f;
            aic->m_bInterrupt = 1;
            return true;
        }
        if (c.blockedFor < kBlockHold) return true;
        Logger::Log("chat-command", "[PvpBot] unblocked pawn=%d why=retry\n", c.pawnId);
        c.backoff = false;
        c.blockedFor = -1.0f;
        c.stuckClock = 0.0f;
        return false;
    }
    const float fx = b.X - c.foe->Location.X, fy = b.Y - c.foe->Location.Y,
                fz = b.Z - c.foe->Location.Z;
    if (c.jetting || fx * fx + fy * fy + fz * fz > kBlockNear * kBlockNear) {
        c.stuckClock = 0.0f;
        return false;
    }
    if (c.stuckClock <= 0.0f) {
        c.stuckRef = b;
        c.stuckClock = dt;
        return false;
    }
    const float mx = b.X - c.stuckRef.X, my = b.Y - c.stuckRef.Y;
    if (mx * mx + my * my > kBlockMove * kBlockMove) {
        c.stuckRef = b;
        c.stuckClock = dt;
        return false;
    }
    c.stuckClock += dt;
    if (c.stuckClock < kBlockAfter) return false;
    Logger::Log("chat-command", "[PvpBot] blocked pawn=%d dist=%.0f dz=%.0f at=(%.0f,%.0f,%.0f) hold=%.0fs\n",
        c.pawnId, std::sqrt(fx * fx + fy * fy + fz * fz), fz, b.X, b.Y, b.Z, kBlockHold);
    c.blockedFor = 0.0f;
    c.stuckClock = 0.0f;
    float sx = b.X - c.sightSpot.X, sy = b.Y - c.sightSpot.Y;
    if (!c.hasSightSpot || sx * sx + sy * sy <= 120.0f * 120.0f) {
        // Never saw the foe from here (bot arrived under
        // the spawn ledge, dz -133, no sight spot) -> pick a vantage path node
        // on the bot's own level, straight-line walkable from the bot, with a
        // clear world trace to the foe's head, 300-1500 uu from the foe.
        FVector head = c.foe->Location; head.Z += c.foe->BaseEyeHeight;
        const FVector zero = {0, 0, 0};
        ANavigationPoint* best = nullptr; float bestD = 1.0e18f; int n = 0;
        int fRing = 0, fZ = 0, fFar = 0, fSee = 0, fWalk = 0;
        for (ANavigationPoint* nav = bot->WorldInfo->NavigationPointList; nav && n < 20000;
             nav = nav->nextNavigationPoint, n++) {
            if (nav->bDeleteMe || nav->PathList.Num() == 0 || IsAirNode((AActor*)nav)) continue;
            const FVector& v = nav->Location;
            const float ex = v.X - c.foe->Location.X, ey = v.Y - c.foe->Location.Y;
            const float e2 = ex * ex + ey * ey;
            if (e2 < 300.0f * 300.0f || e2 > 1500.0f * 1500.0f) { fRing++; continue; }
            if (std::fabs(v.Z - b.Z) > 250.0f) { fZ++; continue; }
            const float gx = v.X - b.X, gy = v.Y - b.Y;
            const float g2 = gx * gx + gy * gy;
            if (g2 >= bestD || g2 > 1600.0f * 1600.0f) { fFar++; continue; }
            FVector eye = v; eye.Z += bot->BaseEyeHeight;
            if (!PbClear((AActor*)bot, head, eye) && !PbClear((AActor*)bot, c.foe->Location, eye)) { fSee++; continue; }
            if (!PbClear((AActor*)bot, v, b)) { fWalk++; continue; }
            best = nav; bestD = g2;
        }
        if (best) {
            c.sightSpot = best->Location; c.hasSightSpot = true;
            sx = b.X - c.sightSpot.X; sy = b.Y - c.sightSpot.Y;
            Logger::Log("chat-command", "[PvpBot] vantage pawn=%d nav=(%.0f,%.0f,%.0f) d=%.0f\n",
                c.pawnId, c.sightSpot.X, c.sightSpot.Y, c.sightSpot.Z, std::sqrt(bestD));
        } else {
            Logger::Log("chat-command", "[PvpBot] vantage pawn=%d none scanned=%d ring=%d z=%d far=%d see=%d walk=%d\n", c.pawnId, n, fRing, fZ, fFar, fSee, fWalk);
        }
    }
    if (c.hasSightSpot && sx * sx + sy * sy > 120.0f * 120.0f) {
        c.backoff = true;
        Logger::Log("chat-command", "[PvpBot] backoff pawn=%d to=(%.0f,%.0f,%.0f) d=%.0f\n",
            c.pawnId, c.sightSpot.X, c.sightSpot.Y, c.sightSpot.Z, std::sqrt(sx * sx + sy * sy));
        return false;  // keep the run pick; ChaseTarget now returns sightSpot
    }
    bot->Velocity.X = bot->Velocity.Y = 0.0f;
    aic->m_bInterrupt = 1;  // behavior re-picks = stand (Wait)
    return true;
}

void Tick(float DeltaSeconds) {
    TickRespawn(DeltaSeconds);
    for (auto& kv : g_chasers) {  // Jetpack thrust every frame
        ATgAIController* aic = kv.first;
        ATgPawn* bot = (aic && !aic->bDeleteMe) ? (ATgPawn*)aic->Pawn : nullptr;
        if (bot && !bot->bDeleteMe && bot->Health > 0 && bot->r_nPawnId == kv.second.pawnId)
            JetTick(bot, kv.second, DeltaSeconds);
    }
    // Re-pick often while chasing so the run target follows the foe, and once
    // more when the chase ends so the behavior takes movement back.
    static float s_chaseClock = 0.0f;
    s_chaseClock += DeltaSeconds;
    if (s_chaseClock >= 0.5f && !g_chasers.empty()) {
        s_chaseClock = 0.0f;
        for (auto it = g_chasers.begin(); it != g_chasers.end();) {
            ATgAIController* aic = it->first;
            ATgPawn* bot = (aic && !aic->bDeleteMe) ? (ATgPawn*)aic->Pawn : nullptr;
            if (!bot || bot->bDeleteMe || bot->Health <= 0 || bot->r_nPawnId != it->second.pawnId) {
                it = g_chasers.erase(it);
                continue;
            }
            if (it->second.flankNode && aic->WorldInfo &&
                aic->WorldInfo->TimeSeconds - it->second.flankLastAt > 1.5f) {  // Chase stopped mid-flank
                const float fnow = aic->WorldInfo->TimeSeconds;
                Logger::Log("chat-command", "[PvpBot] flank end pawn=%d why=left for=%.1f foeD=-1 at=(%.0f,%.0f,%.0f)\n",
                    it->second.pawnId, fnow - it->second.flankAt, bot->Location.X, bot->Location.Y, bot->Location.Z);
                it->second.flankNode = nullptr; it->second.flankFoe = nullptr; it->second.flankEndAt = fnow;
                it->second.routeGoal = nullptr;
            }
            const bool hadFoe = RefreshFoe(aic, bot, it->second);
            TickScope(aic, bot, it->second, hadFoe);
            if (it->second.coverOn && aic->WorldInfo) {  // No stale cover (foe gone / other mode took over / overlong)
                const float cnow = aic->WorldInfo->TimeSeconds;
                if (!hadFoe || !it->second.foe || it->second.foe != it->second.coverFoe || it->second.foe->Health <= 0)
                    EndCover(it->second, "foe", cnow);
                else if (cnow - it->second.coverAt > 8.0f) EndCover(it->second, "long", cnow);
            }
            TickShield(aic, bot, it->second);
            TickRegen(aic, bot, it->second);
            TickScan(aic, bot, it->second);
            if (TickRetreat(aic, bot, it->second, 0.5f)) {
                TickDecoy(aic, bot, it->second, true);  // Decoy left behind, then cloak
                TickMine(aic, bot, it->second, true);  // Mine dropped behind
                TickStealth(aic, bot, it->second, false, true);  // Escape cloak
                EndCover(it->second, "retreat", aic->WorldInfo ? aic->WorldInfo->TimeSeconds : 0.0f); ++it; continue; }
            if (TickPoison(aic, bot, it->second)) {
                EndCover(it->second, "poison", aic->WorldInfo ? aic->WorldInfo->TimeSeconds : 0.0f); ++it; continue; }
            if (TickFfWall(aic, bot, it->second)) {
                EndCover(it->second, "ffwall", aic->WorldInfo ? aic->WorldInfo->TimeSeconds : 0.0f); ++it; continue; }
            if (TickWave(aic, bot, it->second)) { ++it; continue; }
            if (TickNeut(aic, bot, it->second)) { ++it; continue; }
            if (TickWall(aic, bot, it->second)) { ++it; continue; }
            if (TickSensor(aic, bot, it->second)) { ++it; continue; }
            if (TickDecoy(aic, bot, it->second)) { ++it; continue; }
            if (TickMine(aic, bot, it->second)) { ++it; continue; }
            if (TickHeal(aic, bot, it->second)) { ++it; continue; }
            if (TickTurret(aic, bot, it->second)) { ++it; continue; }
            if (TickSlam(aic, bot, it->second)) { ++it; continue; }
            if (TickGrenade(aic, bot, it->second)) { ++it; continue; }
            if (TickCargo(aic, bot, it->second, hadFoe)) { ++it; continue; }
            if (TickObjective(aic, bot, it->second, hadFoe, 0.5f)) {
                EndCover(it->second, "objective", aic->WorldInfo ? aic->WorldInfo->TimeSeconds : 0.0f);
                // A recon keeps sneaking to the objective - same drain + ends (close / max time / power out)
                if (it->second.cloaked || it->second.tellUntil > 0.0f) TickStealth(aic, bot, it->second, true);
                if (it->second.meleeOn) {  // Gun back for the objective
                    it->second.meleeOn = false;
                    WantDevice(aic, bot, it->second, 2, "objective");
                }
                ++it; continue;
            }
            if (!hadFoe) {
                if (it->second.cloaked) EndCloak(aic, bot, it->second, "no foe");
                ++it; continue;
            }
            // m_bInterrupt alone does not make the Wait behavior re-pick, so
            // issue the run pick ourselves (it passes through OverrideMovement,
            // which stamps the foe's position) and once more on chase end.
            const bool was = it->second.chasing;
            it->second.diagClock += 0.5f;
            if (it->second.diagClock >= 2.0f) {
                it->second.diagClock = 0.0f;
                const float now = aic->WorldInfo ? aic->WorldInfo->TimeSeconds : 0.0f;
                const float vx = bot->Velocity.X, vy = bot->Velocity.Y;
                const float ddx = bot->Location.X - it->second.foe->Location.X;
                const float ddy = bot->Location.Y - it->second.foe->Location.Y;
                Logger::Log("chat-command",
                    "[PvpBot] sight pawn=%d dist2d=%.0f enemy=%d seen=%.1f clearLos=%.1f hit=%.1f "
                    "chasing=%d speed=%.0f gs=%.0f walking=%d dz=%.0f los=%d blocked=%d\n",
                    it->second.pawnId, std::sqrt(ddx * ddx + ddy * ddy),
                    (int)(aic->Enemy == it->second.foe),
                    now - aic->m_fTargetLastSeenTime, now - aic->m_fTargetLastClearLOSTime,
                    now - aic->m_fLastTargetHitTime, (int)it->second.chasing,
                    std::sqrt(vx * vx + vy * vy), bot->GroundSpeed, (int)bot->bIsWalking,
                    bot->Location.Z - it->second.foe->Location.Z, (int)it->second.lastSight,
                    (int)(it->second.blockedFor >= 0.0f));
            }
            const bool wants = WantsChase(aic, bot, it->second);
            TickStealth(aic, bot, it->second, wants);
            TickDodge(aic, bot, it->second);
            TickWeapon(aic, bot, it->second);
            if (wants && TickBlocked(aic, bot, it->second, 0.5f)) {
                ++it;  // Foe close but unreachable - hold, retry later
                continue;
            }
            if (wants) {
                // Run, don't walk, while chasing (bIsWalking scales speed
                // by WalkingPct).
                if (bot->bIsWalking) bot->bIsWalking = 0;
                TickCover(aic, bot, it->second, true);  // Ends cover
                it->second.strafeOn = false;
                aic->eventSetWhatToDoNext(238, 289);
                MaybeStartJet(bot, it->second);
                TickChaseUnstick(bot, it->second, 0.5f);
            } else if (it->second.meleeOn) {  // Close in for the swing
                if (bot->bIsWalking) bot->bIsWalking = 0;
                it->second.strafeOn = false;
                aic->eventSetWhatToDoNext(238, 289);
            } else {
                const bool wasStrafe = it->second.strafeOn;
                if (TickCover(aic, bot, it->second, false)) {
                    if (bot->bIsWalking) bot->bIsWalking = 0;
                    aic->eventSetWhatToDoNext(238, 289);
                } else if (TickStrafe(aic, bot, it->second)) {
                    if (bot->bIsWalking) bot->bIsWalking = 0;
                    aic->eventSetWhatToDoNext(238, 289);
                } else if (was || wasStrafe) {
                    aic->m_bInterrupt = 1;
                }
            }
            ++it;
        }
    }

    for (size_t i = 0; i < g_pending.size();) {
        Pending& k = g_pending[i];
        k.next -= DeltaSeconds;
        if (k.next > 0.0f) { i++; continue; }
        ATgPawn* bot = (k.ctrl && !k.ctrl->bDeleteMe) ? (ATgPawn*)k.ctrl->Pawn : nullptr;
        if (!bot || bot->bDeleteMe || bot->Health <= 0 || bot->r_nPawnId != k.pawnId ||
            !PawnAlive(k.foe, bot) || --k.checks < 0) {
            g_pending.erase(g_pending.begin() + i);
            continue;
        }
        if (k.ctrl->Enemy != k.foe) {
            k.ctrl->eventSeePlayer(k.foe);
            k.ctrl->SetTargetActor((AActor*)k.foe);
            // SeePlayer can refuse a factory-less bot (same as NPC hatchlings):
            // set Enemy directly and make the behavior re-pick its action.
            if (k.ctrl->Enemy == nullptr) {
                k.ctrl->Enemy = k.foe;
                k.ctrl->m_bInterrupt = 1;
            }
        }
        const std::string st = k.ctrl->GetStateName().GetName();
        const std::string pst = bot->GetStateName().GetName();
        Logger::Log("chat-command",
            "[PvpBot] kick pawn=%d enemy=%d loc=(%.0f,%.0f,%.0f) hp=%d foeHp=%d "
            "phys=%d state=%s pawnState=%s factory=%p\n",
            k.pawnId, k.ctrl->Enemy == k.foe ? 1 : 0, bot->Location.X, bot->Location.Y,
            bot->Location.Z, bot->Health, k.foe->Health, (int)bot->Physics,
            st.c_str(), pst.c_str(), (void*)k.ctrl->m_pFactory);
        // Why the pawn never moves - speed caps, move target,
        // route and the per-tick velocity/acceleration the AI asks for.
        Logger::Log("chat-command",
            "[PvpBot] move pawn=%d gs=%.0f as=%.0f vel=(%.0f,%.0f,%.0f) acc=(%.0f,%.0f,%.0f) "
            "mt=%s dest=(%.0f,%.0f,%.0f) route=%d fail=%d mtimer=%.1f precise=%d base=%s\n",
            k.pawnId, bot->GroundSpeed, bot->AccelRate, bot->Velocity.X, bot->Velocity.Y,
            bot->Velocity.Z, bot->Acceleration.X, bot->Acceleration.Y, bot->Acceleration.Z,
            k.ctrl->MoveTarget ? k.ctrl->MoveTarget->GetName() : "none",
            k.ctrl->DestinationPosition.Position.X, k.ctrl->DestinationPosition.Position.Y,
            k.ctrl->DestinationPosition.Position.Z, (int)k.ctrl->RouteCache.Num(),
            k.ctrl->MoveFailureCount, k.ctrl->MoveTimer, (int)k.ctrl->bPreciseDestination,
            bot->Base ? bot->Base->GetName() : "none");
        Logger::Log("chat-command",
            "[PvpBot] tgmove pawn=%d mvt=%s mvloc=(%.0f,%.0f,%.0f) destCode=%d moveCode=%d "
            "targetCode=%d paths=%d spawn=(%.0f,%.0f,%.0f)\n",
            k.pawnId, k.ctrl->m_pMovementTarget ? k.ctrl->m_pMovementTarget->GetName() : "none",
            k.ctrl->m_vMovementLocation.X, k.ctrl->m_vMovementLocation.Y, k.ctrl->m_vMovementLocation.Z,
            k.ctrl->m_nMoveDestinationCode, k.ctrl->m_nMovementCode, k.ctrl->m_nTargetCode,
            k.ctrl->m_nFindPathCallCount, k.ctrl->m_vSpawnLocation.X, k.ctrl->m_vSpawnLocation.Y,
            k.ctrl->m_vSpawnLocation.Z);
        {
            ANavigationPoint* selfNav = k.ctrl->FindClosestNav((AActor*)bot);
            ANavigationPoint* foeNav = k.ctrl->FindClosestNav((AActor*)k.foe);
            // GetName() returns one shared buffer - copy each before the next call.
            const std::string anchorName = bot->Anchor ? bot->Anchor->GetName() : "none";
            const std::string selfName = selfNav ? selfNav->GetName() : "none";
            const std::string foeName = foeNav ? foeNav->GetName() : "none";
            Logger::Log("chat-command",
                "[PvpBot] reach pawn=%d anchor=%s reachable=%d lineOk=%d selfNav=%s foeNav=%s "
                "foeLoc=(%.0f,%.0f,%.0f) unreach=(%.0f,%.0f,%.0f)\n",
                k.pawnId, anchorName.c_str(),
                (int)k.ctrl->TargetIsReachable(), (int)k.ctrl->LineCheckMovement((AActor*)k.foe),
                selfName.c_str(), foeName.c_str(),
                k.foe->Location.X, k.foe->Location.Y, k.foe->Location.Z,
                k.ctrl->m_vUnreachableLocation.X, k.ctrl->m_vUnreachableLocation.Y,
                k.ctrl->m_vUnreachableLocation.Z);
        }
        k.next = 2.0f;
        i++;
    }
}

bool Dress(ATgPawn* pawn, const std::string& pvp_class_in, ATgPawn* foe) {
    // "<class>-nodev" (dev A/B): dress but keep the body's own devices.
    std::string pvp_class = pvp_class_in;
    const bool keepDevices = pvp_class.size() > 6 &&
        pvp_class.compare(pvp_class.size() - 6, 6, "-nodev") == 0;
    if (keepDevices) pvp_class.resize(pvp_class.size() - 6);
    // Recon variant: "recon-sniper" / "recon-smg" force it, plain
    // "recon" = sniper on odd respawn slots (kept across respawns), random
    // for slot-less dev bots.
    int sniperPick = -1;
    int regenPick = -1;  // Medic variant: "medic-regen" / "medic-neut"
    int ptPick = -1;     // Assault variant: "assault-pt" / "assault-shield"
    int msPick = -1;     // Robotics variant: "robotics-medst" / "robotics-wall"
    int eyePick = -1;    // Robotics variant: "robotics-eye" / "robotics-sensor"
    int grzPick = -1;    // Robotics variant: "robotics-grizzly" / "robotics-turret"
    int scanPick = -1;   // Recon variant: "recon-scan" / "recon-bomb"
    for (const char* sfx : {"-scan", "-bomb"}) {
        const size_t n = std::strlen(sfx);
        if (pvp_class.size() > n && pvp_class.compare(pvp_class.size() - n, n, sfx) == 0) {
            scanPick = sfx[1] == 's' ? 1 : 0;
            pvp_class.resize(pvp_class.size() - n);
        }
    }
    for (const char* sfx : {"-grizzly", "-turret"}) {
        const size_t n = std::strlen(sfx);
        if (pvp_class.size() > n && pvp_class.compare(pvp_class.size() - n, n, sfx) == 0) {
            grzPick = sfx[1] == 'g' ? 1 : 0;
            pvp_class.resize(pvp_class.size() - n);
        }
    }
    for (const char* sfx : {"-eye", "-sensor"}) {
        const size_t n = std::strlen(sfx);
        if (pvp_class.size() > n && pvp_class.compare(pvp_class.size() - n, n, sfx) == 0) {
            eyePick = sfx[1] == 'e' ? 1 : 0;
            pvp_class.resize(pvp_class.size() - n);
        }
    }
    for (const char* sfx : {"-medst", "-wall", "-pwst"}) {  // -pwst = Power Station
        const size_t n = std::strlen(sfx);
        if (pvp_class.size() > n && pvp_class.compare(pvp_class.size() - n, n, sfx) == 0) {
            msPick = sfx[1] == 'm' ? 1 : sfx[1] == 'p' ? 2 : 0;
            pvp_class.resize(pvp_class.size() - n);
        }
    }
    for (const char* sfx : {"-pt", "-shield"}) {
        const size_t n = std::strlen(sfx);
        if (pvp_class.size() > n && pvp_class.compare(pvp_class.size() - n, n, sfx) == 0) {
            ptPick = sfx[1] == 'p' ? 1 : 0;
            pvp_class.resize(pvp_class.size() - n);
        }
    }
    for (const char* sfx : {"-regen", "-neut"}) {
        const size_t n = std::strlen(sfx);
        if (pvp_class.size() > n && pvp_class.compare(pvp_class.size() - n, n, sfx) == 0) {
            regenPick = sfx[1] == 'r' ? 1 : 0;
            pvp_class.resize(pvp_class.size() - n);
        }
    }
    for (const char* sfx : {"-sniper", "-smg"}) {
        const size_t n = std::strlen(sfx);
        if (pvp_class.size() > n && pvp_class.compare(pvp_class.size() - n, n, sfx) == 0) {
            sniperPick = sfx[1] == 's' && sfx[2] == 'n' ? 1 : 0;
            pvp_class.resize(pvp_class.size() - n);
        }
    }
    const ClassKit* kit = FindKit(pvp_class);
    if (!pawn || !kit) return false;

    SwapBehavior(pawn, kit->donorBotId);
    // First cosmetic flips bValidCustomAssembly -> client builds a player body.
    // per-bot look (gender/head/hair/suit set/dyes/trail/name), kept per
    // respawn slot; class HoloSuit + kit name only if the look's suit fails.
    const BotLooks::Look& look = BotLooks::ForKey(g_dressKey, pvp_class);
    bool suit = BotLooks::Apply(pawn, look);
    if (!suit) {
        suit = CosmeticEquip::ApplyItemToBot(pawn, kit->suitItemId);
        if (look.name.empty()) SetName(pawn, kit->botName);
    }
    // Threat seed = SuperAgent's quarry seed (1000); m_bInterrupt makes the
    // behavior re-pick its action now.
    ATgAIController* aic = AiOf(pawn);
    if (aic && foe) {
        aic->SetTargetActor((AActor*)foe);
        aic->AddThreat(foe, 1000.0f);
        aic->m_bInterrupt = 1;
        g_pending.push_back({aic, pawn->r_nPawnId, (APawn*)foe, 1.0f, 10});
        g_chasers[aic] = {pawn->r_nPawnId, (APawn*)foe, false, 0};
        g_chasers[aic].foeCtrl = ((APawn*)foe)->Controller;
        g_chasers[aic].foePawnId = ((ATgPawn*)foe)->r_nPawnId;
    } else if (aic) {
        // No quarry given (friend bots): RefreshFoe picks the nearest
        // opponent on the next chase round.
        g_chasers[aic] = {pawn->r_nPawnId, nullptr, false, 0};
    }

    if (aic && g_chasers.count(aic)) g_chasers[aic].skill = g_dressSkill;
    Logger::Log("chat-command",
        "[PvpBot] dressed %s pawn=%p donor=%d suit=%d(%d) melee=%d ranged=%d hp=%d foe=%p\n",
        kit->name, (void*)pawn, kit->donorBotId, kit->suitItemId, (int)suit,
        kit->meleeDevice, kit->rangedDevice, pawn->Health, (void*)(aic ? foe : nullptr));

    // Device swaps LAST: EquipInHandDevice ends in UpdateClientDevices
    // (SpawnBotById frame trap) - nothing may follow it.
    if (aic && kit->specialtyDevice) {
        const bool rob = std::string(kit->name) == "robotics";
        const bool rec = std::string(kit->name) == "recon";
        g_chasers[aic].medic = !rob && !rec;  // Heal role
        g_chasers[aic].robotics = rob;   // Turret role
        g_chasers[aic].recon = rec;      // Stealth role
    }
    if (aic && g_chasers.count(aic)) g_chasers[aic].assault = std::string(kit->name) == "assault";
    bool sniper = false;
    if (std::string(kit->name) == "recon") {
        sniper = sniperPick >= 0 ? sniperPick == 1
               : (g_dressKey > 0 ? (g_dressKey % 2) == 1 : Rand01() < 0.5f);
        if (aic && g_chasers.count(aic)) g_chasers[aic].sniper = sniper;
        Logger::Log("chat-command", "[PvpBot] recon variant pawn=%d %s key=%d\n",
            pawn->r_nPawnId, sniper ? "sniper" : "smg", g_dressKey);
    }
    bool scanner = false;
    if (std::string(kit->name) == "recon") {  // Half the recons carry Visual Scanner ((slot/2) even)
        const int sk = ScanKnob(pawn->WorldInfo ? pawn->WorldInfo->TimeSeconds : 0.0f);
        scanner = scanPick >= 0 ? scanPick == 1
                : sk == 0 ? false
                : (g_dressKey > 0 ? ((g_dressKey / 2) % 2) == 0 : Rand01() < 0.5f);
        if (aic && g_chasers.count(aic)) g_chasers[aic].scanner = scanner;
        Logger::Log("chat-command", "[PvpBot] recon variant2 pawn=%d %s key=%d\n",
            pawn->r_nPawnId, scanner ? "scanner" : "firebomb", g_dressKey);
    }
    bool regen = false;
    if (std::string(kit->name) == "medic") {  // Half the medics carry Regeneration (even respawn slots)
        const int rk = RegenKnob(pawn->WorldInfo ? pawn->WorldInfo->TimeSeconds : 0.0f);
        regen = regenPick >= 0 ? regenPick == 1
              : rk == 0 ? false
              : (g_dressKey > 0 ? (g_dressKey % 2) == 0 : Rand01() < 0.5f);
        if (aic && g_chasers.count(aic)) g_chasers[aic].regen = regen;
        Logger::Log("chat-command", "[PvpBot] medic variant pawn=%d %s key=%d\n",
            pawn->r_nPawnId, regen ? "regen" : "neut", g_dressKey);
    }
    bool medst = false, pwst = false;  // Pwst
    if (std::string(kit->name) == "robotics") {  // Half the robotics carry Medical Station (even respawn slots)
        const int mk = MedstKnob(pawn->WorldInfo ? pawn->WorldInfo->TimeSeconds : 0.0f);
        medst = msPick >= 0 ? msPick == 1
              : mk == 0 ? false
              : (g_dressKey > 0 ? (g_dressKey % 2) == 0 : Rand01() < 0.5f);
        if (aic && g_chasers.count(aic)) g_chasers[aic].medst = medst;
        if (!medst) {  // Odd (wall) slots with (slot / 4) odd carry Power Station
            const int pk = PwstKnob(pawn->WorldInfo ? pawn->WorldInfo->TimeSeconds : 0.0f);
            pwst = msPick >= 0 ? msPick == 2
                 : pk == 0 ? false
                 : (g_dressKey > 0 ? ((g_dressKey / 4) % 2) == 1 : Rand01() < 0.5f);
        }
        Logger::Log("chat-command", "[PvpBot] robotics variant pawn=%d %s key=%d\n",
            pawn->r_nPawnId, medst ? "medst" : pwst ? "pwst" : "wall", g_dressKey);
    }
    bool eyedrone = false;
    if (std::string(kit->name) == "robotics") {  // Half the robotics carry Eye Drone (odd respawn slots)
        const int ek = EyeKnob(pawn->WorldInfo ? pawn->WorldInfo->TimeSeconds : 0.0f);
        eyedrone = eyePick >= 0 ? eyePick == 1
                 : ek == 0 ? false
                 : (g_dressKey > 0 ? (g_dressKey % 2) == 1 : Rand01() < 0.5f);
        if (aic && g_chasers.count(aic)) g_chasers[aic].eyedrone = eyedrone;
        Logger::Log("chat-command", "[PvpBot] robotics variant2 pawn=%d %s key=%d\n",
            pawn->r_nPawnId, eyedrone ? "eyedrone" : "sensor", g_dressKey);
    }
    bool grizzly = false;
    if (std::string(kit->name) == "robotics") {  // Half the robotics carry Grizzly Drone ((slot / 2) odd)
        const int gk = GrizzlyKnob(pawn->WorldInfo ? pawn->WorldInfo->TimeSeconds : 0.0f);
        grizzly = grzPick >= 0 ? grzPick == 1
                : gk == 0 ? false
                : (g_dressKey > 0 ? ((g_dressKey / 2) % 2) == 1 : Rand01() < 0.5f);
        if (aic && g_chasers.count(aic)) g_chasers[aic].grizzly = grizzly;
        Logger::Log("chat-command", "[PvpBot] robotics variant3 pawn=%d %s key=%d\n",
            pawn->r_nPawnId, grizzly ? "grizzly" : "turret", g_dressKey);
    }
    bool ptarget = false;
    if (std::string(kit->name) == "assault") {  // Half the assaults carry Perfect Target (odd respawn slots)
        const int pk = PtargetKnob(pawn->WorldInfo ? pawn->WorldInfo->TimeSeconds : 0.0f);
        ptarget = ptPick >= 0 ? ptPick == 1
              : pk == 0 ? false
              : (g_dressKey > 0 ? (g_dressKey % 2) == 1 : Rand01() < 0.5f);
        if (aic && g_chasers.count(aic)) g_chasers[aic].ptarget = ptarget;
        Logger::Log("chat-command", "[PvpBot] assault variant pawn=%d %s key=%d\n",
            pawn->r_nPawnId, ptarget ? "ptarget" : "shield", g_dressKey);
    }
    if (keepDevices) return true;
    ATgRepInfo_Player* pri = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;
    TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, kit->meleeDevice, 1, 1162, 389);
    if (kit->specialtyDevice)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, grizzly ? kGrizzly : kit->specialtyDevice, 3, 1162, 981);
    if (std::string(kit->name) == "assault")  // EMP grenade (offhand)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, kAssaultGrenade, kGrenadePoint, 1162, 390);
    if (std::string(kit->name) == "assault")  // Concussion Grenade (offhand)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, kConcGrenade, kConcPoint, 1162, 390);
    if (std::string(kit->name) == "assault")  // Range Shield (offhand)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, ptarget ? kPerfectTarget : kRangeShield, kShieldPoint, 1162, 390);
    if (std::string(kit->name) == "medic")  // Healing Wave (offhand)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, kHealWave, kWavePoint, 1162, 390);
    if (std::string(kit->name) == "medic")  // Neutralize Wave / Regeneration (offhand)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, regen ? kRegen : kNeutWave, kNeutPoint, 1162, 390);
    if (std::string(kit->name) == "medic")  // Poison Grenade (offhand)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, kPoisonNade, kPoisonPoint, 1162, 390);
    if (std::string(kit->name) == "robotics")  // Force Wall (offhand)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, medst ? kMedStation : pwst ? kPowerStation : kForceWall, kWallPoint, 1162, 390);
    if (std::string(kit->name) == "robotics")  // Sensor (offhand)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, eyedrone ? kEyeDrone : kSensor, kSensorPoint, 1162, 390);
    if (std::string(kit->name) == "recon")  // Fire Bomb (offhand)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, scanner ? kScanner : kFireBomb, kFireBombPoint, 1162, 390);
    if (std::string(kit->name) == "recon")  // Standard Mine (offhand)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, kMine, kMinePoint, 1162, 390);
    if (std::string(kit->name) == "recon")  // Decoy (offhand)
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, kDecoy, kDecoyPoint, 1162, 390);
    if (std::string(kit->name) == "robotics")  // Repair arm
        TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, kRepairArm, kRepairPoint, 1162, 981);
    TgGame__SpawnBotById::EquipInHandDevice(pawn, pri, sniper ? kSniperRifle : kit->rangedDevice, 2, 1162, 388);
    return true;
}

void RegisterRespawn(ATgPawn* bot, int botId, const std::string& pvpClass,
                     ATgRepInfo_TaskForce* tf, bool enemySide,
                     const FVector& loc, const FRotator& rot, ATgPawn* foe,
                     DifficultyScalar scalar) {
    if (!bot || !tf || !foe || !((APawn*)foe)->Controller) return;
    RespawnSlot r;
    r.slot = g_nextSlot++;
    r.botId = botId;
    r.cls = pvpClass;
    r.tf = tf;
    r.enemySide = enemySide;
    r.loc = loc;
    r.rot = rot;
    r.pawnId = bot->r_nPawnId;
    r.foeCtrl = ((APawn*)foe)->Controller;
    r.world = bot->WorldInfo;
    r.scalar = scalar;
    BotLooks::Rekey(BotLooks::LastTempKey(), r.slot);  // Keep the first look
    g_respawn.push_back(r);
    Logger::Log("chat-command", "[PvpBot] respawn slot=%d class=%s bot_id=%d pawn=%d tf=%d\n",
        r.slot, pvpClass.c_str(), botId, r.pawnId, (int)tf->r_nTaskForce);
}

// task-force bots (owner = a tag, not a requesting player).
namespace {
constexpr int kMaxTeamBots = 16;  // per instance, all tags
ATgRepInfo_TaskForce* TaskForceByNumber(int n) {
    for (ATgRepInfo_TaskForce* tf : {GTeamsData.Attackers, GTeamsData.Defenders})
        if (tf && (int)tf->r_nTaskForce == n) return tf;
    return nullptr;
}
int ClassBotId(const std::string& cls) {  // same ids as -spawnpvpbot
    if (cls == "assault")  return 680;
    if (cls == "medic")    return 567;
    if (cls == "recon")    return 681;
    if (cls == "robotics") return 679;
    return 0;
}
} // namespace

int AddTeamBot(int taskForce, const std::string& cls, const std::string& tag, std::string* why, float skill) {
    auto fail = [&](const char* w) {
        if (why) *why = w;
        Logger::Log("chat-command", "[PvpBot] team add tf=%d class=%s tag=%s refused: %s\n",
            taskForce, cls.c_str(), tag.c_str(), w);
        return 0;
    };
    if (tag.empty() || tag.size() > 64) return fail("need a tag (<= 64 chars)");
    std::string base = cls;  // recon-sniper / recon-smg, medic-regen / medic-neut
    for (const char* sfx : {"-scan", "-bomb", "-grizzly", "-turret", "-eye", "-sensor", "-sniper", "-smg", "-regen", "-neut", "-pt", "-shield", "-medst", "-wall", "-pwst"}) {  // robotics-eye / robotics-sensor
    //   // assault-pt / assault-shield, robotics-medst / robotics-wall
        const size_t n = std::strlen(sfx);
        if (base.size() > n && base.compare(base.size() - n, n, sfx) == 0) base.resize(base.size() - n);
    }
    const int botId = ClassBotId(base);
    if (!botId || !FindKit(base)) return fail("unknown class");
    ATgRepInfo_TaskForce* tf = TaskForceByNumber(taskForce);
    if (!tf) return fail("no such task force");
    AWorldInfo* wi = CurWorld();
    if (!wi) return fail("no world");
    int owned = 0;
    for (const RespawnSlot& r : g_respawn) if (!r.tag.empty()) ++owned;
    if (owned >= kMaxTeamBots) return fail("bot cap");
    const int slot = g_nextSlot++;
    FVector at{};
    FRotator facing{};
    int startObj = 0;
    if (!PickTeamStart(taskForce, slot, at, facing, startObj)) return fail("no team start");
    // r_bInitialIsEnemy is one replicated value for every viewer; the PRI's
    // task force decides friend/enemy once it arrives.
    const bool enemySide = (tf == GTeamsData.Defenders);
    if (skill < 0.0f || skill > 1.0f) skill = 0.5f;
    g_dressKey = slot;
    g_dressSkill = skill;
    ATgPawn* bot = TgPlayerActions::SpawnBotCmd::SpawnDressedAt(
        botId, at, facing, tf, enemySide, cls, nullptr, DifficultyScalar());
    g_dressKey = 0;
    g_dressSkill = kDefaultSkill;
    if (!bot) { BotLooks::Release(slot); return fail("spawn refused"); }
    RespawnSlot r;
    r.slot = slot;
    r.botId = botId;
    r.cls = cls;
    r.tf = tf;
    r.enemySide = enemySide;
    r.loc = at;
    r.rot = facing;
    r.pawnId = bot->r_nPawnId;
    r.foeCtrl = nullptr;
    r.world = wi;
    r.tag = tag;
    r.skill = skill;
    g_respawn.push_back(r);
    Logger::Log("chat-command", "[PvpBot] team add slot=%d tf=%d class=%s tag=%s pawn=%d start=%d skill=%.2f at=(%.0f,%.0f,%.0f)\n",
        slot, taskForce, cls.c_str(), tag.c_str(), r.pawnId, startObj, skill, at.X, at.Y, at.Z);
    return slot;
}

int RemoveTeamBots(const std::string& tag, int slot, bool now) {
    AWorldInfo* wi = CurWorld();
    int n = 0;
    for (RespawnSlot& r : g_respawn) {
        if (r.tag.empty() || r.tag != tag || (slot > 0 && r.slot != slot)) continue;
        r.leave = true;
        ++n;
        APawn* p = now ? LivePawnById(wi, r.pawnId) : nullptr;
        if (p) {
            p->Health = 0;
            p->Died(nullptr, nullptr, p->Location);
        }
        Logger::Log("chat-command", "[PvpBot] team remove slot=%d tag=%s now=%d killed=%d\n",
            r.slot, tag.c_str(), (int)now, (int)(p != nullptr));
    }
    return n;
}

// Dev: put live non-medic PvP bots (task force n, 0 = all) at pct of max HP.
int DevSetHealthPct(int pct, int taskForce) {
    if (pct < 1) pct = 1;
    if (pct > 100) pct = 100;
    int n = 0;
    for (auto& kv : g_chasers) {
        ATgAIController* aic = kv.first;
        APawn* p = (aic && !aic->bDeleteMe) ? aic->Pawn : nullptr;
        if (!p || p->bDeleteMe || p->Health <= 0 || ((ATgPawn*)p)->r_nPawnId != kv.second.pawnId) continue;
        ATgRepInfo_TaskForce* tf = TfOf(p);
        if (taskForce && (!tf || (int)tf->r_nTaskForce != taskForce)) continue;
        if (kv.second.medic) continue;  // medics stay full (they are the heal source in the test)
        p->Health = std::max(1, MaxHp(p) * pct / 100);
        n++;
        if (TurretAlive(kv.second, p)) {  // Its turret too (repair test)
            kv.second.turret->Health = std::max(1, MaxHp(kv.second.turret) * pct / 100);
            Logger::Log("chat-command", "[PvpBot] dev hp turret=%d hp=%d\n", kv.second.turretId, kv.second.turret->Health);
        }
    }
    Logger::Log("chat-command", "[PvpBot] dev hp pct=%d tf=%d -> %d bots\n", pct, taskForce, n);
    return n;
}

void ReportRoster(const std::string& result) {
    AWorldInfo* wi = CurWorld();
    int players[3] = {0, 0, 0}, bots[3] = {0, 0, 0};
    for (AController* c = wi ? wi->ControllerList : nullptr; c; c = c->NextController) {
        if (c->bDeleteMe || !ObjectClassCache::ClassNameContains((UObject*)c, "PlayerController")) continue;
        ATgRepInfo_Player* pri = (ATgRepInfo_Player*)c->PlayerReplicationInfo;
        ATgRepInfo_TaskForce* tf = pri ? (ATgRepInfo_TaskForce*)pri->r_TaskForce : nullptr;
        const int n = tf ? (int)tf->r_nTaskForce : 0;
        players[(n == 1 || n == 2) ? n : 0]++;
    }
    nlohmann::json list = nlohmann::json::array();
    for (const RespawnSlot& r : g_respawn) {
        if (r.tag.empty()) continue;
        const int n = r.tf ? (int)r.tf->r_nTaskForce : 0;
        if (!r.leave) bots[(n == 1 || n == 2) ? n : 0]++;
        APawn* body = LivePawnById(wi, r.pawnId);
        float fight = -1.0f;  // Nearest live opponent (uu)
        if (body && wi) {
            for (APawn* q = wi->PawnList; q; q = q->NextPawn) {
                if (q == body || q->bDeleteMe || q->Health <= 0) continue;
                ATgRepInfo_TaskForce* qtf = TfOf(q);
                if (!qtf || qtf == r.tf) continue;
                const float d = std::sqrt(Dist2(body, q));
                if (fight < 0.0f || d < fight) fight = d;
            }
        }
        list.push_back({{"slot", r.slot}, {"tag", r.tag}, {"class", r.cls}, {"task_force", n},
                        {"alive", body != nullptr}, {"leaving", r.leave}, {"fight_dist", (int)fight}});
    }
    const std::string map = Config::GetMapNameChar();
    auto row = MapGameInfo::LookupByName(map);
    nlohmann::json msg;
    msg["type"]   = IpcProtocol::MSG_TEAM_ROSTER;
    msg["map"]    = map;
    msg["pvp"]    = row ? row->is_pvp : false;
    msg["result"] = result;
    msg["teams"]  = nlohmann::json::array({
        {{"task_force", 1}, {"players", players[1]}, {"bots", bots[1]}},
        {{"task_force", 2}, {"players", players[2]}, {"bots", bots[2]}}});
    msg["unassigned_players"] = players[0];
    msg["bots"] = list;
    msg["timer_state"] = SetupRebalanceTrigger::TimerState();
    msg["setup_left"] = SetupRebalanceTrigger::SetupSecondsLeft();
    IpcClient::Send(msg.dump());
    Logger::Log("chat-command", "[PvpBot] roster %s tf1=%d+%d tf2=%d+%d result=%s\n",
        map.c_str(), players[1], bots[1], players[2], bots[2], result.c_str());
}

} // namespace PvpBots
