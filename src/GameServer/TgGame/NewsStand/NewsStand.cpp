#include "src/GameServer/TgGame/NewsStand/NewsStand.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/CommandLineParser/CommandLineParser.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/nlohmann/json.hpp"
#include <unordered_map>
#include <vector>

namespace {
	// horizontal radius + vertical band: the stand's origin (z 683) sits inside the dome's centre
	// structure between walkable floors (z ~516 / ~1050), so a tight 3D sphere was never entered
	const float kRepeatSec = 120.0f;

	struct Seen { bool inside = false; float next = 0.0f; };
	// a watched spot: Global News stand or the AvA Hall of Legends POI (msg 64676)
	struct Spot { FVector at; const char* subtype; float radius; float band; };
	const float kNewsRadius = 1200.0f, kNewsBand = 800.0f;
	const float kHallRadius = 700.0f, kHallBand = 500.0f; // spawn point 4992,-27 is ~900 uu from the hall
	const int kHallOfLegendsMsgId = 64676;
	const FVector kHallOfLegendsAt = { 4080.0f, -16.0f, 402.0f }; // DomeCity_P_VER_3_VENDORS TgPointOfInterest_22

	int s_enabled = -1; // -1 unknown, 0 off, 1 on
	AWorldInfo* s_world = nullptr;
	std::vector<Spot> s_stands;
	std::unordered_map<AController*, std::vector<Seen>> s_seen;
	float s_lastPoll = -1.0f;

	bool InRange(const FVector& a, const Spot& b) {
		float x = a.X - b.at.X, y = a.Y - b.at.Y, z = a.Z - b.at.Z;
		return x * x + y * y < b.radius * b.radius && z < b.band && z > -b.band;
	}

	bool Enabled() {
		if (s_enabled < 0) {
			ParsedOptions options = CommandLineParser::ParseCommandLine();
			std::wstring off = options.switches[L"nonewsstand"];
			bool disabled = off == L"1" || off == L"true";
			std::string map = Config::GetMapNameChar();
			bool dome = map.rfind("DomeCity", 0) == 0;
			s_enabled = (!disabled && dome) ? 1 : 0;
			Logger::Log("news", "[NewsStand] map=%s enabled=%d\n", map.c_str(), s_enabled);
		}
		return s_enabled == 1;
	}

	void Notify(AController* C, const char* subtype) {
		for (auto& kv : GClientConnectionsData) {
			if ((void*)kv.second.Pawn != (void*)C->Pawn || kv.second.SessionGuid.empty()) continue;
			nlohmann::json ev;
			ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
			ev["subtype"]      = subtype;
			ev["instance_id"]  = IpcClient::GetInstanceId();
			ev["session_guid"] = kv.second.SessionGuid;
			IpcClient::Send(ev.dump());
			Logger::Log("news", "[NewsStand] player guid=%s at %s\n", kv.second.SessionGuid.c_str(), subtype);
			return;
		}
	}
}

void NewsStand::Tick(float DeltaSeconds) {
	if (Globals::Get().GWorld == nullptr || !Enabled()) return;
	AWorldInfo* WI = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WI == nullptr) return;

	float now = WI->TimeSeconds;
	if (s_lastPoll >= 0.0f && now - s_lastPoll < 1.0f && WI == s_world) return;
	s_lastPoll = now;

	if (WI != s_world) {
		s_world = WI;
		s_stands.clear();
		s_seen.clear();
		for (UObject* obj : ObjectCache::FindAllByClass("Class TgGame.TgNewsStand")) {
			if (obj == nullptr || (obj->ObjectFlags.A & 0x200)) continue; // skip the class default object
			s_stands.push_back({ ((AActor*)obj)->Location, "news_stand", kNewsRadius, kNewsBand });
			Logger::Log("news", "[NewsStand] stand at %.0f %.0f %.0f\n", ((AActor*)obj)->Location.X,
				((AActor*)obj)->Location.Y, ((AActor*)obj)->Location.Z);
		}
		int stands = (int)s_stands.size();
		for (UObject* obj : ObjectCache::FindAllByClass("Class TgGame.TgPointOfInterest")) {
			if (obj == nullptr || (obj->ObjectFlags.A & 0x200)) continue;
			if (((ATgPointOfInterest*)obj)->m_nNameMsgId == kHallOfLegendsMsgId)
				s_stands.push_back({ ((AActor*)obj)->Location, "hall_of_legends", kHallRadius, kHallBand });
		}
		// the POI sits in a streamed sublevel; fall back to its cooked location
		if ((int)s_stands.size() == stands) s_stands.push_back({ kHallOfLegendsAt, "hall_of_legends", kHallRadius, kHallBand });
		Logger::Log("news", "[NewsStand] %d stand(s), %d hall(s)\n", stands, (int)s_stands.size() - stands);
	}
	if (s_stands.empty()) return;

	for (AController* C = WI->ControllerList; C; C = C->NextController) {
		if (!ObjectClassCache::ClassNameContains((UObject*)C, "TgPlayerController")) continue;
		if (C->Pawn == nullptr) continue;
		std::vector<Seen>& seen = s_seen[C];
		seen.resize(s_stands.size());
		for (size_t i = 0; i < s_stands.size(); ++i) {
			bool inRange = InRange(C->Pawn->Location, s_stands[i]);
			Seen& s = seen[i];
			if (inRange && !s.inside && now >= s.next) {
				s.next = now + kRepeatSec;
				Notify(C, s_stands[i].subtype);
			}
			s.inside = inRange;
		}
	}
}
