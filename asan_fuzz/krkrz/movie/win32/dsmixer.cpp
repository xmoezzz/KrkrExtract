/****************************************************************************/
/*! @file
@brief VMR9を使うオーバーレイクラス

実行にはDirectX9以降が必要
-----------------------------------------------------------------------------
	Copyright (C) 2005 T.Imoto
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2005/09/25
@note
			2005/09/25	T.Imoto		作成
*****************************************************************************/


#include "tjsCommHead.h"
#include "MsgIntf.h"
#include "SysInitIntf.h"
#include "PluginImpl.h"

//#define _WIN32_WINNT 0x0400
//#define _WIN32_DCOM			// DCOM

#include "dsmixer.h"
#include "CIStream.h"
#include "DShowException.h"
#include "OptionInfo.h"
#include <dvdmedia.h>
#include <windowsx.h>
#include <objbase.h>
#include "CVMRCustomAllocatorPresenter9.h"
#include "TVPVideoOverlay.h"

//----------------------------------------------------------------------------
//! @brief	  	初期化
//----------------------------------------------------------------------------
tTVPDSMixerVideoOverlay::tTVPDSMixerVideoOverlay()
{
	m_AvgTimePerFrame = 0;
	m_OwnerInst = 0;
	m_Width = 0;
	m_Height = 0;
	m_AllocatorPresenter = NULL;
	m_hMessageDrainWnd = NULL;
}
//----------------------------------------------------------------------------
//! @brief	  	インターフェイスを解放する
//----------------------------------------------------------------------------
tTVPDSMixerVideoOverlay::~tTVPDSMixerVideoOverlay()
{
	Shutdown = true;
	ReleaseAll();
	m_hMessageDrainWnd = NULL;
}
//----------------------------------------------------------------------------
//! @brief	  	インターフェイスを解放する
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::ReleaseAll()
{
	if( m_MediaControl.p != NULL )
	{
		m_MediaControl->Stop();
	}

	if( m_AllocatorPresenter != NULL )
	{
//		delete m_AllocatorPresenter;
		m_AllocatorPresenter->Release();
		m_AllocatorPresenter = NULL;
	}

	if( m_VMR9SurfAllocNotify.p )
		m_VMR9SurfAllocNotify.Release();

	if( m_VMR9MixerCtrl.p )
		m_VMR9MixerCtrl.Release();

	if( m_VMR9MixerBmp.p )
		m_VMR9MixerBmp.Release();

//	if( m_VMR9WinLessCtrl.p )
//		m_VMR9WinLessCtrl.Release();

	tTVPDSMovie::ReleaseAll();
}
//----------------------------------------------------------------------------
//! @brief	  	フィルタグラフの構築
//! @param 		callbackwin : メッセージを送信するウィンドウ
//! @param 		stream : 読み込み元ストリーム
//! @param 		streamname : ストリームの名前
//! @param 		type : メディアタイプ(拡張子)
//! @param 		size : メディアサイズ
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::BuildGraph( HWND callbackwin, IStream *stream, const wchar_t * streamname, const wchar_t *type, unsigned __int64 size )
{
	HRESULT			hr;

	//CoInitialize(NULL);

	// detect CMediaType from stream's extension ('type')
	try {
		if( FAILED(hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) )
			ThrowDShowException(L"Failed to call CoInitializeEx.", hr);

		// create IFilterGraph instance
		if( FAILED(hr = m_GraphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC)) )
			ThrowDShowException(L"Failed to create FilterGraph.", hr);

		// Register to ROT
		if(GetShouldRegisterToROT())
		{
			AddToROT(m_dwROTReg);
		}

		{
			CAutoLock Lock(&m_VMRLock);
			OwnerWindow = callbackwin;	// copy window handle for D3D
			m_OwnerInst = GetModuleHandle(NULL);
		}
		m_AllocatorPresenter = new CVMRCustomAllocatorPresenter9(this,m_VMRLock);
		m_AllocatorPresenter->AddRef();
		m_AllocatorPresenter->Initialize();
		if( IsWindowsMediaFile(type) )
		{
			CComPtr<IBaseFilter>	pVMR9;
			AddVMR9Filer( pVMR9 );
			BuildWMVGraph( pVMR9, stream );

			if( FAILED(hr = pVMR9.QueryInterface( &m_VMR9MixerCtrl ) ) )
				ThrowDShowException(L"Failed to query IVMRMixerControl9.", hr);
			if( FAILED(hr = pVMR9.QueryInterface( &m_VMR9MixerBmp ) ) )
				ThrowDShowException(L"Failed to query IVMRMixerBitmap9.", hr);
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
				ThrowDShowException(L"Failed to call IFilterGraph::AddFilter.", hr);
	
			// AddFilterしたのでRelease
			m_Reader->Release();

			if( mt.subtype == MEDIASUBTYPE_Avi || mt.subtype == MEDIASUBTYPE_QTMovie )
			{
				// render output pin
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

				CComPtr<IBaseFilter>	pVMR9;
				AddVMR9Filer( pVMR9 );
	
				CComPtr<IPin>	pRdrPinIn;
				pRdrPinIn = GetInPin(pVMR9, 0);
	
				if( FAILED(hr = GraphBuilder()->ConnectDirect( pDecoderPinOut, pRdrPinIn, NULL )) )
					ThrowDShowException(L"Failed to call GraphBuilder()->ConnectDirect( pDecoderPinOut, pRdrPinIn, NULL ).", hr);
	
				if( FAILED(hr = pVMR9.QueryInterface( &m_VMR9MixerCtrl ) ) )
					ThrowDShowException(L"Failed to query IVMRMixerControl9.", hr);
				if( FAILED(hr = pVMR9.QueryInterface( &m_VMR9MixerBmp ) ) )
					ThrowDShowException(L"Failed to query IVMRMixerBitmap9.", hr);
			}
#ifdef ENABLE_THEORA
			else if( mt.subtype == MEDIASUBTYPE_Ogg )
			{
				CComPtr<IBaseFilter>	pVMR9;
				AddVMR9Filer( pVMR9 );
				BuildTheoraGraph( pVMR9, m_Reader); // may throw an exception
	
				if( FAILED(hr = pVMR9.QueryInterface( &m_VMR9MixerCtrl ) ) )
					ThrowDShowException(L"Failed to query IVMRMixerControl9.", hr);
				if( FAILED(hr = pVMR9.QueryInterface( &m_VMR9MixerBmp ) ) )
					ThrowDShowException(L"Failed to query IVMRMixerBitmap9.", hr);
			}
#endif
			else
			{
				tTVPDSFilterHandlerType* handler = TVPGetDSFilterHandler( mt.subtype );
				if( handler )
				{
					CComPtr<IBaseFilter>	pVMR9;
					AddVMR9Filer( pVMR9 );
					BuildPluginGraph( handler, pVMR9, m_Reader );

					if( FAILED(hr = pVMR9.QueryInterface( &m_VMR9MixerCtrl ) ) )
						ThrowDShowException(L"Failed to query IVMRMixerControl9.", hr);
					if( FAILED(hr = pVMR9.QueryInterface( &m_VMR9MixerBmp ) ) )
						ThrowDShowException(L"Failed to query IVMRMixerBitmap9.", hr);

				}
				else
				{
					CComPtr<IBaseFilter>	pVMR9;
					AddVMR9Filer( pVMR9 );
					BuildMPEGGraph( pVMR9, m_Reader); // may throw an exception
		
					if( FAILED(hr = pVMR9.QueryInterface( &m_VMR9MixerCtrl ) ) )
						ThrowDShowException(L"Failed to query IVMRMixerControl9.", hr);
					if( FAILED(hr = pVMR9.QueryInterface( &m_VMR9MixerBmp ) ) )
						ThrowDShowException(L"Failed to query IVMRMixerBitmap9.", hr);
				}
			}
		}
#if 1
		{	// 平均フレーム表示時間を取得する
			CComPtr<IBaseFilter>	pRender;
			if( FAILED(hr = FindVideoRenderer( &pRender ) ) )
				ThrowDShowException(L"Failed to call FindVideoRenderer( &pRender ).", hr);

			CComPtr<IPin>	pRenderPin;
			pRenderPin = GetInPin(pRender, 0);

			AM_MEDIA_TYPE mt;
			if( FAILED(hr = pRenderPin->ConnectionMediaType(&mt)) )
				ThrowDShowException(L"Failed to call IPin::ConnectionMediaType(pmt).", hr);
			if( mt.formattype == FORMAT_VideoInfo && mt.cbFormat != 0 )
			{
				VIDEOINFOHEADER	*vih = reinterpret_cast<VIDEOINFOHEADER*>(mt.pbFormat);
				m_AvgTimePerFrame = vih->AvgTimePerFrame;
				m_Width = vih->bmiHeader.biWidth;
				m_Height = vih->bmiHeader.biHeight;
			}
			else if( mt.formattype == FORMAT_VideoInfo2 && mt.cbFormat != 0 )
			{
				VIDEOINFOHEADER2 *vih = reinterpret_cast<VIDEOINFOHEADER2*>(mt.pbFormat);
				m_AvgTimePerFrame = vih->AvgTimePerFrame;
				m_Width = vih->bmiHeader.biWidth;
				m_Height = vih->bmiHeader.biHeight;
			}
			FreeMediaType(mt);
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

		if( FAILED(hr = m_GraphBuilder.QueryInterface( &m_BasicAudio )) )
			ThrowDShowException(L"Failed to query IBasicAudio", hr);

		// set notify event
		if(callbackwin)
		{
			if(FAILED(hr = Event()->SetNotifyWindow((OAHWND)callbackwin, WM_GRAPHNOTIFY, (long)(this))))
				ThrowDShowException(L"Failed to set IMediaEventEx::SetNotifyWindow.", hr);
		}
	}
	catch(const wchar_t *msg)
	{
		MakeAPause(true);
		ReleaseAll();
		CoUninitialize();
		TVPThrowExceptionMessage(msg);
	}
	catch(...)
	{
		MakeAPause(true);
		ReleaseAll();
		CoUninitialize();
		throw;
	}

	MakeAPause(false);
	CoUninitialize();
}
//----------------------------------------------------------------------------
//! @brief	  	VMR9フィルタをフィルタグラフへ追加する
//! @param 		pVMR9 : VMR9フィルタ
//----------------------------------------------------------------------------
void tTVPDSMixerVideoOverlay::AddVMR9Filer( CComPtr<IBaseFilter> &pVMR9 )
{
	HRESULT			hr = S_OK;
	if( FAILED(hr = pVMR9.CoCreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC) ) )
		ThrowDShowException(L"Failed to create VMR9 Filter. This component requires DirectX9.", hr);

	if( FAILED(hr = GraphBuilder()->AddFilter( pVMR9, L"Video Mixing Render 9")) )
		ThrowDShowException(L"Failed to call GraphBuilder()->AddFilter( pVMR9, L\"Video Mixing Render 9\").", hr);

	{
		CComPtr<IVMRFilterConfig9>	pConfig;
		if( FAILED(hr = pVMR9.QueryInterface(&pConfig) ) )
			ThrowDShowException(L"Failed to query IVMRFilterConfig9.", hr);

		if( FAILED(hr = pConfig->SetNumberOfStreams(1) ) )
			ThrowDShowException(L"Failed to call IVMRFilterConfig9::SetNumberOfStreams(1).", hr);
		if( FAILED(hr = pConfig->SetRenderingMode(VMR9Mode_Renderless ) ) )
			ThrowDShowException(L"Failed to call IVMRFilterConfig9::SetRenderingMode(VMR9Mode_Renderless).", hr);

		// Negotiate Renderless mode
		if( FAILED(hr = pVMR9.QueryInterface( &m_VMR9SurfAllocNotify ) ) )
			ThrowDShowException(L"Failed to query IVMRSurfaceAllocatorNotify9.", hr);

		CComPtr<IVMRSurfaceAllocator9>	alloc;
		if( FAILED(hr = AllocatorPresenter()->QueryInterface( IID_IVMRSurfaceAllocator9, reinterpret_cast<void**>(&alloc.p) ) ) )
			ThrowDShowException(L"Failed to query IVMRSurfaceAllocator9.", hr);
		if( FAILED(hr = AllocatorNotify()->AdviseSurfaceAllocator( reinterpret_cast<DWORD_PTR>(this), alloc ) ) )
			ThrowDShowException(L"Failed to call IVMRSurfaceAllocatorNotify9::AdviseSurfaceAllocator().", hr);
		if( FAILED(hr = Allocator()->AdviseNotify( AllocatorNotify() ) ) )
			ThrowDShowException(L"Failed to call IVMRSurfaceAllocator9::AdviseNotify().", hr);

		// query monitor config
//		if( FAILED(hr = pVMR9.QueryInterface(&m_VMR9MonitorConfig)) )
//			ThrowDShowException(L"Failed to query IVMRMonitorConfig9.", hr);
	}
}
//----------------------------------------------------------------------------
//! @brief	  	ミキシングするビットマップを設定する
//! @param 		hdc : 設定しているビットマップを保持しているデバイスコンテキスト
//! @param 		dest : 転送先位置
//! @param 		alpha : アルファ値 (0.0 - 1.0で指定)
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::SetMixingBitmap( HDC hdc, RECT *dest, float alpha )
{
	HRESULT			hr;
	VMR9AlphaBitmap	bmpInfo;
	ZeroMemory(&bmpInfo, sizeof(bmpInfo));
	bmpInfo.dwFlags = VMR9AlphaBitmap_hDC | VMR9AlphaBitmap_FilterMode;
	bmpInfo.hdc = hdc;

	bmpInfo.rSrc.left = 0;
	bmpInfo.rSrc.top = 0;
	bmpInfo.rSrc.right = dest->right - dest->left;
	bmpInfo.rSrc.bottom = dest->bottom - dest->top;

	long width;
	long height;
//	GetVideoSize( &width, &height );
	// ビデオサイズではなく、最終出力画像のサイズで位置を計算する
	width = Rect.right - Rect.left;
	height = Rect.bottom - Rect.top;
	// 0割り回避
	if( width <= 0 ) width = 1;
	if( height <= 0 ) height = 1;
	if( dest )
	{
		bmpInfo.rDest.left = (static_cast<float>(dest->left)+0.5f)/static_cast<float>(width);
		bmpInfo.rDest.top = (static_cast<float>(dest->top)+0.5f)/static_cast<float>(height);
		bmpInfo.rDest.right = (static_cast<float>(dest->right)+0.5f)/static_cast<float>(width);
		bmpInfo.rDest.bottom = (static_cast<float>(dest->bottom)+0.5f)/static_cast<float>(height);
	}
	else
	{	// NULLの時は、全体にブレンドするようにする
		bmpInfo.rDest.left = 0.0f;
		bmpInfo.rDest.top = 0.0f;
		bmpInfo.rDest.right = 1.0f;
		bmpInfo.rDest.bottom = 1.0f;
	}
	bmpInfo.fAlpha = alpha;
	bmpInfo.dwFilterMode = MixerPref_PointFiltering;

	if(FAILED(hr = MixerBmp()->SetAlphaBitmap( &bmpInfo )) )
		ThrowDShowException(L"Failed to set IVMRMixerBitmap9::SetAlphaBitmap.", hr);
}
//----------------------------------------------------------------------------
//! @brief	  	ミキシングしているビットマップの設定を解除する
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::ResetMixingBitmap()
{
	HRESULT			hr;
	VMR9AlphaBitmap	bmpInfo;
	if(FAILED(hr = m_VMR9MixerBmp->GetAlphaBitmapParameters(&bmpInfo)) )
		ThrowDShowException(L"Failed to set IVMRMixerBitmap9::GetAlphaBitmapParameters.", hr);

	if( bmpInfo.hdc == NULL )	// 設定されていないのでリターン
		return;

	ZeroMemory(&bmpInfo, sizeof(bmpInfo));

	// 設定せずにこのメソッドをコールすると、ビットマップを解除するという仕様らしい。
	if(FAILED(hr = MixerBmp()->UpdateAlphaBitmapParameters( &bmpInfo )) )
		ThrowDShowException(L"Failed to set IVMRMixerBitmap9::UpdateAlphaBitmapParameters.", hr);
}
//----------------------------------------------------------------------------
//! @brief	  	ミキシングするビデオストリームのアルファ値を設定する
//! @param		a : 設定するアルファ値
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::SetMixingMovieAlpha( float a )
{
	HRESULT			hr;
	if(FAILED(hr = MixerControl()->SetAlpha( 0, a )) )
		ThrowDShowException(L"Failed to set IVMRMixerControl9::SetAlpha.", hr);
}
//----------------------------------------------------------------------------
//! @brief	  	ミキシングするビデオストリームのアルファ値を取得する
//! @param		a : アルファ値を受け取るポインタ
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetMixingMovieAlpha( float *a )
{
	HRESULT			hr;
	if(FAILED(hr = MixerControl()->GetAlpha( 0, a )) )
		ThrowDShowException(L"Failed to set IVMRMixerControl9::GetAlpha.", hr);
}
//----------------------------------------------------------------------------
//! @brief	  	ミキシングするビデオストリームの背景色を設定する
//! @param		col : 設定する背景色
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::SetMixingMovieBGColor( unsigned long col )
{
	HRESULT			hr;
	COLORREF	cr = ChangeEndian32(col<<8);
	if(FAILED(hr = MixerControl()->SetBackgroundClr( cr )) )
		ThrowDShowException(L"Failed to set IVMRMixerControl9::SetBackgroundClr.", hr);
}
//----------------------------------------------------------------------------
//! @brief	  	ミキシングするビデオストリームの背景色を取得する
//! @param		col : 背景色を受け取るポインタ
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetMixingMovieBGColor( unsigned long *col )
{
	HRESULT		hr;
	COLORREF	cr;
	if(FAILED(hr = MixerControl()->GetBackgroundClr( &cr )) )
		ThrowDShowException(L"Failed to set IVMRMixerControl9::GetBackgroundClr.", hr);
	*col = ChangeEndian32(cr);
	*col >>= 8;
}
//----------------------------------------------------------------------------
//! @brief	  	ウィンドウハンドルを設定する
//! @param 		window : ウィンドウハンドル
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::SetWindow(HWND window)
{
	if(Shutdown) return;

	if( OwnerWindow != window )
	{
		CAutoLock Lock(&m_VMRLock);
		OwnerWindow = window;
		AllocatorPresenter()->Reset();
	}
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオの表示矩形を設定する
//! @param 		rect : 表示矩形
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::SetRect(RECT *rect)
{
	if(Shutdown) return;
	Rect = *rect;
	AllocatorPresenter()->SetRect(rect);
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオの表示/非表示を設定する
//! @param 		b : 表示/非表示
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::SetVisible(bool b)
{
	AllocatorPresenter()->SetVisible(b);
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオのサイズを取得する
//! @param 		width : 幅
//! @param 		height : 高さ
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetVideoSize( long *width, long *height )
{
	if( width == NULL || height == NULL )
		TVPThrowExceptionMessage(L"Pointer is NULL.(tTVPDSMixerVideoOverlay::GetVideoSize)");

	*width = m_Width;
	*height = m_Height;
}
//----------------------------------------------------------------------------
//! @brief	  	各フレームの平均表示時間を取得する
//! @param 		pAvgTimePerFrame : 各フレームの平均表示時間
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT __stdcall tTVPDSMixerVideoOverlay::GetAvgTimePerFrame( REFTIME *pAvgTimePerFrame )
{
	*pAvgTimePerFrame = (double)m_AvgTimePerFrame / 10000000.0;
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオ画像を画面へ反映する
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::PresentVideoImage()
{
	AllocatorPresenter()->PresentVideoImage();
}
//----------------------------------------------------------------------------
//! @brief	  	メッセージを送るウィンドウを設定する
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::SetMessageDrainWindow(HWND window)
{
	m_hMessageDrainWnd = window;
}
//----------------------------------------------------------------------------
//! @brief	  	最小値を得る
//----------------------------------------------------------------------------
void tTVPDSMixerVideoOverlay::GetAmpControlRangeMin( float *v, int flag )
{
	if(Shutdown) return;

	VMR9ProcAmpControlRange proc = { sizeof(VMR9ProcAmpControlRange) };
	proc.dwProperty = static_cast<VMR9ProcAmpControlFlags>(flag);

	HRESULT			hr;
	if(FAILED(hr = MixerControl()->GetProcAmpControlRange( 0, &proc )) )
		ThrowDShowException(L"Failed to set IVMRMixerControl9::GetProcAmpControlRange.", hr);

	*v = proc.MinValue;
}
//----------------------------------------------------------------------------
//! @brief	  	最大値を得る
//----------------------------------------------------------------------------
void tTVPDSMixerVideoOverlay::GetAmpControlRangeMax( float *v, int flag )
{
	if(Shutdown) return;

	VMR9ProcAmpControlRange proc = { sizeof(VMR9ProcAmpControlRange) };
	proc.dwProperty = static_cast<VMR9ProcAmpControlFlags>(flag);

	HRESULT			hr;
	if(FAILED(hr = MixerControl()->GetProcAmpControlRange( 0, &proc )) )
		ThrowDShowException(L"Failed to set IVMRMixerControl9::GetProcAmpControlRange.", hr);

	*v = proc.MaxValue;
}
//----------------------------------------------------------------------------
//! @brief	  	デフォルト値を得る
//----------------------------------------------------------------------------
void tTVPDSMixerVideoOverlay::GetAmpControlDefaultValue( float *v, int flag )
{
	if(Shutdown) return;

	VMR9ProcAmpControlRange proc = { sizeof(VMR9ProcAmpControlRange) };
	proc.dwProperty = static_cast<VMR9ProcAmpControlFlags>(flag);

	HRESULT			hr;
	if(FAILED(hr = MixerControl()->GetProcAmpControlRange( 0, &proc )) )
		ThrowDShowException(L"Failed to set IVMRMixerControl9::GetProcAmpControlRange.", hr);

	*v = proc.DefaultValue;
}
//----------------------------------------------------------------------------
//! @brief	  	ステップサイズを得る
//----------------------------------------------------------------------------
void tTVPDSMixerVideoOverlay::GetAmpControlStepSize( float *v, int flag )
{
	if(Shutdown) return;

	VMR9ProcAmpControlRange proc = { sizeof(VMR9ProcAmpControlRange) };
	proc.dwProperty = static_cast<VMR9ProcAmpControlFlags>(flag);

	HRESULT			hr;
	if(FAILED(hr = MixerControl()->GetProcAmpControlRange( 0, &proc )) )
		ThrowDShowException(L"Failed to set IVMRMixerControl9::GetProcAmpControlRange.", hr);

	*v = proc.StepSize;
}
//----------------------------------------------------------------------------
//! @brief	  	値を得る
//----------------------------------------------------------------------------
void tTVPDSMixerVideoOverlay::GetAmpControl( float *v, int flag )
{
	if(Shutdown) return;

	VMR9ProcAmpControl proc = { sizeof(VMR9ProcAmpControl) };
	proc.dwFlags = flag;

	HRESULT			hr;
	if(FAILED(hr = MixerControl()->GetProcAmpControl( 0, &proc )) )
		ThrowDShowException(L"Failed to set IVMRMixerControl9::GetProcAmpControl.", hr);

	switch( flag ) {
		case ProcAmpControl9_Contrast:
			*v = proc.Contrast;
			break;
		case ProcAmpControl9_Brightness:
			*v = proc.Brightness;
			break;
		case ProcAmpControl9_Hue:
			*v = proc.Hue;
			break;
		case ProcAmpControl9_Saturation:
			*v = proc.Saturation;
			break;
	}
}
//----------------------------------------------------------------------------
//! @brief	  	値を設定する
//----------------------------------------------------------------------------
void tTVPDSMixerVideoOverlay::SetAmpControl( float v, int flag )
{
	if(Shutdown) return;

	VMR9ProcAmpControl proc = { sizeof(VMR9ProcAmpControl) };
	HRESULT			hr;
	if(FAILED(hr = MixerControl()->GetProcAmpControl( 0, &proc )) )
		ThrowDShowException(L"Failed to set IVMRMixerControl9::GetProcAmpControl.", hr);

	if( proc.dwFlags & flag ) {
		proc.dwFlags = flag;
		switch( flag ) {
			case ProcAmpControl9_Contrast:
				proc.Contrast = v;
				break;
			case ProcAmpControl9_Brightness:
				proc.Brightness = v;
				break;
			case ProcAmpControl9_Hue:
				proc.Hue = v;
				break;
			case ProcAmpControl9_Saturation:
				proc.Saturation = v;
				break;
		}

		if(FAILED(hr = MixerControl()->SetProcAmpControl( 0, &proc )) )
			ThrowDShowException(L"Failed to set IVMRMixerControl9::SetProcAmpControl.", hr);
	} else {
		ThrowDShowException(L"Not supported parameter. IVMRMixerControl9::SetProcAmpControl.", hr);
	}
}

//----------------------------------------------------------------------------
//! @brief	  	コントラストの幅の最小値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetContrastRangeMin( float *v )
{
	GetAmpControlRangeMin( v, ProcAmpControl9_Contrast );
}
//----------------------------------------------------------------------------
//! @brief	  	コントラストの幅の最大値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetContrastRangeMax( float *v )
{
	GetAmpControlRangeMax( v, ProcAmpControl9_Contrast );
}
//----------------------------------------------------------------------------
//! @brief	  	コントラストのデフォルト値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetContrastDefaultValue( float *v )
{
	GetAmpControlDefaultValue( v, ProcAmpControl9_Contrast );
}
//----------------------------------------------------------------------------
//! @brief	  	コントラストのステップサイズを得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetContrastStepSize( float *v )
{
	GetAmpControlStepSize( v, ProcAmpControl9_Contrast );
}
//----------------------------------------------------------------------------
//! @brief	  	コントラストを得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetContrast( float *v )
{
	GetAmpControl( v, ProcAmpControl9_Contrast );
}
//----------------------------------------------------------------------------
//! @brief	  	コントラストを設定する
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::SetContrast( float v )
{
	SetAmpControl( v, ProcAmpControl9_Contrast );
}
//----------------------------------------------------------------------------
//! @brief	  	輝度の幅の最小値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetBrightnessRangeMin( float *v )
{
	GetAmpControlRangeMin( v, ProcAmpControl9_Brightness );
}
//----------------------------------------------------------------------------
//! @brief	  	輝度の幅の最大値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetBrightnessRangeMax( float *v )
{
	GetAmpControlRangeMax( v, ProcAmpControl9_Brightness );
}
//----------------------------------------------------------------------------
//! @brief	  	輝度のデフォルト値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetBrightnessDefaultValue( float *v )
{
	GetAmpControlDefaultValue( v, ProcAmpControl9_Brightness );
}
//----------------------------------------------------------------------------
//! @brief	  	輝度のステップサイズを得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetBrightnessStepSize( float *v )
{
	GetAmpControlStepSize( v, ProcAmpControl9_Brightness );
}
//----------------------------------------------------------------------------
//! @brief	  	輝度を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetBrightness( float *v )
{
	GetAmpControl( v, ProcAmpControl9_Brightness );
}
//----------------------------------------------------------------------------
//! @brief	  	輝度を設定する
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::SetBrightness( float v )
{
	SetAmpControl( v, ProcAmpControl9_Brightness );
}
//----------------------------------------------------------------------------
//! @brief	  	色相の幅の最小値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetHueRangeMin( float *v )
{
	GetAmpControlRangeMin( v, ProcAmpControl9_Hue );
}
//----------------------------------------------------------------------------
//! @brief	  	色相の幅の最大値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetHueRangeMax( float *v )
{
	GetAmpControlRangeMax( v, ProcAmpControl9_Hue );
}
//----------------------------------------------------------------------------
//! @brief	  	色相のデフォルト値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetHueDefaultValue( float *v )
{
	GetAmpControlDefaultValue( v, ProcAmpControl9_Hue );
}
//----------------------------------------------------------------------------
//! @brief	  	色相のステップサイズを得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetHueStepSize( float *v )
{
	GetAmpControlStepSize( v, ProcAmpControl9_Hue );
}
//----------------------------------------------------------------------------
//! @brief	  	色相を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetHue( float *v )
{
	GetAmpControl( v, ProcAmpControl9_Hue );
}
//----------------------------------------------------------------------------
//! @brief	  	色相を設定する
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::SetHue( float v )
{
	SetAmpControl( v, ProcAmpControl9_Hue );
}
//----------------------------------------------------------------------------
//! @brief	  	彩度の幅の最小値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetSaturationRangeMin( float *v )
{
	GetAmpControlRangeMin( v, ProcAmpControl9_Saturation );
}
//----------------------------------------------------------------------------
//! @brief	  	彩度の幅の最大値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetSaturationRangeMax( float *v )
{
	GetAmpControlRangeMax( v, ProcAmpControl9_Saturation );
}
//----------------------------------------------------------------------------
//! @brief	  	彩度のデフォルト値を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetSaturationDefaultValue( float *v )
{
	GetAmpControlDefaultValue( v, ProcAmpControl9_Saturation );
}
//----------------------------------------------------------------------------
//! @brief	  	彩度のステップサイズを得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetSaturationStepSize( float *v )
{
	GetAmpControlStepSize( v, ProcAmpControl9_Saturation );
}
//----------------------------------------------------------------------------
//! @brief	  	彩度を得る
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::GetSaturation( float *v )
{
	GetAmpControl( v, ProcAmpControl9_Saturation );
}
//----------------------------------------------------------------------------
//! @brief	  	彩度を設定する
//----------------------------------------------------------------------------
void __stdcall tTVPDSMixerVideoOverlay::SetSaturation( float v )
{
	SetAmpControl( v, ProcAmpControl9_Saturation );
}

