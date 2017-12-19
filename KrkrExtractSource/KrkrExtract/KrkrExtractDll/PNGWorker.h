#pragma once

#include "my.h"

int Bmp2PNG(PBYTE Buffer, ULONG Size, const WCHAR *pngfn);
int Bmp2JPG(PBYTE Buffer, ULONG Size, const WCHAR *Path);
