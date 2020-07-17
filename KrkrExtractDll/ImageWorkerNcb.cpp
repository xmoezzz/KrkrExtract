#include <windows.h>
#include "ncbind.h"

//---------------------------------------------------------------------------
static tjs_int GlobalRefCountAtInit = 0;

HRESULT WINAPI ImageWorkerV2Link(iTVPFunctionExporter *exporter)
{
	NCB_LOG_W("V2Link");

	ncbAutoRegister::AllRegist();
	GlobalRefCountAtInit = TVPPluginGlobalRefCount;
	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT WINAPI ImageWorkerV2Unlink()
{
	if (TVPPluginGlobalRefCount > GlobalRefCountAtInit) 
	{
		NCB_LOG_W("V2Unlink ...failed");
		return E_FAIL;
	}
	else 
	{
		NCB_LOG_W("V2Unlink");
	}
	ncbAutoRegister::AllUnregist();
	return S_OK;
}


ncbAutoRegister::ThisClassT const*
ncbAutoRegister::_top[ncbAutoRegister::LINE_COUNT] = NCB_INNER_AUTOREGISTER_LINES_INSTANCE;

