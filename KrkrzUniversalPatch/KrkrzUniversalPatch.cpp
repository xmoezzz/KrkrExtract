#include "my.h"

BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

