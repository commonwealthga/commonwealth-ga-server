#include "src/TcpServer/TcpEvents/TcpEvents.hpp"

std::map<std::string, TcpEvent> GTcpEvents;
std::map<std::string, std::vector<BeaconPickupEvent>> GBeaconPickupEvents;
std::map<std::string, std::vector<BeaconRemoveEvent>> GBeaconRemoveEvents;
std::map<ATgPawn*, std::string> GPawnSessions;
std::map<std::string, std::vector<QuestEvent>> GQuestEvents;

