#pragma once
#include "src/pch.hpp"
#include <string>

namespace MapDumpWriters {
	void WriteTgBotFactoryLocationList(sqlite3* db, ATgBotFactory* factory,
	                                   const std::string& mapName,
	                                   int mapObjectId);
}
