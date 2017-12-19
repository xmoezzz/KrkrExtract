#include <my.h>


BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		ExceptionBox(L"Nothing to do...(64Bit Version)");
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}





