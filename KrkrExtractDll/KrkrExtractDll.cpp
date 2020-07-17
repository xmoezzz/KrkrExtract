#pragma comment(linker, "/SECTION:.X'moe,ERW /MERGE:.text=.X'moe")

#include "detours.h"
#include "tp_stub.h"
#include "MiniLocale.h"
#include <Windows.h>

#pragma comment(lib, "detours.lib")

extern "C" __declspec(dllexport) HRESULT __stdcall V2Link(iTVPFunctionExporter *exporter)
{
	TVPInitImportStub(exporter);
	return S_OK;
}

extern "C" __declspec(dllexport) HRESULT __stdcall V2Unlink()
{
	return S_OK;
}

