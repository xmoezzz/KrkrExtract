#ifndef _OutputConsole_
#define _OutputConsole_

#include <Windows.h>

VOID WINAPI OutputString(const WCHAR* name, BOOL ForRelease = FALSE);
VOID WINAPI OutputString(const CHAR*  name, BOOL ForRelease = FALSE);

#endif
