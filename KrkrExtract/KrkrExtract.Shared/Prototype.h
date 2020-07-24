#pragma once

#include <my.h>
#include "tp_stub.h"


namespace Prototype
{
	//
	// Prototype
	//

	using SpecialChunkDecoderFunc = ULONG(CDECL*) (PBYTE, PBYTE, ULONG);
	using V2LinkFunc              = HRESULT(NTAPI*)(iTVPFunctionExporter *);
	using SetXP3FilterFunc        = void (NTAPI *)(tTVPXP3ArchiveExtractionFilter);
	using TVPGetFunctionExporterFunc        = iTVPFunctionExporter * (NTAPI *)();
	using TVPXP3ArchiveExtractionFilterFunc = void (NTAPI *)(tTVPXP3ExtractionFilterInfo *Info);
};
