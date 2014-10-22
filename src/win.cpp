#include "gameplay.h"

//#define WIN32_LEAN_AND_MEAN 
#include <iostream>
#include <Windows.h>
#include <shellapi.h>

using namespace gameplay;

void usleep(unsigned long microseconds)
{
	int ms = (int) (microseconds / 1000);
	if (ms) {
		::Sleep(ms);
	}
	//print("Sleep(%d)\n", ms);
}

void winLaunchFile(const char *pFile)
{
    size_t newsize = strlen(pFile) + 1;
    wchar_t * wcstring = new wchar_t[newsize];
    size_t convertedChars = 0;

    mbstowcs_s(&convertedChars, wcstring, newsize, pFile, _TRUNCATE);

//	::ShellExecute(NULL, L"open", wcstring, NULL, NULL, SW_SHOWDEFAULT);

	delete[] wcstring;
}