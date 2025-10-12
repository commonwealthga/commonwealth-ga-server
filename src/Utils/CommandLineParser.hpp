#pragma once

#include "src/pch.hpp"

class CommandLineParser {

public:
	static inline std::vector<std::wstring> GetCommandLineArgs() {
		int argc = 0;
		wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		std::vector<std::wstring> out;
		if (argv)
		{
			out.reserve(argc);
			for (int i = 0; i < argc; ++i)
				out.emplace_back(argv[i]);
			LocalFree(argv);
		}
		return out;
	}
};

