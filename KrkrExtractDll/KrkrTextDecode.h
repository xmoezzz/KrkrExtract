#ifndef _KrkrTextExtract_
#define _KrkrTextExtract_

#include <Windows.h>

int WINAPI DecodeText(PBYTE Buffer, ULONG RawFileSize, HWND hWnd, const WCHAR* lpOutFileName);

#endif
