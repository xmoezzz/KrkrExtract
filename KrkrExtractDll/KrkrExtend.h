#pragma once

#include "my.h"
#include <string>

using std::wstring;

int WINAPI ExtractPsb(IStream* InFile, BOOL Image, BOOL Script, const wstring& ExtName, const WCHAR* lpOutName);
int WINAPI DecodeText(PBYTE Buffer, ULONG RawFileSize, LPCWSTR lpOutFileName);