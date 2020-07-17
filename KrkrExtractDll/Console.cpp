#include "Console.h"

VOID WINAPI OutputString(const WCHAR* name, BOOL ForRelease)
{
	if (!ForRelease)
		return;

	HANDLE hOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD nRet = 0;
	WriteConsole(hOutputHandle, name, lstrlenW(name), &nRet, NULL);
}


VOID WINAPI OutputString(const CHAR* name, BOOL ForRelease)
{
	if (!ForRelease)
		return;

	HANDLE hOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD nRet = 0;
	WriteConsoleA(hOutputHandle, name, lstrlenA(name), &nRet, NULL);
}
