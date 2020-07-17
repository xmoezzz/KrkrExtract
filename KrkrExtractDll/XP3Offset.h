#ifndef _XP3Offset_
#define _XP3Offset_

#include <Windows.h>
#include "tp_stub.h"
#include "XP3Info.h"
#include "WinFile.h"

HRESULT WINAPI FindEmbededXp3OffsetSlow(const WCHAR* FileName, PLARGE_INTEGER Offset);

#endif
