#pragma once
//===============================================================================================
//	CrimeCraft Hack By The1domo
//	i used TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
//	Credits: TheFeckless, uNrEaL, Tamimego, SystemFiles, R00T88, _silencer, K@N@VEL, R13
//	Thanks: HOOAH07, lowHertz
//	Forums: www.uc-forum.com, www.gamedeception.net, gh05tn3t.net
//===============================================================================================

#ifndef UTILS_H
#define UTILS_H

// #pragma comment(lib, "Psapi.lib")
#include <psapi.h>

class Utils
{
public:
	static DWORD FindPattern( DWORD dwStart, DWORD dwLen, BYTE* pszPatt, char pszMask[] );
	static void AllocateConsole(const char* pTitle);
	static MODULEINFO GetModuleInfo ( LPCTSTR lpModuleName );
	static void add_log( char* LOG_FILE, const char *fmt, ... );
};

#endif
