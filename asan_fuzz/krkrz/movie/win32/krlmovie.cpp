/****************************************************************************/
/*! @file
@brief part of KRLMOVIE.DLL

-----------------------------------------------------------------------------
	Copyright (C) 2004 T.Imoto <http://www.kaede-software.com>
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2004/09/22
@note
			2004/09/22	T.Imoto		
*****************************************************************************/

#include "tjsCommHead.h"
#include "SysInitIntf.h"
#include "PluginImpl.h"
#include "dslayerd.h"
#include "krmovie.h"

#include "asyncio.h"
#include "asyncrdr.h"

#include "OptionInfo.h"


//----------------------------------------------------------------------------
//! @brief	  	VideoOverlay Object (レイヤ再生用) を取得する
//! @param		callbackwin : 
//! @param		stream : 
//! @param		streamname : 
//! @param		type : 
//! @param		size : 
//! @param		out : VideoOverlay Object
//! @return		エラー文字列
//----------------------------------------------------------------------------
void __stdcall GetVideoLayerObject(
	HWND callbackwin, IStream *stream, const wchar_t * streamname,
	const wchar_t *type, unsigned __int64 size, iTVPVideoOverlay **out)
{
	*out = new tTVPDSLayerVideo;

	if( *out )
		static_cast<tTVPDSLayerVideo*>(*out)->BuildGraph( callbackwin, stream, streamname, type, size );
}
