#pragma once

class Config {
public:
	static wchar_t* GetMapUrl();
	static char* GetMapUrlChar();
	static wchar_t* GetMapName();
	static char* GetMapNameChar();
	static wchar_t* GetMapParams();
	static char* GetMapParamsChar();
};

