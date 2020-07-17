#ifndef _XP3FilterModity_
#define _XP3FilterModify_

#include <Windows.h>
#include "tp_stub.h"
#include "CMem.h"

typedef VOID(WINAPI *TVPXP3ArchiveExtractionFilterFunc)(tTVPXP3ExtractionFilterInfo *info);

typedef struct GLOBAL_INFO
{
	BOOL                        InitDone;
	ULONG                       SizeOfImage;
	ULONG_PTR                   OriginalReturnAddress, FirstSectionAddress, V2Unlink;
	HINSTANCE                   hInstance;
} GLOBAL_INFO;

void WINAPI SetRealFilter(TVPXP3ArchiveExtractionFilterFunc Filter);
TVPXP3ArchiveExtractionFilterFunc WINAPI InitFakeFilter();


#endif
