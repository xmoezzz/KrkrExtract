#pragma once

#include "my.h"
#include "ml.h"

int WINAPI ExtractPsb(IStream* InFile, BOOL Image, BOOL Script, const wstring& ExtName, LPCWSTR lpOutName);
int WINAPI DecodeText(PBYTE Buffer, ULONG RawFileSize, LPCWSTR lpOutFileName);
