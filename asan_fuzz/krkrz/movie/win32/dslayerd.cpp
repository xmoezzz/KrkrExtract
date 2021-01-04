/****************************************************************************/
/*! @file
@brief DirectShowを利用したムービーのレイヤー描画再生

-----------------------------------------------------------------------------
	Copyright (C) 2004 T.Imoto <http://www.kaede-software.com>
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2004/08/25
@note
			2004/08/25	T.Imoto		
*****************************************************************************/


#include "tjsCommHead.h"
#include "MsgIntf.h"
#include "SysInitIntf.h"
#include "PluginImpl.h"
#include "dslayerd.h"
#include "CIStream.h"

#include "DShowException.h"
#include "BufferRenderer.h"
#include "OptionInfo.h"
#include "TVPVideoOverlay.h"

//----------------------------------------------------------------------------
//! @brief	  	m_BmpBitsにNULLを設定する
//----------------------------------------------------------------------------
tTVPDSLayerVideo::tTVPDSLayerVideo()
{
	m_BmpBits[0] = NULL;
	m_BmpBits[1] = NULL;
}
//----------------------------------------------------------------------------
//! @brief	  	m_BmpBitsにNULLを設定する
//----------------------------------------------------------------------------
tTVPDSLayerVideo::~tTVPDSLayerVideo()
{
	Shutdown = true;

	m_BmpBits[0] = NULL;
	m_BmpBits[1] = NULL;
	ReleaseAll();
}
//----------------------------------------------------------------------------
//! @brief	  	フィルタグラフの構築
//! @param 		callbackwin : メッセージを送信するウィンドウ
//! @param 		stream : 読み込み元ストリーム
//! @param 		streamname : ストリームの名前
//! @param 		type : メディアタイプ(拡張子)
//! @param 		size : メディアサイズ
//----------------------------------------------------------------------------
void __stdcall tTVPDSLayerVideo::BuildGraph( HWND callbackwin, IStream *stream,
	const wchar_t * streamname, const wchar_t *type, unsigned __int64 size )
{
	HRESULT			hr;

//	CoInitialize(NULL);

	// detect CMediaType from stream's extension ('type')
	try {
		// create IFilterGraph instance
		if( FAILED(hr = m_GraphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC)) )
			ThrowDShowException(L"Failed to create FilterGraph.", hr);

// ログを書き出す時に有効にする。でも、あんまり役に立たないような。。。
#if	0
		{
			HANDLE	hFile = CreateFile( "C:\\krdslog.txt", GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL );
			if( hFile != INVALID_HANDLE_VALUE )
			{
				GraphBuilder()->SetLogFile( (DWORD_PTR)hFile );
			}
		}
#endif

		// Register to ROT
		if(GetShouldRegisterToROT())
		{
			AddToROT(m_dwROTReg);
		}

		// Create the Buffer Renderer object
		CComPtr<IBaseFilter>	pBRender;	// for buffer renderer filter
		TBufferRenderer			*pCBR;
		pCBR = new TBufferRenderer( NAME("Buffer Renderer"), NULL, &hr );
		if( FAILED(hr) )
			ThrowDShowException(L"Failed to create buffer renderer object.", hr);
		pBRender = pCBR;

		if( IsWindowsMediaFile(type) )
		{
			if( FAILED(hr = GraphBuilder()->AddFilter( pBRender, L"Buffer Renderer")) )
				ThrowDShowException(L"Failed to call GraphBuilder()->AddFilter( pBRender, L\"Buffer Renderer\").", hr);

			BuildWMVGraph( pBRender, stream );
		}
		else
		{
			CMediaType mt;
			mt.majortype = MEDIATYPE_Stream;
			ParseVideoType( mt, type ); // may throw an exception
	
			// create proxy filter
			m_Proxy = new CIStreamProxy( stream, size );
			hr = S_OK;
			m_Reader = new CIStreamReader( m_Proxy, &mt, &hr );
			if( FAILED(hr) || m_Reader == NULL )
				ThrowDShowException(L"Failed to create proxy filter object.", hr);
			m_Reader->AddRef();

			// add fliter
			if( FAILED(hr = GraphBuilder()->AddFilter( m_Reader, L"Stream Reader")) )
				ThrowDShowException(L"Failed to call GraphBuilder()->AddFilter( m_Reader, L\"Stream Reader\").", hr);

			// AddFilter したのでリリースする。
			m_Reader->Release();
	
			if( mt.subtype == MEDIASUBTYPE_Avi || mt.subtype == MEDIASUBTYPE_QTMovie )
			{
// GraphBuilderに自動的にグラフを構築させた後、Video Rendererをすげ替える
// 自らグラフを構築していくよりも、AVIファイルへの対応状況が良くなるはず
#if 1
				if( FAILED(hr = GraphBuilder()->Render(m_Reader->GetPin(0))) )
					ThrowDShowException(L"Failed to call IGraphBuilder::Render.", hr);
	
				CComPtr<IBaseFilter>	pRender;
				if( FAILED(hr = FindVideoRenderer( &pRender ) ) )
					ThrowDShowException(L"Failed to call FindVideoRenderer( &pRender ).", hr);
	
				CComPtr<IPin>	pRenderPin;
				pRenderPin = GetInPin(pRender, 0);
	
				// get decoder output pin
				CComPtr<IPin>			pDecoderPinOut;
				if( FAILED(hr = pRenderPin->ConnectedTo( &pDecoderPinOut )) )
					ThrowDShowException(L"Failed to call pRenderPin->ConnectedTo( &pDecoderPinOut ).", hr);
	
				// dissconnect pins
				if( FAILED(hr = pDecoderPinOut->Disconnect()) )
					ThrowDShowException(L"Failed to call pDecoderPinOut->Disconnect().", hr);
				if( FAILED(hr = pRenderPin->Disconnect()) )
					ThrowDShowException(L"Failed to call pRenderPin->Disconnect().", hr);
	
				// remove default render
				if( FAILED(hr = GraphBuilder()->RemoveFilter( pRender ) ) )
					ThrowDShowException(L"Failed to call GraphBuilder->RemoveFilter(pRenderPin).", hr);
	
				if( FAILED(hr = GraphBuilder()->AddFilter( pBRender, L"Buffer Renderer")) )
					ThrowDShowException(L"Failed to call GraphBuilder()->AddFilter( pBRender, L\"Buffer Renderer\").", hr);
	
				CComPtr<IPin>	pRdrPinIn;
				pRdrPinIn = GetInPin(pBRender, 0);
	
				if( FAILED(hr = GraphBuilder()->ConnectDirect( pDecoderPinOut, pRdrPinIn, NULL )) )
					ThrowDShowException(L"Failed to call GraphBuilder()->ConnectDirect( pDecoderPinOut, pRdrPinIn, NULL ).", hr);
#else
				CComPtr<IPin>			pRdrPinIn;
				CComPtr<IPin>			pSrcPinOut;
				if( FAILED(hr = pBRender->FindPin( L"In", &pRdrPinIn )) )
					ThrowDShowException(L"Failed to call pBRender->FindPin( L\"In\", &pRdrPinIn ).", hr);
				pSrcPinOut = m_Reader->GetPin(0);
				if( FAILED(hr = GraphBuilder()->Connect( pSrcPinOut, pRdrPinIn )) )
					ThrowDShowException(L"Failed to call GraphBuilder()->Connect( pSrcPinOut, pRdrPinIn ).", hr);
		
				CComPtr<IPin>			pSpliterPinIn;
				if( FAILED(hr = pSrcPinOut->ConnectedTo( &pSpliterPinIn )) )
					ThrowDShowException(L"Failed to call pSrcPinOut->ConnectedTo( &pSpliterPinIn ).", hr);
		
				{	// Connect to DDS render filter
					CComPtr<IBaseFilter>	pDDSRenderer;	// for sound renderer filter
					if( FAILED(hr = pDDSRenderer.CoCreateInstance(CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER)) )
						ThrowDShowException(L"Failed to create sound render filter object.", hr);
					if( FAILED(hr = GraphBuilder()->AddFilter(pDDSRenderer, L"Sound Renderer")) )
						ThrowDShowException(L"Failed to call GraphBuilder()->AddFilter(pDDSRenderer, L\"Sound Renderer\").", hr);
		
					CComPtr<IBaseFilter>	pSpliter;
					PIN_INFO	pinInfo;
					if( FAILED(hr = pSpliterPinIn->QueryPinInfo( &pinInfo )) )
						ThrowDShowException(L"Failed to call pSpliterPinIn->QueryPinInfo( &pinInfo ).", hr);
					pSpliter = pinInfo.pFilter;
					pinInfo.pFilter->Release();
					if( FAILED(hr = ConnectFilters( pSpliter, pDDSRenderer ) ) )
					{
						if( FAILED(hr = GraphBuilder()->RemoveFilter( pDDSRenderer)) )	// 音無しとみなして、フィルタを削除する
							ThrowDShowException(L"Failed to call GraphBuilder()->RemoveFilter( pDDSRenderer).", hr);
					}
				}
#endif
			}
#ifdef ENABLE_THEORA
			else if( mt.subtype == MEDIASUBTYPE_Ogg )
			{
				if( FAILED(hr = GraphBuilder()->AddFilter( pBRender, L"Buffer Renderer")) )
					ThrowDShowException(L"Failed to call GraphBuilder()->AddFilter( pBRender, L\"Buffer Renderer\").", hr);
				BuildTheoraGraph( pBRender, m_Reader); // may throw an exception
			}
#endif
			else
			{
				tTVPDSFilterHandlerType* handler = TVPGetDSFilterHandler( mt.subtype );
				if( handler )
				{
					if( FAILED(hr = GraphBuilder()->AddFilter( pBRender, L"Buffer Renderer")) )
						ThrowDShowException(L"Failed to call GraphBuilder()->AddFilter( pBRender, L\"Buffer Renderer\").", hr);
					BuildPluginGraph( handler, pBRender, m_Reader );
				}
				else
				{
					if( FAILED(hr = GraphBuilder()->AddFilter( pBRender, L"Buffer Renderer")) )
						ThrowDShowException(L"Failed to call GraphBuilder()->AddFilter( pBRender, L\"Buffer Renderer\").", hr);
					BuildMPEGGraph( pBRender, m_Reader); // may throw an exception
				}
			}
		}

#if 0	// 吉里吉里のBitmapは上下逆の形式らしいので、上下反転のための再接続は必要ない
		{	// Reconnect buffer render filter
			// get decoder output pin
			CComPtr<IPin>			pDecoderPinOut;
			if( FAILED(hr = pRdrPinIn->ConnectedTo( &pDecoderPinOut )) )
				ThrowDShowException(L"Failed to call pRdrPinIn->ConnectedTo( &pDecoderPinOut ).", hr);

			// get connection media type
			CMediaType	mt;
			if( FAILED(hr = pRdrPinIn->ConnectionMediaType( &mt )) )
				ThrowDShowException(L"Failed to call pRdrPinIn->ConnectionMediaType( &mt ).", hr);

			// dissconnect pins
			if( FAILED(hr = pDecoderPinOut->Disconnect()) )
				ThrowDShowException(L"Failed to call pDecoderPinOut->Disconnect().", hr);
			if( FAILED(hr = pRdrPinIn->Disconnect()) )
				ThrowDShowException(L"Failed to call pRdrPinIn->Disconnect().", hr);

			if( IsEqualGUID( mt.FormatType(), FORMAT_VideoInfo) )
			{	// reverse vertical line
				VIDEOINFOHEADER	*pVideoInfo;
				pVideoInfo = reinterpret_cast<VIDEOINFOHEADER*>(mt.Format());
				if( pVideoInfo->bmiHeader.biHeight > 0 )
					pVideoInfo->bmiHeader.biHeight *= -1;
			}

			if( FAILED(hr = GraphBuilder()->ConnectDirect( pDecoderPinOut, pRdrPinIn, &mt )) )
				ThrowDShowException(L"Failed to call GraphBuilder()->ConnectDirect( pDecoderPinOut, pRdrPinIn, &mt ).", hr);
		}
#endif

		// query each interfaces
		if( FAILED(hr = m_GraphBuilder.QueryInterface( &m_MediaControl )) )
			ThrowDShowException(L"Failed to query IMediaControl", hr);
		if( FAILED(hr = m_GraphBuilder.QueryInterface( &m_MediaPosition )) )
			ThrowDShowException(L"Failed to query IMediaPosition", hr);
		if( FAILED(hr = m_GraphBuilder.QueryInterface( &m_MediaSeeking )) )
			ThrowDShowException(L"Failed to query IMediaSeeking", hr);
		if( FAILED(hr = m_GraphBuilder.QueryInterface( &m_MediaEventEx )) )
			ThrowDShowException(L"Failed to query IMediaEventEx", hr);

		if( FAILED(hr = m_GraphBuilder.QueryInterface( &m_BasicVideo )) )
			ThrowDShowException(L"Failed to query IBasicVideo", hr);
//		m_GraphBuilder.QueryInterface( &m_BasicAudio );
		if( FAILED(hr = m_GraphBuilder.QueryInterface( &m_BasicAudio )) )
			ThrowDShowException(L"Failed to query IBasicAudio", hr);

		if( FAILED(hr = pBRender->QueryInterface( &m_BuffAccess )) )
			ThrowDShowException(L"Failed to query IRendererBufferAccess.", hr);
		if( FAILED(hr = pBRender->QueryInterface( &m_BuffVideo )) )
			ThrowDShowException(L"Failed to query IRendererBufferVideo.", hr);

//		if( FAILED(hr = MediaSeeking()->SetTimeFormat( &TIME_FORMAT_FRAME )) )
//			ThrowDShowException(L"Failed to call IMediaSeeking::SetTimeFormat.", hr);

		// set notify event
		if(callbackwin)
		{
			if(FAILED(Event()->SetNotifyWindow((OAHWND)callbackwin, WM_GRAPHNOTIFY, (long)(this))))
				ThrowDShowException(L"Failed to set IMediaEventEx::SetNotifyWindow.", hr);
		}
	}
	catch(const wchar_t *msg)
	{
		MakeAPause(true);
		ReleaseAll();
//		CoUninitialize();
		TVPThrowExceptionMessage(msg);
	}
	catch(...)
	{
		MakeAPause(true);
		ReleaseAll();
//		CoUninitialize();
		throw;
	}

	MakeAPause(false);
//	CoUninitialize();	// ここでこれを呼ぶとまずそうな気がするけど、大丈夫なのかなぁ
}

//----------------------------------------------------------------------------
//! @brief	  	インターフェイスを解放する
//----------------------------------------------------------------------------
void __stdcall tTVPDSLayerVideo::ReleaseAll()
{
	if( m_MediaControl.p != NULL )
	{
		m_MediaControl->Stop();
	}

	if( m_BuffAccess.p )
		m_BuffAccess.Release();

	if( m_BuffVideo.p )
		m_BuffVideo.Release();

	tTVPDSMovie::ReleaseAll();
}
//----------------------------------------------------------------------------
//! @brief	  	描画するバッファを設定する
//! @param		buff1 : バッファ1
//! @param		buff2 : バッファ2
//! @param		size : バッファのサイズ
//----------------------------------------------------------------------------
void __stdcall tTVPDSLayerVideo::SetVideoBuffer( BYTE *buff1, BYTE *buff2, long size )
{
	if( buff1 == NULL || buff2 == NULL )
		TVPThrowExceptionMessage(L"SetVideoBuffer Parameter Error");

	m_BmpBits[0] = buff1;
	m_BmpBits[1] = buff2;
	HRESULT hr;
	if( FAILED(hr = BufferAccess()->SetFrontBuffer( m_BmpBits[0], &size )) )
		ThrowDShowException(L"Failed to call IBufferAccess::SetFrontBuffer.", hr);
	if( FAILED(hr = BufferAccess()->SetBackBuffer( m_BmpBits[1], &size )) )
		ThrowDShowException(L"Failed to call IBufferAccess::SetBackBuffer.", hr);
}
//----------------------------------------------------------------------------
//! @brief	  	フロントバッファを取得する
//! @param		buff : バッファ
//----------------------------------------------------------------------------
void __stdcall tTVPDSLayerVideo::GetFrontBuffer( BYTE **buff )
{
	long		size;
	HRESULT hr;
	if( FAILED(hr = BufferAccess()->GetFrontBuffer( buff, &size )) )
		ThrowDShowException(L"Failed to call IBufferAccess::GetFrontBuffer.", hr);
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオの画像サイズを取得する
//! @param		width : 幅
//! @param		height : 高さ
//----------------------------------------------------------------------------
void __stdcall tTVPDSLayerVideo::GetVideoSize( long *width, long *height )
{
	if( width != NULL )
		BufferVideo()->get_VideoWidth( width );

	if( height != NULL )
		BufferVideo()->get_VideoHeight( height );
}
//----------------------------------------------------------------------------
//! @brief	  	1フレームの平均表示時間を取得します
//! @param		pAvgTimePerFrame : 1フレームの平均表示時間
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT __stdcall tTVPDSLayerVideo::GetAvgTimePerFrame( REFTIME *pAvgTimePerFrame )
{
	return BufferVideo()->get_AvgTimePerFrame( pAvgTimePerFrame );
}
