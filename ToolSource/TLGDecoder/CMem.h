#ifndef _Cmem_
#define _Cmem_

#include <Windows.h>

class CMem
{
public:

	static void* WINAPI Alloc(DWORD Size)
	{
		return HeapAlloc(GetProcessHeap(), 0, Size);
	}

	static void WINAPI Free(void* First)
	{
		HeapFree(GetProcessHeap(), 0, First);
	}
};


#endif
