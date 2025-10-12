#include "src/Config/Config.hpp"

wchar_t* Config::GetMapUrl() {
	return L"127.0.0.1:9002/HEX_AVA_Push_Factory1_P.ut3?Game=TgGame.TgGame_Mission";
}

char* Config::GetMapUrlChar() {
	return "127.0.0.1:9002/HEX_AVA_Push_Factory1_P.ut3?Game=TgGame.TgGame_Mission";
}

wchar_t* Config::GetMapName() {
	return L"HEX_AVA_Push_Factory1_P.ut3";
}

char* Config::GetMapNameChar() {
	return "HEX_AVA_Push_Factory1_P.ut3";
}

wchar_t* Config::GetMapParams() {
	return L"?Game=TgGame.TgGame_Mission";
}

char* Config::GetMapParamsChar() {
	return "?Game=TgGame.TgGame_Mission";
}

