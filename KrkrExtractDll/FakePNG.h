#ifndef _FackPNG_
#define _FackPNG_

#include "tp_stub.h"
#include <Windows.h>
#include <string>

using std::wstring;

void WINAPI InitLayer();
void WINAPI SavePng(const wchar_t * filename, const wchar_t * path);

#endif
