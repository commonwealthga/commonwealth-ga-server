#include "src/Config/Config.hpp"

wchar_t* Config::GetMapUrl() {
	return L"127.0.0.1:9002/Rot_Redistribution05.ut3?Game=TgGame.TgGame_PointRotation";
}

char* Config::GetMapUrlChar() {
	return "127.0.0.1:9002/Rot_Redistribution05.ut3?Game=TgGame.TgGame_PointRotation";
}

wchar_t* Config::GetMapName() {
	return L"Rot_Redistribution05.ut3";
}

char* Config::GetMapNameChar() {
	return "Rot_Redistribution05.ut3";
}

wchar_t* Config::GetMapParams() {
	return L"?Game=TgGame.TgGame_PointRotation";
}

char* Config::GetMapParamsChar() {
	return "?Game=TgGame.TgGame_PointRotation";
}

