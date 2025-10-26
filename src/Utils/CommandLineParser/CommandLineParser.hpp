#pragma once

#include "src/pch.hpp"

struct MapSpec {
    std::wstring mapName;
    std::map<std::wstring, std::wstring> params;
};

struct ParsedOptions {
    std::vector<std::wstring> positional;
    std::map<std::wstring, std::wstring> switches;
    MapSpec level;
};

class CommandLineParser {
private:
	static std::wstring ToLowerW(std::wstring s);
	static std::pair<std::wstring, std::wstring> SplitOnce(const std::wstring &s, wchar_t delim);
	static std::vector<std::wstring> Split(const std::wstring &s, wchar_t delim);
	static std::wstring UrlDecodeToW(const std::wstring &in);
	static std::wstring TrimW(const std::wstring &s);
	static MapSpec ParseMapArg(const std::wstring &mapArg);
	static ParsedOptions ParseArgs(const std::vector<std::wstring> &argv);
public:
	static bool bIsParsed;
	static ParsedOptions CommandLine;
	static ParsedOptions ParseCommandLine();
	static std::string WideToUtf8(const std::wstring &w);
	static std::wstring Utf8ToWide(const std::string &s);
};

