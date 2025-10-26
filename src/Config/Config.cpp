#include "src/Config/Config.hpp"
#include "src/Utils/CommandLineParser/CommandLineParser.hpp"
#include "src/Utils/Logger/Logger.hpp"

std::string Config::GetIpChar() {
	Logger::Log("config", "GetIpChar\n");
	ParsedOptions options = CommandLineParser::ParseCommandLine();

	std::wstring ip = options.switches[L"host"].empty() ? L"127.0.0.1" : options.switches[L"host"];

	return CommandLineParser::WideToUtf8(ip);
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

