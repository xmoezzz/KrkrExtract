/****************************************************************************/
/*! @file
@brief DirectShow

バッファヘレンダリングする
-----------------------------------------------------------------------------
	Copyright (C) 2004 T.Imoto
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2004/08/05
@note
			2004/08/05	T.Imoto		
*****************************************************************************/

#include "tjsCommHead.h"
#include <streams.h>
#include <atlbase.h>
#include "BufferRenderer.h"

#ifdef _DEBUG
#include <stdio.h>
#include "DShowException.h"
#endif

//----------------------------------------------------------------------------
//##	TBufferRenderer
//----------------------------------------------------------------------------
//! @brief	  	This goes in the factory template table to create new filter instances
//! @param		pUnk : 集成した IUnknown インターフェイスへのポインタ。
//! @param		phr : メソッドの成功・失敗を示す HRESULT 値を受け取る変数へのポインタ。
//----------------------------------------------------------------------------
CUnknown * WINAPI TBufferRenderer::CreateInstance( LPUNKNOWN pUnk, HRESULT *phr )
{
	CUnknown	*punk = new TBufferRenderer( NAME("Buffer Renderer"), pUnk, phr );
	if( punk == NULL )
		*phr = E_OUTOFMEMORY;
	return punk;
}
#pragma warning(disable: 4355)	// コンストラクタのベースメンバ初期化時にthisを使うとワーニングが出るのでそれを抑止
//----------------------------------------------------------------------------
//! @brief	  	TBufferRenderer constructor
//! @param		pName : デバッグのために使用される記述へのポインタ。
//! @param		pUnk : 集成された所有者オブジェクトへのポインタ。
//! @param		phr : HRESULT 値へのポインタ。
//----------------------------------------------------------------------------
TBufferRenderer::TBufferRenderer( TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr )
 : CBaseVideoRenderer( CLSID_BufferRenderer, pName, pUnk, phr )
, m_InputPin( this, &m_InterfaceLock, phr, L"Input" )
, m_Allocator( this, NAME("Allocator"), GetOwner(), phr )
{
	//CBaseRender::m_pInputPinにポインタを設定する。
	m_pInputPin = &m_InputPin;

	// Store and AddRef the texture for our use.
	*phr = S_OK;
	m_Buffer[0] = NULL;
	m_Buffer[1] = NULL;

	m_IsBufferOwner[0] = false;
	m_IsBufferOwner[1] = false;

	m_FrontBuffer = 0;

	m_StartFrame = 0;
}
#pragma warning(default: 4355)
//----------------------------------------------------------------------------
//! @brief	  	TBufferRenderer destructor
//----------------------------------------------------------------------------
TBufferRenderer::~TBufferRenderer()
{
	//CBaseRender::m_pInputPinにポインタをリセットする。
	//これをしないとCBaseRenderのデストラクタでdeleteされてしまうので注意！
    m_pInputPin = NULL;

	// 自分で確保している場合バッファの解放
	FreeFrontBuffer();
	FreeBackBuffer();
}
//----------------------------------------------------------------------------
//! @brief	  	要求されたインターフェイスを返す
//! 
//! Overriden to say what interfaces we support and where
//! @param		riid : インターフェイスのIID
//! @param		ppv : インターフェイスを返すポインターへのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP TBufferRenderer::NonDelegatingQueryInterface( REFIID riid, void **ppv )
{
	CheckPointer( ppv, E_POINTER );
	if(riid == IID_IRendererBufferAccess) {
		*ppv = static_cast<IRendererBufferAccess*>(this);
		static_cast<IUnknown*>(*ppv)->AddRef();
		return S_OK;
	} else if(riid == IID_IRendererBufferVideo) {
		*ppv = static_cast<IRendererBufferVideo*>(this);
		static_cast<IUnknown*>(*ppv)->AddRef();
		return S_OK;
	}
	return CBaseVideoRenderer::NonDelegatingQueryInterface( riid, ppv );
}
//----------------------------------------------------------------------------
//! @brief	  	特定のメディア タイプをフィルタが受け入れるかどうかを確認する
//! 
//! This method forces the graph to give us an R8G8B8 video type, making our copy 
//! to texture memory trivial.
//! @param		pmt : 提案されたメディア タイプを含む CMediaType オブジェクトへのポインタ
//! @return		提案されたメディア タイプが受け入れられるなら S_OK を返す。
//!				そうでなければ S_FALSE かエラー コードを返す。
//----------------------------------------------------------------------------
HRESULT TBufferRenderer::CheckMediaType( const CMediaType *pmt )
{
	HRESULT		hr = E_FAIL;
	VIDEOINFO	*pvi;

	// Reject the connection if this is not a video type
	if( *pmt->FormatType() != FORMAT_VideoInfo )
		return E_INVALIDARG;

	// Only accept RGB32
	pvi = (VIDEOINFO *)pmt->Format();
	if( IsEqualGUID( *pmt->Type(), MEDIATYPE_Video) )
	{
		if( IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_RGB32) || 
			IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_ARGB32) )
		{
			hr = S_OK;
			m_MtIn = (*pmt);
		}
	}

	return hr;
}
//----------------------------------------------------------------------------
//! @brief	  	Graph connection has been made. 
//! @param		pmt : メディア タイプを指定する CMediaType オブジェクトへのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT TBufferRenderer::SetMediaType( const CMediaType *pmt )
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション

	// Retrive the size of this media type
	VIDEOINFO *pviBmp;						// Bitmap info header
	pviBmp = (VIDEOINFO *)pmt->Format();
	m_VideoWidth  = pviBmp->bmiHeader.biWidth;
	m_VideoHeight = abs(pviBmp->bmiHeader.biHeight);
	m_VideoPitch = m_VideoWidth * 4;	// RGB32に決め打ち

	if( !IsAllocatedFrontBuffer() )
		AllocFrontBuffer( GetBufferSize() );

	if( !IsAllocatedBackBuffer() )
		AllocBackBuffer( GetBufferSize() );

	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	A sample has been delivered. Copy it to the texture.
//! @param		pSample : サンプルの IMediaSample インターフェイスへのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT TBufferRenderer::DoRenderSample( IMediaSample * pSample )
{
	DWORD	*pBmpBuffer, *pTxtBuffer;	// Bitmap buffer, texture buffer
	BYTE	*pTxtOrgPos;

//	if( m_bEOS ) return S_OK;

	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション

	// Get the video bitmap buffer
	pSample->GetPointer( reinterpret_cast<BYTE**>(&pBmpBuffer) );

	// Get the texture buffer & pitch
	pTxtBuffer = reinterpret_cast<DWORD*>(GetBackBuffer());
	pTxtOrgPos = reinterpret_cast<BYTE*>(pTxtBuffer);

	HRESULT		hr;
	LONG		EventParam1 = -1;
	LONGLONG	TimeStart = 0;
	LONGLONG	TimeEnd = 0;

	if( SUCCEEDED(hr = pSample->GetMediaTime( &TimeStart, &TimeEnd )) )
	{
		EventParam1 = (LONG)TimeStart;
	}
	if( m_StopFrame && EventParam1 >= m_StopFrame )
		return S_OK;	// 再生しないフレーム

	if( pTxtBuffer == pBmpBuffer )	// 自前のアロケーターが使われている
	{
		SwapBuffer( pSample );	// FrontとBackバッファを入れ替える
		if( m_pSink )
			m_pSink->Notify( EC_UPDATE, EventParam1, NULL );
		return S_OK;
	}

	// 自前のアロケーターではないのでメモリをコピーする
#if 0
	// 下から上にコピー(上下反転化)
	{
		int		height = m_VideoHeight;
		int		width = m_VideoWidth;
		pBmpBuffer += width * (height-1);
		for( int j = 0; j < height; j++ )
		{
			for( int i = 0; i < width; i++ )
			{
				pTxtBuffer[i] = pBmpBuffer[i];
			}
			pBmpBuffer -= width;
			pTxtBuffer += width;
		}
	}
#else
	// 上から下にコピー
	{
		int		height = m_VideoHeight;
		int		width = m_VideoWidth;
		for( int j = 0; j < height; j++ )
		{
			for( int i = 0; i < width; i++ )
			{
				pTxtBuffer[i] = pBmpBuffer[i];
			}
			pBmpBuffer += width;
			pTxtBuffer += width;
		}
	}
#endif
	if( m_pSink )
		m_pSink->Notify( EC_UPDATE, EventParam1, NULL );
	SwapBuffer( pSample );	// FrontとBackバッファを入れ替える
	return S_OK;
}
//---------------------------------------------------------------------------
//! @brief	  	フロントバッファとバックバッファを入れ替える
//! @param		pSample : サンプル。この中のポインタを変更する
//----------------------------------------------------------------------------
void TBufferRenderer::SwapBuffer( IMediaSample *pSample )
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	if( m_FrontBuffer == 1 )
	{
		SetPointer( pSample, m_Buffer[1] );
		m_FrontBuffer = 0;
	}
	else
	{
		SetPointer( pSample, m_Buffer[0] );
		m_FrontBuffer = 1;
	}
}
//---------------------------------------------------------------------------
//! @brief	  	フロントバッファにメモリを割り当てる
//! @param		size : 割り当てるサイズ
//----------------------------------------------------------------------------
void TBufferRenderer::AllocFrontBuffer( size_t size )
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	BYTE	*buff = NULL;

	FreeFrontBuffer();
	if( m_FrontBuffer == 1 )
	{
		buff = m_Buffer[1] = reinterpret_cast<BYTE*>(CoTaskMemAlloc(size));
		m_IsBufferOwner[1] = true;
	}
	else
	{
		buff = m_Buffer[0] = reinterpret_cast<BYTE*>(CoTaskMemAlloc(size));
		m_IsBufferOwner[0] = true;
	}

	if( buff == NULL )
		throw L"Cannot allocate memory in filter.";
}
//---------------------------------------------------------------------------
//! @brief	  	バックバッファにメモリを割り当てる。
//! @param		size : 割り当てるサイズ
//----------------------------------------------------------------------------
void TBufferRenderer::AllocBackBuffer( size_t size )
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	BYTE	*buff = NULL;

	FreeBackBuffer();
	if( m_FrontBuffer == 1 )
	{
		buff = m_Buffer[0] = reinterpret_cast<BYTE*>(CoTaskMemAlloc(size));
		m_IsBufferOwner[0] = true;
	}
	else
	{
		buff = m_Buffer[1] = reinterpret_cast<BYTE*>(CoTaskMemAlloc(size));
		m_IsBufferOwner[1] = true;
	}

	if( buff == NULL )
		throw L"Cannot allocate memory in filter.";
}
//---------------------------------------------------------------------------
//! @brief	  	フロントバッファに割り当てられているメモリを開放する
//!
//! もし、割り当てられているメモリが、このクラスによって割り当てられたものでない場合は、
//! 解放しない。
//----------------------------------------------------------------------------
void TBufferRenderer::FreeFrontBuffer()
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	if( m_FrontBuffer == 1 )
	{
		if( m_Buffer[1] != NULL )
		{
			if( m_IsBufferOwner[1] )
				CoTaskMemFree( m_Buffer[1] );
			m_Buffer[1] = NULL;
		}
		m_IsBufferOwner[1] = false;
	}
	else
	{
		if( m_Buffer[0] != NULL )
		{
			if( m_IsBufferOwner[0] )
				CoTaskMemFree( m_Buffer[0] );
			m_Buffer[0] = NULL;
		}
		m_IsBufferOwner[0] = false;
	}
}
//---------------------------------------------------------------------------
//! @brief	  	バックバッファに割り当てられているメモリを開放する
//!
//! もし、割り当てられているメモリが、このクラスによって割り当てられたものでない場合は、
//! 解放しない。
//----------------------------------------------------------------------------
void TBufferRenderer::FreeBackBuffer()
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	if( m_FrontBuffer == 1 )
	{
		if( m_Buffer[0] != NULL )
		{
			if( m_IsBufferOwner[0] )
				CoTaskMemFree( m_Buffer[0] );
			m_Buffer[0] = NULL;
		}
		m_IsBufferOwner[0] = false;
	}
	else
	{
		if( m_Buffer[1] != NULL )
		{
			if( m_IsBufferOwner[1] )
				CoTaskMemFree( m_Buffer[1] );
			m_Buffer[1] = NULL;
		}
		m_IsBufferOwner[1] = false;
	}
}
//---------------------------------------------------------------------------
//! @brief	  	フロントバッファにバッファへのポインタを設定する
//! @param		buff : バッファへのポインタ
//----------------------------------------------------------------------------
void TBufferRenderer::SetFrontBuffer( BYTE *buff )
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	FreeFrontBuffer();
	if( m_FrontBuffer == 1 )
		m_Buffer[1] = buff;
	else
		m_Buffer[0] = buff;
}
//---------------------------------------------------------------------------
//! @brief	  	バックバッファにバッファへのポインタを設定する
//! @param		buff : バッファへのポインタ
//----------------------------------------------------------------------------
void TBufferRenderer::SetBackBuffer( BYTE *buff )
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	FreeBackBuffer();
	if( m_FrontBuffer == 1 )
		m_Buffer[0] = buff;
	else
		m_Buffer[1] = buff;
	
	SetPointer( buff );
}
//---------------------------------------------------------------------------
//! @brief	  	フロントバッファへのポインタを取得する
//! @return		バッファへのポインタ
//----------------------------------------------------------------------------
BYTE *TBufferRenderer::GetFrontBuffer()
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	if( m_FrontBuffer == 1 )
		return m_Buffer[1];
	else
		return m_Buffer[0];
}
//---------------------------------------------------------------------------
//! @brief	  	バックバッファへのポインタを取得する
//! @return		バッファへのポインタ
//----------------------------------------------------------------------------
BYTE *TBufferRenderer::GetBackBuffer()
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	if( m_FrontBuffer == 1 )
		return m_Buffer[0];
	else
		return m_Buffer[1];
}
//----------------------------------------------------------------------------
//! @brief	  	フロントバッファを設定します。
//! @param		buff : フロントバッファ用バッファへのポインタ
//! @param		size : バッファのサイズを渡す変数へのポインタ。@n
//!					buffがNULLの時、ここに欲しいサイズが返る
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT TBufferRenderer::SetFrontBuffer( BYTE *buff, long *size )
{
	if( m_State == State_Running )
		return S_FALSE;

	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	if( buff == NULL && size != NULL )
	{
		*size = GetBufferSize();
		return S_OK;
	}
	if( buff == NULL || size == NULL )
		return E_POINTER;

	if( (*size) != GetBufferSize() )
		return E_INVALIDARG;

	FreeFrontBuffer();
	SetFrontBuffer(buff);
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	バックバッファを設定します。
//! @param		buff : バックバッファ用バッファへのポインタ
//! @param		size : バッファのサイズを渡す変数へのポインタ。@n
//!					buffがNULLの時、ここに欲しいサイズが返る
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT TBufferRenderer::SetBackBuffer( BYTE *buff, long *size )
{
	if( m_State == State_Running )
		return S_FALSE;

	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	if( buff == NULL && size != NULL )
	{
		*size = GetBufferSize();
		return S_OK;
	}
	if( buff == NULL || size == NULL )
		return E_POINTER;

	if( (*size) != GetBufferSize() )
		return E_INVALIDARG;

	FreeBackBuffer();
	SetBackBuffer(buff);
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief		フロントバッファへのポインタを取得します。
//! @param		buff : フロントバッファへのポインタを返すためのバッファへのポインタ
//! @param		size : バッファのサイズを返す変数へのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT TBufferRenderer::GetFrontBuffer( BYTE **buff, long *size )
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	*buff = GetFrontBuffer();
	*size = GetBufferSize();
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	バックバッファへのポインタを取得します。
//! @param		buff : バックバッファへのポインタを返すためのバッファへのポインタ
//! @param		size : バッファのサイズを返す変数へのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT TBufferRenderer::GetBackBuffer( BYTE **buff, long *size )
{
	CAutoLock cAutoLock(&m_BufferLock);	// クリティカルセクション
	*buff = GetBackBuffer();
	*size = GetBufferSize();
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	1フレームの平均表示時間を取得します
//! @param		pAvgTimePerFrame : 1フレームの平均表示時間
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT TBufferRenderer::get_AvgTimePerFrame( REFTIME *pAvgTimePerFrame )
{
	if( pAvgTimePerFrame ) {
		*pAvgTimePerFrame = (reinterpret_cast<VIDEOINFOHEADER *>(m_MtIn.Format())->AvgTimePerFrame) / 10000000.0;
		return S_OK;
	}
	else
		return E_POINTER;
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオの幅を取得します
//! @param		pVideoWidth : ビデオの幅
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT TBufferRenderer::get_VideoWidth( long *pVideoWidth )
{
	if( pVideoWidth ) {
		*pVideoWidth = reinterpret_cast<VIDEOINFOHEADER *>(m_MtIn.Format())->bmiHeader.biWidth;
		return S_OK;
	}
	else
		return E_POINTER;
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオの高さを取得します
//! @param		pVideoHeight : ビデオの高さ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT TBufferRenderer::get_VideoHeight( long *pVideoHeight )
{
	if( pVideoHeight ) {
		*pVideoHeight = reinterpret_cast<VIDEOINFOHEADER *>(m_MtIn.Format())->bmiHeader.biHeight;
		return S_OK;
	}
	else
		return E_POINTER;
}
//----------------------------------------------------------------------------
//! @brief	  	ストリーミングが開始された時にコールされる
//!
//! 開始フレームを記録する。
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT TBufferRenderer::OnStartStreaming(void)
{
	HRESULT		hr;
	CComPtr<IMediaSeeking>	mediaSeeking;
	
	if( m_pGraph )
	{
		if( m_pGraph->QueryInterface( &mediaSeeking ) != S_OK )
			mediaSeeking = NULL;
	}

	bool		bGetTime = false;
	LONGLONG	Current = 0;
	if( mediaSeeking.p != NULL )
	{	// IMediaSeekingを使って時間の取得を試みる
		GUID	Format;
		if( SUCCEEDED(hr = mediaSeeking->GetTimeFormat( &Format ) ) )
		{
			if( SUCCEEDED(hr = mediaSeeking->GetCurrentPosition( &Current )) )
			{
				if( IsEqualGUID( TIME_FORMAT_MEDIA_TIME, Format ) )
				{
					double	renderTime = Current / 10000000.0;
					REFTIME	AvgTimePerFrame;	// REFTIME :  秒数を示す小数を表す倍精度浮動小数点数。
					if( SUCCEEDED( hr = get_AvgTimePerFrame( &AvgTimePerFrame ) ) )
					{
						m_StartFrame = (LONG)(renderTime / AvgTimePerFrame + 0.5);
						bGetTime = true;
					}
				}
				else if( IsEqualGUID( TIME_FORMAT_FRAME, Format ) )
				{
					m_StartFrame = (LONG)Current;
					bGetTime = true;
				}
			}
		}
	}
	if( bGetTime == false )
		m_StartFrame = 0;
	
	mediaSeeking.Release();

	return CBaseVideoRenderer::OnStartStreaming();
}
//----------------------------------------------------------------------------
//! @brief	  	レンダリング前にコールされる
//!
//! メディアサンプルにメディアタイムを記録する。
//! メディアタイムは開始フレームに現在のストリーム時間を加算したものになる。
//! もし、フィルタのIMediaSeekingインターフェイスが利用できない場合は、
//! このレンダーフィルタが描画したフレーム数とドロップしたフレーム数を加算する。
//! この場合、より上位のフィルタでドロップしたフレーム数はわからないので、
//! 若干精度が落ちる。
//! @param		pMediaSample : メディアサンプル
//----------------------------------------------------------------------------
void TBufferRenderer::OnRenderStart( IMediaSample *pMediaSample )
{
	CBaseVideoRenderer::OnRenderStart(pMediaSample);

	HRESULT		hr;
	bool		bGetTime = false;
	LONGLONG	Current = 0, Stop = 0;
	IMediaSeeking	*mediaSeeking = NULL;
	if( GetMediaPositionInterface( IID_IMediaSeeking, (void**)&mediaSeeking) == S_OK )
	{
		GUID	Format;
		if( SUCCEEDED(hr = mediaSeeking->GetTimeFormat( &Format ) ) )
		{
			if( SUCCEEDED(hr = mediaSeeking->GetCurrentPosition( &Current )) &&
				SUCCEEDED(hr = mediaSeeking->GetStopPosition( &Stop )) )
			{
				if( IsEqualGUID( TIME_FORMAT_MEDIA_TIME, Format ) )
				{
					double	renderTime = Current / 10000000.0;
					double	stopTime = Stop / 10000000.0;
					REFTIME	AvgTimePerFrame;	// REFTIME :  秒数を示す小数を表す倍精度浮動小数点数。
					if( SUCCEEDED( hr = get_AvgTimePerFrame( &AvgTimePerFrame ) ) )
					{
						Current = (LONGLONG)(renderTime / AvgTimePerFrame + 0.5);
						Stop = (LONGLONG)(stopTime / AvgTimePerFrame + 0.5);
						bGetTime = true;
					}
				}
				else if( IsEqualGUID( TIME_FORMAT_FRAME, Format ) )
				{
					bGetTime = true;
				}
			}
		}
		mediaSeeking->Release();
		mediaSeeking = NULL;
	}
	LONGLONG	TimeStart = m_StartFrame + m_cFramesDrawn + m_cFramesDropped;;
	LONGLONG	TimeEnd = m_StartFrame + m_cFramesDrawn + m_cFramesDropped;;
	if( bGetTime == true )
	{
		TimeStart = m_StartFrame + Current;
		TimeEnd = m_StartFrame + Current;
		m_StopFrame = m_StartFrame + static_cast<LONG>(Stop);
	}
	else
	{
		TimeStart = m_StartFrame + m_cFramesDrawn + m_cFramesDropped;;
		TimeEnd = m_StartFrame + m_cFramesDrawn + m_cFramesDropped;;
		m_StopFrame = 0;
	}
	pMediaSample->SetMediaTime( &TimeStart, &TimeEnd );
}
//----------------------------------------------------------------------------
//##	TBufferRendererInputPin
//----------------------------------------------------------------------------
//! @brief	  	入力ピンオブジェクトを構築します。
//! @param		pRenderer : レンダーオブジェクトを指定します。
//! @param		pInterfaceLock : CCritSec ロックへのポインタで、状態移行を継続するために使用する。@n
//!					これはフィルタ ロック CBaseFilter.m_pLock と同様のクリティカル セクションとなりうる。 
//! @param		phr : メソッドの成功・失敗を示す HRESULT 値を取得する変数のポインタ。
//! @param		name : オブジェクトのデバッグ用の名前が入る文字列。
//----------------------------------------------------------------------------
TBufferRendererInputPin::TBufferRendererInputPin( TBufferRenderer *pRenderer, CCritSec *pInterfaceLock, HRESULT *phr, LPCWSTR name)
 : CRendererInputPin( pRenderer, phr, name ), m_pRenderer( pRenderer ), m_pInterfaceLock( pInterfaceLock ),
 m_ActiveAllocator( false )
{
}
//----------------------------------------------------------------------------
//! @brief	  	デストラクタ。現在は何もしない。
//----------------------------------------------------------------------------
TBufferRendererInputPin::~TBufferRendererInputPin()
{
}
//----------------------------------------------------------------------------
//! @brief	  	自前のアロケータが有効かどうか調べます。
//! @return		有効ならTRUEを返します。
//----------------------------------------------------------------------------
bool TBufferRendererInputPin::ActiveAllocator( void ) const
{
	return m_ActiveAllocator;
}
//----------------------------------------------------------------------------
//! @brief	  	自前のアロケータオブジェクトを割り当てます。
//! @param		ppAllocator : 返すアロケーター
//----------------------------------------------------------------------------
STDMETHODIMP TBufferRendererInputPin::GetAllocator( IMemAllocator **ppAllocator )
{
	CAutoLock cInterfaceLock(m_pInterfaceLock);
	CheckPointer(ppAllocator,E_POINTER);

	// アロケータがまだ設定されていないとき
	if (m_pAllocator == NULL) {
		m_pAllocator = &(m_pRenderer->m_Allocator);
		m_pAllocator->AddRef();
	}
	// 参照カウントを残すのはインタフェースの仕様です。
	m_pAllocator->AddRef();
	*ppAllocator = m_pAllocator;

	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	アロケータが決まったときに呼び出されます。
//! @param		pAllocator 今回の接続で使用するアロケータを指定します。
//! @param		bReadOnly このアロケータからのサンプルが読みとり専用ならTRUEを指定します。
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP TBufferRendererInputPin::NotifyAllocator( IMemAllocator * pAllocator, BOOL bReadOnly )
{
	CAutoLock cInterfaceLock(m_pInterfaceLock);

	// 基底クラス呼び出し
	HRESULT hr = CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
	if( FAILED(hr) )
		return hr;

	//自前のアロケータが有効かどうかを記録します
	m_ActiveAllocator = (pAllocator == (&(m_pRenderer->m_Allocator)));

	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	指定したメディアサンプルにポインタを設定します
//! @param		media : メディアサンプル
//! @param		ptr : 設定するポインタ
//----------------------------------------------------------------------------
void TBufferRendererInputPin::SetPointer( IMediaSample *media, BYTE *ptr )
{
	m_pRenderer->m_Allocator.SetPointer( media, ptr );
}
//----------------------------------------------------------------------------
//! @brief	  	アロケーターが持つメディアサンプルにポインタを設定します
//! @param		ptr : 設定するポインタ
//----------------------------------------------------------------------------
void TBufferRendererInputPin::SetPointer( BYTE *ptr )
{
	m_pRenderer->m_Allocator.SetPointer( ptr );
}
//----------------------------------------------------------------------------
//## TBufferRendererAllocator
//----------------------------------------------------------------------------
//! @brief	  	コンストラクタ
//! @param		pRenderer : レンダーオブジェクトを指定します。
//! @param		pName : オブジェクトのデバッグ用の名前が入る文字列。
//! @param		pUnk : 集成された所有者オブジェクトへのポインタ。
//! @param		phr : メソッドの成功・失敗を示す HRESULT 値を取得する変数のポインタ。
//----------------------------------------------------------------------------
TBufferRendererAllocator::TBufferRendererAllocator( TBufferRenderer *pRenderer, TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr)
 : CBaseAllocator(pName, pUnk, phr), m_pMediaSample(NULL), m_pRenderer(pRenderer)
{}
//----------------------------------------------------------------------------
//! @brief	  	CBaseAllocator::Decommitをコールしなければならないので、コールする。
//----------------------------------------------------------------------------
TBufferRendererAllocator::~TBufferRendererAllocator()
{
	Decommit();
	ASSERT(m_lAllocated == m_lFree.GetCount());
	// Free up all the CMediaSamples
	CMediaSample *pSample;
	for(;;)
	{
		pSample = m_lFree.RemoveHead();
		if( pSample != NULL )
			delete pSample;
		else
			break;
	}
	m_lAllocated = 0;
}
//----------------------------------------------------------------------------
//! @brief	  	何もしない。アロケーターがメモリを解放することはない。
//----------------------------------------------------------------------------
void TBufferRendererAllocator::Free( void )
{
}
//----------------------------------------------------------------------------
//! @brief	  	メモリを割り当てて、それをリストに追加する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT TBufferRendererAllocator::Alloc( void )
{
	CAutoLock	lck(this);

	/* Check he has called SetProperties */
	HRESULT hr = CBaseAllocator::Alloc();
	if( FAILED(hr) )
		return hr;

	/* If the requirements haven't changed then don't reallocate */
	if( hr == S_FALSE )
		return NOERROR;

	ASSERT(hr == S_OK); // we use this fact in the loop below
	LPBYTE	pBuffer = static_cast<LPBYTE>(m_pRenderer->GetBackBuffer());

	CMediaSample *pSample;
	pSample = new CMediaSample( NAME("buffer media sample"), this, &hr, pBuffer, m_lSize );
	ASSERT(SUCCEEDED(hr));
	if( pSample == NULL ) {
		return E_OUTOFMEMORY;
	}
	m_lFree.Add(pSample);
	m_lAllocated++;
	m_pMediaSample = pSample;
	return NOERROR;
}

//----------------------------------------------------------------------------
//! @brief	  	要求するメモリの詳細を設定する。
//! @param		pRequest : バッファ要求を含む ALLOCATOR_PROPERTIES 構造体のポインタ
//! @param		pActual : 実際のバッファ プロパティを受け取る ALLOCATOR_PROPERTIES 構造体のポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP TBufferRendererAllocator::SetProperties( ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual )
{
	CheckPointer( pActual, E_POINTER );
	ValidateReadWritePtr( pActual, sizeof(ALLOCATOR_PROPERTIES) );
	CAutoLock	cObjectLock(this);

    ZeroMemory( pActual, sizeof(ALLOCATOR_PROPERTIES) );

	if( m_bCommitted == TRUE)
		return VFW_E_ALREADY_COMMITTED;

	if( m_lFree.GetCount() < m_lAllocated )	// m_lAllocatedが1個以上の時はエラーにした方がよい？
		return VFW_E_BUFFERS_OUTSTANDING;

	if( pRequest->cBuffers == 1 && pRequest->cbBuffer == m_pRenderer->GetBufferSize() &&
		pRequest->cbAlign == 1 && pRequest->cbPrefix == 0 )
	{
		*pActual = *pRequest;
		m_lSize = pRequest->cbBuffer;
		m_lCount = pRequest->cBuffers;
		m_lAlignment = pRequest->cbAlign;
		m_lPrefix = pRequest->cbPrefix;
		m_bChanged = TRUE;
		return S_OK;
	}

	return VFW_E_BADALIGN;
}
//----------------------------------------------------------------------------
//! @brief	  	指定したメディアサンプルにポインタを設定します
//! @param		media : メディアサンプル
//! @param		ptr : 設定するポインタ
//----------------------------------------------------------------------------
void TBufferRendererAllocator::SetPointer( IMediaSample *media, BYTE *ptr )
{
	BYTE	*pBufferParam, *pBufferOwn;
	if( media )
	{
		media->GetPointer( &pBufferParam);
		if( m_pMediaSample != NULL )
		{
			m_pMediaSample->GetPointer( &pBufferOwn );
			if( pBufferOwn == pBufferParam )	// 同じバッファを指しているので、保持しているサンプルと同じと見なす
			{
				LONG	cBytes = m_pMediaSample->GetSize();	// サイズは変わっていないと見なす、事前にチェックしておくこと
				m_pMediaSample->SetPointer( ptr, cBytes );
			}
		}
	}
}
