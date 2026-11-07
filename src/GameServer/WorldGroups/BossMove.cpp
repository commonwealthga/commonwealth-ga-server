#include "src/GameServer/WorldGroups/BossMove.hpp"
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

// See BossMove.hpp.
namespace BossMove {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kSample = 250.0f;   // floor sample spacing along a step
constexpr float kLook = 750.0f;     // how far ahead a straight step is checked
constexpr float kDetourLen = 600.0f;
constexpr float kProgress = 80.0f;  // 2D movement that counts as progress
constexpr float kChaseNoGain = 14.0f;  // chase seconds without getting 150 uu closer -> Blocked

struct TagCfg {
	float limitR = 0.0f, limitX = 0.0f, limitY = 0.0f;
	float maxDrop = 450.0f, maxRise = 180.0f, stuckS = 4.0f;
	bool evade = false, evading = false, bots = false;
	float evadeSince = 0.0f;
	int evades = 0;
	std::map<APawn*, float> unreach;  // target -> until (pointer only compared)
	int unreachMarks = 0;
	double costSum = 0.0, costMax = 0.0;
	int costN = 0;
	int logs = 0;
};
std::map<std::string, TagCfg> s_cfg;

struct PawnState {
	FVector ref{};
	float refAt = -1.0f;
	int stucks = 0, side = 1;
	FVector detour{};
	float detourUntil = 0.0f, blockedSince = -1.0f;
	int detours = 0, blocks = 0, teleports = 0, cliffs = 0, walls = 0, edges = 0, wallLogs = 0;
	const char* last = "-";
	// Chase-progress watch: best 3D distance to the chase goal and when it last improved.
	float goalBest = 0.0f, goalBestAt = -1.0f, lastChaseAt = -100.0f;
	int paced = 0;
};
std::map<ATgPawn*, PawnState> s_pawn;

float D2(const FVector& a, const FVector& b) {
	const float dx = a.X - b.X, dy = a.Y - b.Y;
	return dx * dx + dy * dy;
}

// World-only trace straight down (static geometry / terrain, no actors).
bool FloorAt(float x, float y, float zTop, float depth, float* outZ) {
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game) return false;
	FVector start; start.X = x; start.Y = y; start.Z = zTop;
	FVector end = start; end.Z = zTop - depth;
	FVector hitLoc, hitNorm;
	FTraceHitInfo hitInfo;
	std::memset(&hitInfo, 0, sizeof(hitInfo));
	if (!Game->Trace(end, start, 0, FVector(0, 0, 0), 0, &hitLoc, &hitNorm, &hitInfo)) return false;
	*outZ = hitLoc.Z;
	return true;
}

// World-only trace from a to b; true if something is hit (hit Z in *hitZ).
bool WorldHit(const FVector& a, const FVector& b, float* hitZ) {
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (!Game) return false;
	FVector hitLoc, hitNorm;
	FTraceHitInfo hitInfo;
	std::memset(&hitInfo, 0, sizeof(hitInfo));
	FVector end = b, start = a;
	if (!Game->Trace(end, start, 0, FVector(0, 0, 0), 0, &hitLoc, &hitNorm, &hitInfo)) return false;
	*hitZ = hitLoc.Z;
	return true;
}

// Is the straight line from the pawn along (dx,dy) (unit) walkable for len uu?
// Samples the floor every kSample: no floor / below kill-Z = edge, a drop over
// maxDrop = cliff, a rise over maxRise = wall or steep rock; plus a body-height
// trace between samples for buildings/overhangs.
bool Walkable(ATgPawn* pawn, const TagCfg& c, float dx, float dy, float len, PawnState& st) {
	const float ch = pawn->CylinderComponent ? pawn->CylinderComponent->CollisionHeight : 60.0f;
	float prev = pawn->Location.Z - ch;
	const float killZ = pawn->WorldInfo ? pawn->WorldInfo->KillZ : -1.0e6f;
	FVector a = pawn->Location;
	for (float s = kSample; ; s += kSample) {
		const float t = std::fmin(s, len);
		const float x = pawn->Location.X + dx * t, y = pawn->Location.Y + dy * t;
		float fz;
		if (!FloorAt(x, y, prev + c.maxRise + 2.0f * ch, 4000.0f, &fz) || fz < killZ + 300.0f) { st.edges++; st.last = "edge"; return false; }
		if (prev - fz > c.maxDrop) { st.cliffs++; st.last = "cliff"; return false; }
		if (fz - prev > c.maxRise) { st.walls++; st.last = "steep"; return false; }
		// Body check between samples at 1.3 x half-height above each floor (knee-high bumps and
		// the slope itself don't count; buildings, rock walls and overhangs do). World-only trace.
		FVector b; b.X = x; b.Y = y; b.Z = fz + 1.3f * ch;
		FVector a2 = a; a2.Z = std::fmax(a.Z, prev + 1.3f * ch);
		float hz;
		if (WorldHit(a2, b, &hz)) {
			st.walls++; st.last = "wall";
			if (st.wallLogs < 6) {
				++st.wallLogs;
				Logger::Log("worldgroups", "[BossMove] wall (%.0f,%.0f,%.0f)->(%.0f,%.0f,%.0f) hit z=%.0f floor prev=%.0f next=%.0f\n",
					a2.X, a2.Y, a2.Z, b.X, b.Y, b.Z, hz, prev, fz);
			}
			return false;
		}
		a = b;
		prev = fz;
		if (t >= len) break;
	}
	return true;
}

} // namespace

void Configure(const std::string& tag, const nlohmann::json& j) {
	if (!(j.contains("limit_r") || j.contains("max_drop") || j.contains("max_rise") || j.contains("stuck_s") || j.contains("evade") || j.contains("engage_bots")))
		return;
	TagCfg& c = s_cfg[tag];
	auto f = [&](const char* k, float lo, float hi, float def) {
		return j.contains(k) && j[k].is_number() ? std::fmax(lo, std::fmin(hi, j[k].get<float>())) : def;
	};
	c.limitR = f("limit_r", 0.0f, 50000.0f, c.limitR);
	c.limitX = f("limit_x", -1.0e6f, 1.0e6f, c.limitX);
	c.limitY = f("limit_y", -1.0e6f, 1.0e6f, c.limitY);
	c.maxDrop = f("max_drop", 100.0f, 3000.0f, c.maxDrop);
	c.maxRise = f("max_rise", 40.0f, 1000.0f, c.maxRise);
	c.stuckS = f("stuck_s", 1.0f, 30.0f, c.stuckS);
	if (j.contains("evade") && j["evade"].is_boolean()) c.evade = j["evade"].get<bool>();
	if (j.contains("engage_bots") && j["engage_bots"].is_boolean()) c.bots = j["engage_bots"].get<bool>();
	Logger::Log("worldgroups", "[BossMove] cfg tag=%s limit=%.0f@(%.0f,%.0f) drop=%.0f rise=%.0f stuck=%.1f evade=%d\n",
		tag.c_str(), c.limitR, c.limitX, c.limitY, c.maxDrop, c.maxRise, c.stuckS, (int)c.evade);
}

void Forget(const std::string& tag) { s_cfg.erase(tag); }
bool TargetsBots(const std::string& tag) {
	auto it = s_cfg.find(tag);
	return it != s_cfg.end() && it->second.bots;
}
void ForgetPawn(ATgPawn* pawn) { s_pawn.erase(pawn); }
bool EvadeEnabled(const std::string& tag) {
	auto it = s_cfg.find(tag);
	return it != s_cfg.end() && it->second.evade;
}

Step Steer(const std::string& tag, ATgPawn* pawn, const FVector& goal, bool chase, float now, FVector* out) {
	static TagCfg defaults;
	auto ci = s_cfg.find(tag);
	TagCfg& c = ci != s_cfg.end() ? ci->second : defaults;
	PawnState& st = s_pawn[pawn];
	const FVector pos = pawn->Location;
	*out = pos;

	// Progress watch: no kProgress uu of 2D movement for stuckS -> try the other side;
	// three in a row = give up (chase) or put it home (home walk).
	if (st.refAt < 0.0f || D2(pos, st.ref) > kProgress * kProgress) {
		if (st.refAt >= 0.0f) st.stucks = 0;
		st.ref = pos; st.refAt = now;
	} else if (now - st.refAt > c.stuckS) {
		st.refAt = now;
		st.stucks++;
		st.side = -st.side;
		st.detourUntil = 0.0f;
		if (c.logs < 300) {
			++c.logs;
			Logger::Log("worldgroups", "[BossMove] no progress tag=%s at (%.0f,%.0f,%.0f) stucks=%d chase=%d last=%s\n",
				tag.c_str(), pos.X, pos.Y, pos.Z, st.stucks, (int)chase, st.last);
		}
		if (st.stucks >= 3) {
			st.stucks = 0;
			if (chase) { st.blocks++; return Step::Blocked; }
			st.teleports++;
			return Step::Teleport;
		}
	}

	// A chase that never gets closer to its target (target on a rock/cliff top it
	// keeps detouring under, or kiting it in circles) gives up after kChaseNoGain s.
	if (chase) {
		const float dz = goal.Z - pos.Z;
		const float d3 = std::sqrt(D2(pos, goal) + dz * dz);
		// (a target outside the limit circle is handled by AtLimit: it fights from the edge)
		const bool outside = c.limitR > 0.0f && (goal.X - c.limitX) * (goal.X - c.limitX) + (goal.Y - c.limitY) * (goal.Y - c.limitY) > c.limitR * c.limitR;
		if (outside || st.goalBestAt < 0.0f || now - st.lastChaseAt > 2.0f || d3 < st.goalBest - 150.0f) {
			st.goalBest = d3; st.goalBestAt = now;
		} else if (now - st.goalBestAt > kChaseNoGain) {
			st.goalBestAt = -1.0f;
			st.paced++; st.blocks++;
			if (c.logs < 300) {
				++c.logs;
				Logger::Log("worldgroups", "[BossMove] chase no gain tag=%s %.0f s at (%.0f,%.0f,%.0f) best=%.0f dz=%.0f\n",
					tag.c_str(), kChaseNoGain, pos.X, pos.Y, pos.Z, st.goalBest, dz);
			}
			st.lastChaseAt = now;
			return Step::Blocked;
		}
		st.lastChaseAt = now;
	}

	FVector g = goal;
	if (chase && c.limitR > 0.0f) {
		FVector l; l.X = c.limitX; l.Y = c.limitY; l.Z = 0.0f;
		const float d = std::sqrt(D2(g, l));
		if (d > c.limitR) {
			g.X = l.X + (g.X - l.X) * c.limitR / d;
			g.Y = l.Y + (g.Y - l.Y) * c.limitR / d;
			if (D2(pos, g) < 250.0f * 250.0f) { st.ref = pos; st.refAt = now; return Step::AtLimit; }
		}
	}
	if (now < st.detourUntil && D2(pos, st.detour) > 150.0f * 150.0f) { *out = st.detour; return Step::Detour; }

	float dx = g.X - pos.X, dy = g.Y - pos.Y;
	const float len = std::sqrt(dx * dx + dy * dy);
	if (len < 60.0f) { *out = g; return Step::Go; }
	dx /= len; dy /= len;
	if (Walkable(pawn, c, dx, dy, std::fmin(len, kLook), st)) {
		st.blockedSince = -1.0f;
		*out = g;
		return Step::Go;
	}
	static const float kAngles[] = {35.0f, 70.0f, 105.0f, 140.0f};
	for (float deg : kAngles) {
		for (int sgn = 0; sgn < 2; ++sgn) {
			const float a = (sgn == 0 ? st.side : -st.side) * deg * kPi / 180.0f;
			const float rx = dx * std::cos(a) - dy * std::sin(a), ry = dx * std::sin(a) + dy * std::cos(a);
			if (!Walkable(pawn, c, rx, ry, kDetourLen, st)) continue;
			st.detour = pos; st.detour.X += rx * kDetourLen; st.detour.Y += ry * kDetourLen;
			st.detourUntil = now + 3.0f;
			st.detours++;
			st.blockedSince = -1.0f;
			*out = st.detour;
			return Step::Detour;
		}
	}
	if (st.blockedSince < 0.0f) st.blockedSince = now;
	if (now - st.blockedSince > 2.0f) {
		st.blockedSince = -1.0f;
		if (chase) { st.blocks++; return Step::Blocked; }
		st.teleports++;
		return Step::Teleport;
	}
	return Step::Hold;
}

void MarkUnreachable(const std::string& tag, APawn* foe, float now, float forS) {
	if (!foe) return;
	TagCfg& c = s_cfg[tag];
	c.unreach[foe] = now + forS;
	c.unreachMarks++;
	for (auto it = c.unreach.begin(); it != c.unreach.end();) it = it->second < now ? c.unreach.erase(it) : std::next(it);
}

bool IsUnreachable(const std::string& tag, APawn* foe, float now) {
	auto ci = s_cfg.find(tag);
	if (ci == s_cfg.end() || !foe) return false;
	auto it = ci->second.unreach.find(foe);
	return it != ci->second.unreach.end() && it->second > now;
}

void Face(ATgPawn* pawn, APawn* foe) {
	if (!pawn || !foe || !pawn->Controller) return;
	pawn->Controller->Focus = (AActor*)foe;
	const float dx = foe->Location.X - pawn->Location.X, dy = foe->Location.Y - pawn->Location.Y;
	FRotator r = pawn->Rotation;
	r.Yaw = (int)(std::atan2(dy, dx) * 32768.0f / kPi);
	pawn->DesiredRotation = r;
}

void SetEvading(const std::string& tag, bool on, float now) {
	TagCfg& c = s_cfg[tag];
	if (on == c.evading) return;
	c.evading = on;
	if (on) { c.evadeSince = now; c.evades++; }
	Logger::Log("worldgroups", "[BossMove] evade tag=%s %s (#%d)\n", tag.c_str(), on ? "ON (hit only by unreachable targets)" : "off", c.evades);
}

bool Evading(const std::string& tag) {
	auto it = s_cfg.find(tag);
	return it != s_cfg.end() && it->second.evading;
}

void Status(const std::string& tag, ATgPawn* pawn, nlohmann::json& mj) {
	(void)tag;
	auto it = s_pawn.find(pawn);
	if (it == s_pawn.end()) return;
	const PawnState& st = it->second;
	mj["mv"] = {{"detours", st.detours}, {"blocks", st.blocks}, {"tp", st.teleports}, {"cliff", st.cliffs},
	            {"wall", st.walls}, {"edge", st.edges}, {"paced", st.paced}, {"last", st.last}};
}

void TagStatus(const std::string& tag, nlohmann::json& msg) {
	auto it = s_cfg.find(tag);
	if (it == s_cfg.end()) return;
	TagCfg& c = it->second;
	msg["evading"] = c.evading;
	msg["evades"] = c.evades;
	msg["unreach_marks"] = c.unreachMarks;
	if (c.costN > 0) {
		msg["ai_us_avg"] = c.costSum / c.costN;
		msg["ai_us_max"] = c.costMax;
		msg["ai_ticks"] = c.costN;  // cumulative since spawn
	}
}

void AddCost(const std::string& tag, double us) {
	TagCfg& c = s_cfg[tag];
	c.costSum += us;
	c.costN++;
	if (us > c.costMax) c.costMax = us;
}

} // namespace BossMove
