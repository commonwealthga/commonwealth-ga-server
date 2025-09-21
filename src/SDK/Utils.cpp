//===============================================================================================
//	CrimeCraft Hack By The1domo
//	i used TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
//	Credits: TheFeckless, uNrEaL, Tamimego, SystemFiles, R00T88, _silencer, K@N@VEL, R13
//	Thanks: HOOAH07, lowHertz
//	Forums: www.uc-forum.com, www.gamedeception.net, gh05tn3t.net
//===============================================================================================
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include "Utils.h"


using namespace std;

void Utils::AllocateConsole(const char* pTitle)
{
	// Allocate Console Window
	AllocConsole() ;
	AttachConsole( GetCurrentProcessId() );
	freopen( "CON", "w", stdout ) ;
	SetConsoleTitleA( pTitle );

	// Resize console (max length)
	COORD cordinates = {80, 32766};
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleScreenBufferSize(handle, cordinates);
}

DWORD Utils::FindPattern( DWORD dwStart, DWORD dwLen, BYTE* pszPatt, char pszMask[] )
{
	unsigned int i = NULL;
	int iLen = strlen( pszMask ) - 1;

	for( DWORD dwRet = dwStart; dwRet < dwStart + dwLen; dwRet++ )
	{
		if( *(BYTE*)dwRet == pszPatt[i] || pszMask[i] == '?' )
		{
			if( pszMask[i+1] == '\0' )
				return( dwRet - iLen );
			i++;
		} 
		else 
			i = NULL;
	}
	return NULL;
}

MODULEINFO Utils::GetModuleInfo ( LPCTSTR lpModuleName )
{
	MODULEINFO miInfos = { NULL };

	HMODULE hmModule = GetModuleHandle ( lpModuleName );

	if ( hmModule )
	{
		GetModuleInformation ( GetCurrentProcess(), hmModule, &miInfos, sizeof ( MODULEINFO ) );
	}

	return miInfos;
}

ofstream	ofile;
void Utils::add_log( char* LOG_FILE, const char *fmt, ... )
{
	ofile.open( LOG_FILE, ios::app );

	va_list va_alist;
	char logbuf[256] = {0};

	va_start( va_alist, fmt );
	vsnprintf( logbuf + strlen(logbuf), sizeof(logbuf) - strlen(logbuf), fmt, va_alist );
	va_end( va_alist );

	ofile << logbuf << endl;

	ofile.close();
}