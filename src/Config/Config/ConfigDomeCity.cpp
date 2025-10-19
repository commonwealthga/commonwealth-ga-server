#include "src/Config/Config.hpp"

wchar_t* Config::GetMapUrl() {
	return L"127.0.0.1:9002/DomeCity_P_VER_3.ut3?Game=TgGame.TgGame_City";
}

char* Config::GetMapUrlChar() {
	return "127.0.0.1:9002/DomeCity_P_VER_3.ut3?Game=TgGame.TgGame_City";
}

wchar_t* Config::GetMapName() {
	return L"DomeCity_P_VER_3.ut3";
}

char* Config::GetMapNameChar() {
	return "DomeCity_P_VER_3.ut3";
}

wchar_t* Config::GetMapParams() {
	return L"?Game=TgGame.TgGame_City";
}

char* Config::GetMapParamsChar() {
	return "?Game=TgGame.TgGame_City";
}

