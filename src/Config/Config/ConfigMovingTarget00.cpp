#include "src/Config/Config.hpp"

wchar_t* Config::GetMapUrl() {
	return L"127.0.0.1:9002/Moving_Target00.ut3?Game=TgGame.TgGame_Defense";
}

char* Config::GetMapUrlChar() {
	return "127.0.0.1:9002/Moving_Target00.ut3?Game=TgGame.TgGame_Defense";
}

wchar_t* Config::GetMapName() {
	return L"Moving_Target00.ut3";
}

char* Config::GetMapNameChar() {
	return "Moving_Target00.ut3";
}

wchar_t* Config::GetMapParams() {
	return L"?Game=TgGame.TgGame_Defense";
}

char* Config::GetMapParamsChar() {
	return "?Game=TgGame.TgGame_Defense";
}

