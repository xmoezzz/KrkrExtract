#pragma once

#include <Windows.h>
#include <string>

using std::wstring;

int WINAPI GenSha1Code(WCHAR* pStr, wstring& Result);
int WINAPI GenSha1CodeConst(const WCHAR* pCStr, wstring& Result);


