#include <my.h>
#include "KrkrExtract.h"
#include "tp_stub.h"
#include "mt64.h"
#include "XP3Parser.h"

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Dbghelp.lib")


#pragma comment(linker, "/EXPORT:V2Link=_V2Link@4")
EXTERN_C MY_DLL_EXPORT HRESULT NTAPI V2Link(iTVPFunctionExporter *exporter)
{
	TVPInitImportStub(exporter);
	return S_OK;
}

#pragma comment(linker, "/EXPORT:V2Unlink=_V2Unlink@0")
EXTERN_C MY_DLL_EXPORT HRESULT NTAPI V2Unlink()
{
	return S_OK;
}

#pragma comment(linker, "/EXPORT:GetKrkrExtractVersion=_GetKrkrExtractVersion@0")
EXTERN_C MY_DLL_EXPORT
PCWSTR
NTAPI
GetKrkrExtractVersion()
{
	return L"5.0.0.0";
}


static KrkrExtractCore* g_Engine = nullptr;

BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	NTSTATUS         Status;

	UNREFERENCED_PARAMETER(lpReserved);

	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		AllocConsole();
		LdrDisableThreadCalloutsForDll(hModule);

		MessageBoxW(0, 0, 0, 0);
		g_Engine = new (std::nothrow) KrkrExtractCore();
		if (!g_Engine) 
		{
			PrintConsoleW(L"no memory for KrkrExtract");
			Ps::ExitProcess(-1);
		}

		Status = g_Engine->Initialize(hModule);
		if (NT_FAILED(Status))
		{
			ExceptionBox(L"Initialization failed!");
			Ps::ExitProcess(-1);
		}
	break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

