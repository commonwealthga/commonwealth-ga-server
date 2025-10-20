#include "src/Config/Config.hpp"

wchar_t* Config::GetMapUrl() {
	return L"127.0.0.1:9002/CTR_Recursive_P.ut3?Game=TgGame.TgGame_CTF";
}

char* Config::GetMapUrlChar() {
	return "127.0.0.1:9002/CTR_Recursive_P.ut3?Game=TgGame.TgGame_CTF";
}

wchar_t* Config::GetMapName() {
	return L"CTR_Recursive_P.ut3";
}

char* Config::GetMapNameChar() {
	return "CTR_Recursive_P.ut3";
}

wchar_t* Config::GetMapParams() {
	return L"?Game=TgGame.TgGame_CTF";
}

char* Config::GetMapParamsChar() {
	return "?Game=TgGame.TgGame_CTF";
}

