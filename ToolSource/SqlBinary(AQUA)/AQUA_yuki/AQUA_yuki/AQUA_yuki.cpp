#include <Windows.h>
#include "tp_stub.h"
#include "Cxdec.h"

static cxdec_callback AQUACallBack = 
{
	"AQUA-X'moe",
	{ 0x1A8, 0x776 },
	xcode_building_stage1
};


void TVP_tTVPXP3ArchiveExtractionFilter_CONVENTION
TVPXP3ArchiveExtractionFilter(tTVPXP3ExtractionFilterInfo *info)
{
	if (info->SizeOfSelf != sizeof(tTVPXP3ExtractionFilterInfo))
	{
		TVPThrowExceptionMessage(TJS_W("Incompatible tTVPXP3ExtractionFilterInfo size"));
	}

	cxdec_decode(&AQUACallBack, info->FileHash, info->Offset, (PBYTE)(info->Buffer), (DWORD)(info->BufferSize));
}


