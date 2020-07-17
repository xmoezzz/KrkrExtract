#ifndef _PNG_WORKER_
#define _PNG_WORKER_

#include "PNGDLib.h"
#pragma comment(lib, "libpng16.lib")

#define B2P_ERROR 0
#define B2P_OK    1

int Bmp2PNG(PBYTE Buffer, ULONG Size, const TCHAR *pngfn);

#endif
