#pragma once

#include "my.h"
#include "md5.h"
#include <string>

using std::wstring;

Void WINAPI GenMD5Code(const WCHAR* FileName, wstring& OutHex);
Void WINAPI GenMD5Code(const WCHAR* FileName, const WCHAR* ProjectName, wstring& OutHex);

