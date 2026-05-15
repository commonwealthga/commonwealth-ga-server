#pragma once
#include "src/pch.hpp"
#include <string>

namespace MapDumpWriters {
	void WriteTgBotFactory(sqlite3* db, AActor* actor,
	                       const std::string& mapName,
	                       const std::string& className,
	                       int mapObjectId);
}
