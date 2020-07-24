#include "KrkrExtract.h"

BOOL NTAPI KrkrExtractCore::EnumXp3Plugin(void* Param, ENUMXP3PLUGINPROC Proc)
{
	if (!Proc)
		return FALSE;

	for (auto Xp3Plugin : m_Xp3PluginsVersion1) 
	{
		if (!Proc(Param, Xp3Plugin))
			break;
	}

	return TRUE;
}


//
// not available now
//
BOOL NTAPI KrkrExtractCore::EnumXp3Plugin2(void* Param, ENUMXP3PLUGINPROC2 Proc)
{
	return FALSE;
}

