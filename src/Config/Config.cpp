#include "src/Config/Config.hpp"
#include "src/Utils/CommandLineParser/CommandLineParser.hpp"
#include "src/Utils/Logger/Logger.hpp"

static std::string DetectLocalIP() {
	// Connect a UDP socket to an external address to discover which
	// local interface (and thus which IP) the OS would use for outbound traffic.
	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == INVALID_SOCKET) return "127.0.0.1";

	sockaddr_in probe = {};
	probe.sin_family = AF_INET;
	probe.sin_port   = htons(53);
	probe.sin_addr.s_addr = inet_addr("8.8.8.8");

	if (connect(s, reinterpret_cast<sockaddr*>(&probe), sizeof(probe)) == SOCKET_ERROR) {
		closesocket(s);
		return "127.0.0.1";
	}

	sockaddr_in local = {};
	int len = sizeof(local);
	getsockname(s, reinterpret_cast<sockaddr*>(&local), &len);
	closesocket(s);

	char buf[INET_ADDRSTRLEN] = {};
	const char* result = inet_ntoa(local.sin_addr);
	if (result) strncpy(buf, result, sizeof(buf) - 1);
	return buf;
}

std::string Config::GetIpChar() {
	Logger::Log("config", "GetIpChar\n");
	ParsedOptions options = CommandLineParser::ParseCommandLine();

	if (!options.switches[L"host"].empty()) {
		return CommandLineParser::WideToUtf8(options.switches[L"host"]);
	}

	std::string detected = DetectLocalIP();
	Logger::Log("config", "GetIpChar: auto-detected IP = %s\n", detected.c_str());
	return detected;
}

int Config::GetPort() {
	Logger::Log("config", "GetPort\n");
	ParsedOptions options = CommandLineParser::ParseCommandLine();

	std::wstring port = options.switches[L"port"].empty() ? L"9002" : options.switches[L"port"];

	return std::stoi(CommandLineParser::WideToUtf8(port));
}

std::wstring Config::GetMapUrl() {
	Logger::Log("config", "GetMapUrl\n");

	ParsedOptions options = CommandLineParser::ParseCommandLine();

	std::wstring ip = options.switches[L"host"].empty() ? L"127.0.0.1" : options.switches[L"host"];
	std::wstring port = options.switches[L"port"].empty() ? L"9002" : options.switches[L"port"];

	std::wstring mapName = options.level.mapName;
	std::wstring mapParams;
	std::map<std::wstring, std::wstring> params = options.level.params;

	for (auto& [key, value] : params) {
		mapParams += L"?";
		mapParams += key;
		mapParams += L"=";
		mapParams += value;
	}

	std::wstring url = ip + L":" + port + L"/" + mapName + mapParams;

	return url;
}

std::string Config::GetMapNameChar() {
	ParsedOptions options = CommandLineParser::ParseCommandLine();

	return CommandLineParser::WideToUtf8(options.level.mapName);
}

std::wstring Config::GetMapParams() {
	ParsedOptions options = CommandLineParser::ParseCommandLine();

	std::wstring mapParams;
	std::map<std::wstring, std::wstring> params = options.level.params;
	for (auto& [key, value] : options.level.params) {
		mapParams += L"?";
		mapParams += key;
		mapParams += L"=";
		mapParams += value;
	}

	return mapParams;
}

std::string Config::GetMapParamsChar() {
	ParsedOptions options = CommandLineParser::ParseCommandLine();

	std::string mapParams;
	std::map<std::wstring, std::wstring> params = options.level.params;
	for (auto& [key, value] : options.level.params) {
		mapParams += "?";
		mapParams += CommandLineParser::WideToUtf8(key);
		mapParams += "=";
		mapParams += CommandLineParser::WideToUtf8(value);
	}

	return mapParams;
}

int Config::GetDifficultyValueId() {

	std::string MapName = GetMapNameChar();
	if (MapName == "Inception_ALL" || MapName == "Inception_3_TEMP" || MapName == "Adrenaline_P" || MapName == "Skylark_P" || MapName == "AgencyZero_P") {
		return 1028;
	}

	return 1471;
}

uint16_t Config::GetIpcPort() {
	ParsedOptions options = CommandLineParser::ParseCommandLine();
	std::wstring port = options.switches[L"ipcport"].empty()
	                  ? L"9010" : options.switches[L"ipcport"];
	return static_cast<uint16_t>(std::stoi(CommandLineParser::WideToUtf8(port)));
}

std::string Config::GetIpcHost() {
	ParsedOptions options = CommandLineParser::ParseCommandLine();
	if (!options.switches[L"ipchost"].empty())
		return CommandLineParser::WideToUtf8(options.switches[L"ipchost"]);
	return "127.0.0.1";
}

int64_t Config::GetInstanceId() {
	ParsedOptions options = CommandLineParser::ParseCommandLine();
	std::wstring val = options.switches[L"instanceid"];
	if (val.empty()) return 0;
	return static_cast<int64_t>(std::stoll(CommandLineParser::WideToUtf8(val)));
}

std::string Config::GetGamePath() {
	ParsedOptions options = CommandLineParser::ParseCommandLine();
	if (!options.switches[L"gamepath"].empty())
		return CommandLineParser::WideToUtf8(options.switches[L"gamepath"]);
	return "";
}

bool Config::GetFixPackageGuids() {
	ParsedOptions options = CommandLineParser::ParseCommandLine();
	std::wstring val = options.switches[L"fixpackageguids"];
	if (val.empty()) return true; // default: on
	return val == L"1";
}

std::string Config::GetCrashDir() {
	ParsedOptions options = CommandLineParser::ParseCommandLine();
	if (!options.switches[L"crashdir"].empty())
		return CommandLineParser::WideToUtf8(options.switches[L"crashdir"]);
	// Fallback matches the historical hardcoded value (Wine Z: maps / on host).
	return "Z:\\home\\zax\\games\\crashes";
}

std::string Config::GetLogDir() {
	ParsedOptions options = CommandLineParser::ParseCommandLine();
	if (!options.switches[L"logdir"].empty())
		return CommandLineParser::WideToUtf8(options.switches[L"logdir"]);
	// Fallback matches the historical "C:\<channel>.txt" path inside drive_c.
	return "C:";
}

// Parse a comma-separated list switch into a vector of channel names.
// Empty switch (missing or "-foo=") yields an empty vector — i.e. no channels.
static std::vector<std::string> ParseCsvSwitch(const std::wstring& switchName) {
	std::vector<std::string> out;
	ParsedOptions options = CommandLineParser::ParseCommandLine();
	std::wstring value = options.switches[switchName];
	if (value.empty()) return out;

	std::string utf8 = CommandLineParser::WideToUtf8(value);
	size_t start = 0;
	while (start <= utf8.size()) {
		size_t pos = utf8.find(',', start);
		if (pos == std::string::npos) pos = utf8.size();
		if (pos > start) out.emplace_back(utf8.substr(start, pos - start));
		start = pos + 1;
	}
	return out;
}

std::vector<std::string> Config::GetEnabledChannels() {
	return ParseCsvSwitch(L"enabledchannels");
}

std::vector<std::string> Config::GetEnabledCrashChannels() {
	return ParseCsvSwitch(L"enabledcrashchannels");
}


