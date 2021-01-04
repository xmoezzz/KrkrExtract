/****************************************************************************/
/*! @file
@brief part of KRLMOVIE.DLL

実行にはDirectX9以降が必要
-----------------------------------------------------------------------------
	Copyright (C) 2005 T.Imoto <http://www.kaede-software.com>
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2005/09/25
@note
			2005/09/25	T.Imoto		作成
*****************************************************************************/

#include "tjsCommHead.h"
#include "SysInitIntf.h"
#include "PluginImpl.h"
#include "dsmixer.h"
#include "krmovie.h"

#include "asyncio.h"
#include "asyncrdr.h"

#include "OptionInfo.h"


//----------------------------------------------------------------------------
//! @brief	  	VideoOverlay Object (ミキシング再生用) を取得する
//! @param		callbackwin : コールバックウィンドウのハンドル
//! @param		stream : 入力ストリーム
//! @param		streamname : ストリームの名前
//! @param		type : ストリームの拡張子
//! @param		size : ストリームのサイズ
//! @param		out : VideoOverlay Object
//! @return		エラー文字列
//----------------------------------------------------------------------------
void __stdcall GetMixingVideoOverlayObject(
	HWND callbackwin, IStream *stream, const wchar_t * streamname,
	const wchar_t *type, unsigned __int64 size, iTVPVideoOverlay **out)
{
	*out = new tTVPDSMixerVideoOverlay;

	if( *out )
		static_cast<tTVPDSMixerVideoOverlay*>(*out)->BuildGraph( callbackwin, stream, streamname, type, size );
}
