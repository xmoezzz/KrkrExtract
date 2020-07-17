#pragma once

#include <Windows.h>
#include <string>

int WINAPI ExtractPsb(IStream* InFile, BOOL Image, BOOL Script, const std::wstring& ExtName, const WCHAR* lpOutName);


