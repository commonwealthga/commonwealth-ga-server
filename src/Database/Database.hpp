#pragma once

#include "lib/sqlite3/sqlite3.h"

class Database {
private:
	static sqlite3* connection;
public:
	static int Callback(void* data, int argc, char** argv, char** azColName);
	static sqlite3* GetConnection();
	static void CloseConnection();
	static void Init();
};

