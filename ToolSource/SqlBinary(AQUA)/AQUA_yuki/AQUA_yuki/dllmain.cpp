#pragma comment(linker, "/SECTION:.text,ERW /MERGE:.rdata=.text /MERGE:.data=.text")
#pragma comment(linker, "/SECTION:.Xmoe,ERW /MERGE:.text=.Xmoe")


#include <Windows.h>
#include "tp_stub.h"
#include "AQUA.h"

extern void InitCritical();

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  Reason,  LPVOID lpReserved )
{
	return TRUE;
}


#pragma comment(linker, "/EXPORT:V2Link=_V2Link@4,PRIVATE")
extern "C" __declspec(dllexport) HRESULT _stdcall V2Link(iTVPFunctionExporter *exporter)
{
	TVPInitImportStub(exporter);
	
	TVPSetXP3ArchiveExtractionFilter(TVPXP3ArchiveExtractionFilter);

	return S_OK;
}

#pragma comment(linker, "/EXPORT:V2Unlink=_V2Unlink@0,PRIVATE")
extern "C" __declspec(dllexport) HRESULT _stdcall V2Unlink()
{
	TVPSetXP3ArchiveExtractionFilter(NULL);
	TVPUninitImportStub();

	//cxdec_release();
	return S_OK;
}

