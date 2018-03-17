#pragma once
#include "my.h"
#include <string>

using std::wstring;

int WINAPI CompilePsbText(PBYTE PsbBuffer, ULONG PsbSize, PBYTE TextBuffer, ULONG TextSize, PBYTE& OutBuffer, ULONG& OutSize);


NTSTATUS NTAPI QueryFilePrototype(LPCWSTR FileName, PBYTE& Buffer, ULONG& Size);
NTSTATUS NTAPI CompilePsbFull(wstring& FileName, PBYTE& OutBuffer, ULONG& OutSize, API_POINTER(QueryFilePrototype) QueryFileFunc);
