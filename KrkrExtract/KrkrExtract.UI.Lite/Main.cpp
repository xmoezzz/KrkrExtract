#include <my.h>
#include "UIViewer.h"

static UIViewer g_Viewer;

BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	if (Reason == DLL_PROCESS_ATTACH)
	{
		g_Viewer.NotifyDllLoad(hModule);
		LdrDisableThreadCalloutsForDll(hModule);
	}
    return TRUE;
}


#pragma comment(linker, "/EXPORT:KrCreateWindow=_KrCreateWindow@12")
EXTERN_C __declspec(dllexport)
NTSTATUS
NTAPI
KrCreateWindow(
	_In_  PVOID DllModule, 
	_In_  ClientStub* Client, 
	_Out_ ServerStub** Server
)
{
	if (g_Viewer.ActiveUIViewer(DllModule, Client, Server))
		return STATUS_SUCCESS;

	return STATUS_INVALID_PARAMETER;
}




