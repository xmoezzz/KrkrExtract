//---------------------------------------------------------------------------
// ( part of KRMOVIE.DLL )
// (c)2001-2009, W.Dee <dee@kikyou.info> and contributors
//---------------------------------------------------------------------------

/*
	We must separate this module because sucking MS library has a lack of
	compiler portability.

	This requires DirectX7 or later or Windows Media Player 6.4 or later for
	playbacking MPEG streams.


	Modified by T.Imoto <http://www.kaede-software.com>
*/

#ifndef __DSOVERLAY_H__
#define __DSOVERLAY_H__

#include "dsmovie.h"

//----------------------------------------------------------------------------
//! @brief オーバーレイ ビデオ描画クラス
//----------------------------------------------------------------------------
class tTVPDSVideoOverlay : public tTVPDSMovie
{
protected:
	CComPtr<IVideoWindow>	m_VideoWindow;

	//----------------------------------------------------------------------------
	//! @brief	  	IVideoWindowを取得する
	//! @return		IVideoWindowインターフェイス
	//----------------------------------------------------------------------------
	IVideoWindow *VideoWindow()
	{
		assert( m_VideoWindow.p );
		return m_VideoWindow;
	}

public:
	tTVPDSVideoOverlay();
	virtual ~tTVPDSVideoOverlay();

	virtual void __stdcall BuildGraph( HWND callbackwin, IStream *stream,
		const wchar_t * streamname, const wchar_t *type, unsigned __int64 size );

	virtual void __stdcall ReleaseAll();

	virtual void __stdcall SetWindow(HWND window);
	virtual void __stdcall SetMessageDrainWindow(HWND window);
	virtual void __stdcall SetRect(RECT *rect);
	virtual void __stdcall SetVisible(bool b);

	virtual void __stdcall Play();
};

#endif	// __DSOVERLAY_H__
