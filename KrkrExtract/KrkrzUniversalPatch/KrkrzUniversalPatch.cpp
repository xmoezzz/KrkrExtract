#include "my.h"
#include "KaresekaHook.h"

BOOL FASTCALL KaresekaInit(PVOID hModule)
{
	KaresekaHook* Kareseka;
	Kareseka = GetKareseka();
	return NT_SUCCESS(Kareseka->Init((HMODULE)hModule));
}


BOOL FASTCALL KaresekaUnInit(PVOID hModule)
{
	UNREFERENCED_PARAMETER(hModule);
	return TRUE;
}


BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		return KaresekaInit(hModule);

	case DLL_PROCESS_DETACH:
		return KaresekaUnInit(hModule);
	}
	return TRUE;
}

