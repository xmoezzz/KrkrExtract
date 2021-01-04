/****************************************************************************/
/*! @file
@brief VMR9のレンダーレスモード用アロケータとプレゼンタ

-----------------------------------------------------------------------------
	Copyright (C) 2005 T.Imoto ( http://www.kaede-software.com/ )
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2005/12/24
@note
*****************************************************************************/

#include "tjsCommHead.h"
#include "MsgIntf.h"
#include "DebugIntf.h"
#include "SysInitIntf.h"
#include "PluginImpl.h"

#include "CVMRCustomAllocatorPresenter9.h"
#include "dsmixer.h"
#include "DShowException.h"
#include "OptionInfo.h"
#include <windows.h>
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>

#define EC_UPDATE		(EC_USER+1)

ATOM CVMRCustomAllocatorPresenter9::m_ChildAtom = 0;

/* IID_IDirect3DTexture9 */
/* {85C31227-3DE5-4f00-9B3A-F11AC38C18B5} */
static const GUID IID_IDirect3DTexture9 = 
{ 0x85c31227, 0x3de5, 0x4f00, { 0x9b, 0x3a, 0xf1, 0x1a, 0xc3, 0x8c, 0x18, 0xb5} };

//----------------------------------------------------------------------------
//! @brief	  	CVMRCustomAllocatorPresenter9 constructor
//! @param		owner : このクラスを保持しているクラス
//! @param		lock : ロックオブジェクト
//----------------------------------------------------------------------------
CVMRCustomAllocatorPresenter9::CVMRCustomAllocatorPresenter9( tTVPDSMixerVideoOverlay* owner, CCritSec &lock )
 : CUnknown(NAME("VMR Custom Allocator Presenter"),NULL), m_ChildWnd(NULL), m_Visible(false)
 , /*m_Surfaces(NULL),*/ m_dwUserID(NULL), m_Owner(owner), m_Lock(&lock), m_RebuildingWindow(false)
  {
	m_Rect.left = 0;
	m_Rect.top = 0;
	m_Rect.right = 0;
	m_Rect.bottom = 0;
	m_VideoSize.cx = 0;
	m_VideoSize.cy = 0;
	m_BackBufferSize.cx = m_BackBufferSize.cy = 0;
	m_Vtx[0].z = m_Vtx[1].z = m_Vtx[2].z = m_Vtx[3].z = 1.0f;
	m_Vtx[0].w = m_Vtx[1].w = m_Vtx[2].w = m_Vtx[3].w = 1.0f;
	m_SrcRect.left = m_SrcRect.top = m_SrcRect.right = m_SrcRect.bottom = 0;
	m_ChildRect.left = m_ChildRect.top = m_ChildRect.right = m_ChildRect.bottom = 0;
}
//----------------------------------------------------------------------------
//! @brief	  	CDemuxSource destructor
//----------------------------------------------------------------------------
CVMRCustomAllocatorPresenter9::~CVMRCustomAllocatorPresenter9()
{
	ReleaseAll();
}
//----------------------------------------------------------------------------
//! @brief	  	初期化処理
//----------------------------------------------------------------------------
void CVMRCustomAllocatorPresenter9::Initialize()
{
	CAutoLock	autoLock(m_Lock);
	HRESULT	hr;
	if( FAILED(hr = CreateChildWindow()) )
		ThrowDShowException(L"Failed to create window.", hr );

	if( FAILED(hr = CreateD3D() ) )
		ThrowDShowException(L"Failed to create device.", hr );
}
//----------------------------------------------------------------------------
//! @brief	  	要求されたインターフェイスを返す
//! @param		riid : インターフェイスのIID
//! @param		ppv : インターフェイスを返すポインターへのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP CVMRCustomAllocatorPresenter9::NonDelegatingQueryInterface( REFIID riid, void ** ppv )
{
	if( riid == IID_IVMRSurfaceAllocator9 ) {
		return GetInterface(static_cast<IVMRSurfaceAllocator9*>(this), ppv);
	} else if( riid == IID_IVMRImagePresenter9 ) {
		return GetInterface(static_cast<IVMRImagePresenter9*>(this), ppv);
	} else {
		return CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}
}
//----------------------------------------------------------------------------
//! @brief	  	インターフェイスを解放する
//----------------------------------------------------------------------------
void CVMRCustomAllocatorPresenter9::ReleaseAll()
{
	CAutoLock Lock(m_Lock);
	if( m_VMR9SurfAllocNotify.p )
		m_VMR9SurfAllocNotify.Release();

	ReleaseSurfaces();
	ReleaseD3D();
	DestroyChildWindow();
}
//----------------------------------------------------------------------------
//! @brief	  	Direct3D デバイスを初期化する ( 実際はサーフェイスを確保する)
//! @param		dwUserID : VMR のこのインスタンスを指定する
//! @param		lpAllocInfo : 初期化引数
//! @param		lpNumBuffers : 入力では、作成するバッファの数を指定する。メソッドが返ると、この引数には実際に割り当てられたバッファの数が格納されている
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CVMRCustomAllocatorPresenter9::InitializeDevice( DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers )
{
	if( lpAllocInfo == NULL || lpNumBuffers == NULL )
		return E_INVALIDARG;

	m_dwUserID = dwUserID;
	HRESULT hr = S_OK;
	CAutoLock Lock(m_Lock);

	m_VideoSize.cx = lpAllocInfo->dwWidth;
	m_VideoSize.cy = lpAllocInfo->dwHeight;

	D3DCAPS9	d3dcaps;
	D3DDevice()->GetDeviceCaps( &d3dcaps );
	if( d3dcaps.TextureCaps & D3DPTEXTURECAPS_POW2 )
	{	// 2の累乗のみ許可するかどうか判定
		// ムービーなので、最低値は64にしておく
		DWORD		dwWidth = 64;
		DWORD		dwHeight = 64;

		while( dwWidth < lpAllocInfo->dwWidth ) dwWidth = dwWidth << 1;
		while( dwHeight < lpAllocInfo->dwHeight ) dwHeight = dwHeight << 1;

		lpAllocInfo->dwWidth = dwWidth;
		lpAllocInfo->dwHeight = dwHeight;

		if( dwWidth > d3dcaps.MaxTextureWidth || dwHeight > d3dcaps.MaxTextureHeight ) {
			TVPAddLog( ttstr("krmovie warning : Video size too large. May be cannot create texture.") );
		}

		TVPAddLog( ttstr("krmovie : Use power of two surface.") );
	}

	// テクスチャとして使えるようにする
	lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;
	ReleaseSurfaces();
	m_Surfaces.resize(*lpNumBuffers);
	hr = AllocatorNotify()->AllocateSurfaceHelper( lpAllocInfo, lpNumBuffers, &m_Surfaces.at(0) );

	if( FAILED(hr) && !(lpAllocInfo->dwFlags & VMR9AllocFlag_3DRenderTarget) )
	{
		// テクスチャ生成失敗
		ReleaseSurfaces();

		// YUV サーフェイスかどうか
		if( lpAllocInfo->Format > '0000' )
		{
			D3DDISPLAYMODE dm; 
			if( FAILED( hr = (D3DDevice()->GetDisplayMode( NULL, &dm )) ) )
				return hr;

			if( D3D_OK != ( hr = D3DDevice()->CreateTexture(lpAllocInfo->dwWidth, lpAllocInfo->dwHeight, 1, D3DUSAGE_RENDERTARGET, dm.Format, 
								D3DPOOL_DEFAULT, &m_Texture.p, NULL) ) )
				return hr;
			TVPAddLog( ttstr("krmovie : Use offscreen and YUV surface.") );
		} else {
			TVPAddLog( ttstr("krmovie : Use offscreen surface.") );
		}
		// テクスチャは止めてオフスクリーンに
		lpAllocInfo->dwFlags &= ~VMR9AllocFlag_TextureSurface;
		lpAllocInfo->dwFlags |= VMR9AllocFlag_OffscreenSurface;
		m_Surfaces.resize(*lpNumBuffers);
		if( FAILED( hr = AllocatorNotify()->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, &m_Surfaces.at(0) ) ) )
			return hr;
	} else {
		TVPAddLog( ttstr("krmovie : Use texture surface.") );
	}

	if( SUCCEEDED(hr) ) {
		if( FAILED(hr = CreateVertexBuffer( lpAllocInfo->dwWidth, lpAllocInfo->dwHeight )) )
			return hr;
	}

	return hr;
}
//----------------------------------------------------------------------------
//! @brief	  	頂点バッファを生成し、初期値を入れる
//! @param		texWidth : テクスチャの幅
//! @param		texHeight : テクスチャの高さ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::CreateVertexBuffer( int texWidth, int texHeight )
{
	// 頂点情報を計算しておく
	HRESULT		hr;

	CAutoLock Lock(m_Lock);
	D3DDISPLAYMODE dm;
	UINT iCurrentMonitor = GetMonitorNumber();
	if( FAILED(hr = D3D()->GetAdapterDisplayMode( iCurrentMonitor, &dm )) )
		return hr;

	float	vtx_l = 0.0f;
	float	vtx_r = 0.0f;
	float	vtx_t = 0.0f;
	float	vtx_b = 0.0f;

	vtx_l = static_cast<float>(m_Rect.left - m_ChildRect.left);
	vtx_t = static_cast<float>(m_Rect.top - m_ChildRect.top);
	vtx_r = vtx_l + static_cast<float>(m_Rect.right-m_Rect.left);
	vtx_b = vtx_t + static_cast<float>(m_Rect.bottom-m_Rect.top);

	float	tex_w = static_cast<float>(texWidth);
	float	tex_h = static_cast<float>(texHeight);
	float	video_w = static_cast<float>(m_VideoSize.cx);
	float	video_h = static_cast<float>(m_VideoSize.cy);
	if( vtx_r == 0.0f || vtx_b == 0.0f ) {
		vtx_r = static_cast<float>(dm.Width);
		vtx_b = static_cast<float>(dm.Height);
	}

	m_Vtx[0].x = vtx_l - 0.5f;	// TL
	m_Vtx[0].y = vtx_t - 0.5f;
	m_Vtx[0].tu = 0.0f;
	m_Vtx[0].tv = 0.0f;

	m_Vtx[1].x = vtx_r - 0.5f;	// TR
	m_Vtx[1].y = vtx_t - 0.5f;
	m_Vtx[1].tu = video_w / tex_w;
	m_Vtx[1].tv = 0.0f;

	m_Vtx[2].x = vtx_r - 0.5f;	// BR
	m_Vtx[2].y = vtx_b - 0.5f;
	m_Vtx[2].tu = video_w / tex_w;
	m_Vtx[2].tv = video_h / tex_h;

	m_Vtx[3].x = vtx_l - 0.5f;	// BL
	m_Vtx[3].y = vtx_b - 0.5f;
	m_Vtx[3].tu = 0.0f;
	m_Vtx[3].tv = video_h / tex_h;

	m_VertexBuffer = NULL;
	if( FAILED( hr = D3DDevice()->CreateVertexBuffer( sizeof(m_Vtx) ,D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW|D3DFVF_TEX1, D3DPOOL_MANAGED, &m_VertexBuffer.p, NULL ) ) )
		return hr;

	void* pData;
	if( FAILED( hr = m_VertexBuffer->Lock( 0, sizeof(pData), &pData, 0 ) ) )
		return hr;

	memcpy( pData, m_Vtx, sizeof(m_Vtx) );

	if( FAILED( hr = m_VertexBuffer->Unlock() ) )
		return hr;

	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	頂点バッファの頂点情報を更新する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::UpdateVertex()
{
	HRESULT	hr;
	CAutoLock Lock(m_Lock);

	// 頂点バッファがまだ確保されていない時はスルー
	if( m_VertexBuffer == NULL )
		return S_OK;

	float	vtx_l = 0.0f;
	float	vtx_r = 0.0f;
	float	vtx_t = 0.0f;
	float	vtx_b = 0.0f;

	vtx_l = static_cast<float>(m_Rect.left - m_ChildRect.left);
	vtx_t = static_cast<float>(m_Rect.top - m_ChildRect.top);
	vtx_r = vtx_l + static_cast<float>(m_Rect.right-m_Rect.left);
	vtx_b = vtx_t + static_cast<float>(m_Rect.bottom-m_Rect.top);

	m_Vtx[0].x = vtx_l - 0.5f;	// TL
	m_Vtx[0].y = vtx_t - 0.5f;

	m_Vtx[1].x = vtx_r - 0.5f;	// TR
	m_Vtx[1].y = vtx_t - 0.5f;

	m_Vtx[2].x = vtx_r - 0.5f;	// BR
	m_Vtx[2].y = vtx_b - 0.5f;

	m_Vtx[3].x = vtx_l - 0.5f;	// BL
	m_Vtx[3].y = vtx_b - 0.5f;

	void* pData;
	if( FAILED( hr = m_VertexBuffer->Lock( 0, sizeof(pData), &pData, 0 ) ) )
		return hr;
	memcpy( pData, m_Vtx, sizeof(m_Vtx) );
	if( FAILED( hr = m_VertexBuffer->Unlock() ) )
		return hr;

	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	Direct3D デバイスを解放する ( 実際はサーフェイスを解放する)
//! @param		dwID : VMR のこのインスタンスを示す ID を指定する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CVMRCustomAllocatorPresenter9::TerminateDevice( DWORD_PTR dwID )
{
	HRESULT hr = S_OK;
//	CAutoLock Lock(m_Lock);
	if( m_dwUserID == dwID ) {
		if( FAILED(hr = ReleaseSurfaces()) )
			return hr;
		return S_OK;
	} else {
		return S_OK;
	}
}
//----------------------------------------------------------------------------
//! @brief	  	Direct3D サーフェイスを取得する
//! @param		dwUserID : VMR のこのインスタンスを示す ID を指定する
//! @param		SurfaceIndex : 取得するサーフェイスのインデックスを指定する
//! @param		SurfaceFlags : サーフェイス フラグを指定する ( 何に使うの？ )
//! @param		lplpSurface : IDirect3DSurface9 インターフェイス ポインタを受け取る変数のアドレス。呼び出し元はインターフェイスを必ず解放すること
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CVMRCustomAllocatorPresenter9::GetSurface( DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9 **lplpSurface )
{
    if( lplpSurface == NULL )
		return E_POINTER;

	if( m_dwUserID == dwUserID ) {
		CAutoLock Lock(m_Lock);
		if( SurfaceIndex < m_Surfaces.size() )
		{
			CAutoLock Lock(m_Lock);
			m_Surfaces[SurfaceIndex].CopyTo( lplpSurface );
			if( *lplpSurface == NULL )
				return E_FAIL;
//			ULONG cnt = (*lplpSurface)->AddRef();
			return S_OK;
		}
		else
		{
			return E_FAIL;
		}
	}

	return E_INVALIDARG;
}
//----------------------------------------------------------------------------
//! @brief	  	VMR から呼び出され、アロケータプレゼンタに通知コールバックのインターフェイス ポインタを提供する
//! @param		lpIVMRSurfAllocNotify : アロケータプレゼンタが通知コールバックを VMR に渡すために使う、IVMRSurfaceAllocatorNotify9 インターフェイスを指定する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CVMRCustomAllocatorPresenter9::AdviseNotify( IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify )
{
	CAutoLock Lock(m_Lock);
	m_VMR9SurfAllocNotify = lpIVMRSurfAllocNotify;
//	HMONITOR hMonitor = D3D()->GetAdapterMonitor( D3DADAPTER_DEFAULT );
	HMONITOR hMonitor = D3D()->GetAdapterMonitor( GetMonitorNumber() );
	HRESULT	hr;
    if( FAILED(hr = AllocatorNotify()->SetD3DDevice( D3DDevice(), hMonitor ) ) )
		return hr;
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	Direct3D関連のものを開放する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::ReleaseD3D()
{
	CAutoLock Lock(m_Lock);
	if( m_VertexBuffer.p )
		m_VertexBuffer.Release();

	if( m_D3DDevice.p )
		m_D3DDevice.Release();

	if( m_D3D.p )
		m_D3D.Release();
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	サーフェイスを開放する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::ReleaseSurfaces()
{
	CAutoLock Lock(m_Lock);

	m_Texture = NULL;

	for( DWORD i = 0; i < m_Surfaces.size(); ++i )
	{
//		m_Surfaces[i].Release();
		m_Surfaces[i] = NULL;
	}

	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオが再生を開始する直前に呼び出す。アロケータプレゼンタは、このメソッドの必要な構成を実行する必要がある
//! @param		dwUserID : アロケータプレゼンタの 1 つのインスタンスが複数の VMR インスタンスで使われる場合に使う、VMR のこのインスタンスを一意に識別するアプリケーションが定義した DWORD_PTR クッキー
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CVMRCustomAllocatorPresenter9::StartPresenting( DWORD_PTR dwUserID )
{
	CAutoLock Lock(m_Lock);
	if( m_D3DDevice.p == NULL )
		return E_FAIL;
    return S_OK;
}

//----------------------------------------------------------------------------
//! @brief	  	ビデオテクスチャをポリゴンに貼り付けて描画する
//! @param		device : Direct3D Device
//! @param		tex : テクスチャ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::DrawVideoPlane( IDirect3DDevice9* device, IDirect3DTexture9* tex )
{
//	device->Clear(0,NULL,D3DCLEAR_TARGET,0,1.0f,0);
	if( SUCCEEDED(device->BeginScene()) )
	{
		struct CAutoEndSceneCall {
			IDirect3DDevice9*	m_Device;
			CAutoEndSceneCall( IDirect3DDevice9* device ) : m_Device(device) {}
			~CAutoEndSceneCall() { m_Device->EndScene(); }
		};
		{
			HRESULT				hr;
			CAutoEndSceneCall	autoEnd(device);
			if( FAILED( hr = device->SetTexture( 0, tex ) ) )
				return hr;
			if( FAILED( hr = device->SetStreamSource(0, m_VertexBuffer.p, 0, sizeof(VideoVertex) ) ) )
				return hr;
			if( FAILED( hr = device->SetFVF( D3DFVF_XYZRHW|D3DFVF_TEX1 ) ) )
				return hr;
			if( FAILED( hr = device->DrawPrimitive( D3DPT_TRIANGLEFAN, 0, 2 ) ) )
				return hr;
//			device->DrawPrimitiveUP( D3DPT_TRIANGLEFAN, 2, reinterpret_cast<void*>(m_Vtx), sizeof(m_Vtx[0]) );
			if( FAILED( hr = device->SetTexture( 0, NULL) ) )
				return hr;
		}

		AllocatorNotify()->NotifyEvent(EC_UPDATE,0,0);
	}
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	このビデオ フレームを表示しなければならないときに呼び出される
//! @param		dwUserID : アロケータプレゼンタの 1 つのインスタンスが複数の VMR インスタンスで使われる場合に使う、VMR のこのインスタンスを一意に識別するアプリケーションが定義した DWORD_PTR クッキー
//! @param		lpPresInfo : ビデオ フレームに関する情報を格納する VMR9PresentationInfo 構造体を指定する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CVMRCustomAllocatorPresenter9::PresentImage( DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo )
{
	if( (lpPresInfo == NULL) || (lpPresInfo->lpSurf == NULL) )
		return E_POINTER;

	HRESULT hr = S_OK;
	if( m_dwUserID == dwUserID ) {
		CAutoLock Lock(m_Lock);
//		AllocatorNotify()->NotifyEvent(EC_UPDATE,0,0);
		if( m_RebuildingWindow ) return S_OK;	// フルスクリーン切り替え中は描画しない
		hr = PresentHelper( lpPresInfo );
		if( hr == D3DERR_DEVICELOST)
		{
			TVPAddLog( ttstr("krmovie warning : Device lost.") );
			hr = D3DDevice()->TestCooperativeLevel();
			if( hr == D3DERR_DEVICENOTRESET )
			{	// リセット可能
				ReleaseSurfaces();
				RebuildD3DDevice();
//				if( FAILED(hr = RebuildD3DDevice()) )
//					AllocatorNotify()->NotifyEvent(EC_ERRORABORT,hr,0);
				// 失敗しても通知しない。
				// どうやらフルスクリーン切り替え時などに何度か失敗することがあるようだ
			}
		}
	}
	return hr;
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオ フレームを描画する
//! @param		lpPresInfo : ビデオ フレームに関する情報を格納する VMR9PresentationInfo 構造体を指定する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::PresentHelper( VMR9PresentationInfo *lpPresInfo )
{
	HRESULT hr;
	CAutoLock Lock(m_Lock);
	CComPtr<IDirect3DDevice9> device;
	if( FAILED(hr = lpPresInfo->lpSurf->GetDevice(&device.p )) )
		return hr;

	if( FAILED(hr = device->SetRenderTarget( 0, m_RenderTarget ) ) )
		return hr;
	if( m_Texture != NULL )
	{
		CComPtr<IDirect3DSurface9> pSurf;
		if( SUCCEEDED(hr = m_Texture->GetSurfaceLevel(0, &pSurf)) ) {
			if( FAILED(hr = device->StretchRect( lpPresInfo->lpSurf, NULL, pSurf, NULL, D3DTEXF_NONE )) ) {
				return hr;
			}
		} else {
			return hr;
		}
		if( FAILED(hr = DrawVideoPlane( device, m_Texture.p ) ) )
			return hr;
	} else {
		CComPtr<IDirect3DTexture9> texture;
		if( FAILED(hr = lpPresInfo->lpSurf->GetContainer( IID_IDirect3DTexture9, (LPVOID*)&texture.p ) ) )
			return hr;
		if( FAILED(hr = DrawVideoPlane( device, texture.p ) ) )
			return hr;
	}

	return hr;
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオ フレームを画面に反映する
//----------------------------------------------------------------------------
void CVMRCustomAllocatorPresenter9::PresentVideoImage()
{
	CAutoLock Lock(m_Lock);
	if( m_D3DDevice.p ) {
		HRESULT hr = S_OK;
		hr = m_D3DDevice->Present( &m_SrcRect, NULL, m_ChildWnd, NULL );
//		hr = m_D3DDevice->Present( NULL, NULL, m_ChildWnd, NULL );
		if( hr == D3DERR_DEVICELOST)
		{
			hr = D3DDevice()->TestCooperativeLevel();
			if( hr == D3DERR_DEVICENOTRESET )
			{	// リセット可能
				ReleaseSurfaces();
				RebuildD3DDevice();
			}
		}
	}
}
//----------------------------------------------------------------------------
//! @brief	  	ウィンドウのあるモニタの序数を取得する
//! @return		ウィンドウのあるモニタの序数
//----------------------------------------------------------------------------
UINT CVMRCustomAllocatorPresenter9::GetMonitorNumber()
{
	CAutoLock Lock(m_Lock);
	if( m_D3D.p == NULL || m_ChildWnd == NULL ) return D3DADAPTER_DEFAULT;
	HMONITOR windowMonitor = MonitorFromWindow( m_ChildWnd, MONITOR_DEFAULTTOPRIMARY );
	UINT iCurrentMonitor = 0;
	UINT numOfMonitor = D3D()->GetAdapterCount();
	for( ; iCurrentMonitor < numOfMonitor; ++iCurrentMonitor )
	{
		if( D3D()->GetAdapterMonitor(iCurrentMonitor) == windowMonitor )
			break;
	}
	if( iCurrentMonitor == numOfMonitor )
		iCurrentMonitor = D3DADAPTER_DEFAULT;
	return iCurrentMonitor;
}

//----------------------------------------------------------------------------
//! @brief	  	プレゼント パラメータを決定する
//! @param		d3dpp : D3DPRESENT_PARAMETERS
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::DecideD3DPresentParameters( D3DPRESENT_PARAMETERS& d3dpp )
{
	HRESULT			hr;
	D3DDISPLAYMODE	dm;
	UINT iCurrentMonitor = GetMonitorNumber();
	if( FAILED( hr = D3D()->GetAdapterDisplayMode( iCurrentMonitor, &dm ) ) )
		return hr;

	UINT	width = 0;
	UINT	height = 0;
	LONG_PTR	ownerStyle = ::GetWindowLongPtr( Owner()->OwnerWindow, GWL_STYLE );
	if( !(ownerStyle&WS_THICKFRAME) ) {
		// オーナーはサイズ変更不可
		RECT	clientRect;
		if( ::GetClientRect( Owner()->OwnerWindow, &clientRect ) ) {
			width = clientRect.right - clientRect.left;
			height = clientRect.bottom - clientRect.top;
			// 初回は、サイズ0のウィンドウハンドルが渡されるので、ここは0になる。
			// その時は、ウィンドウのクライアント領域と同じサイズのバックバッファが作られることになる。
			// ただし、直後に正しいウィンドウハンドルが渡されて、期待した大きさのバックバッファになるはず。
		}
	} else {
//	if( width == 0 || height == 0 ) {
		// サイズ変更可の時は、画面サイズと同じ大きさのバックバッファサイズにしてしまう
		width = dm.Width;
		height = dm.Height;
	}

	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	d3dpp.BackBufferFormat = dm.Format;
	d3dpp.BackBufferHeight = height;
	d3dpp.BackBufferWidth = width;
	d3dpp.hDeviceWindow = m_ChildWnd;

	m_BackBufferSize.cx = width;
	m_BackBufferSize.cy = height;

	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	Direct3Dを初期化する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::CreateD3D()
{
	HRESULT	hr;
	CAutoLock Lock(m_Lock);
	ReleaseSurfaces();
	ReleaseD3D();

	if( m_D3DDll.IsLoaded() == false )
#if _DEBUG
		m_D3DDll.Load(_T("d3d9d.dll"));
#else
		m_D3DDll.Load(_T("d3d9.dll"));
#endif
#if _DEBUG
	// load non debug version
	if( m_D3DDll.IsLoaded() == false )
		m_D3DDll.Load(_T("d3d9.dll"));
#endif
	if( m_D3DDll.IsLoaded() == false )
		return m_D3DDll.GetLastError();

	typedef IDirect3D9 *(WINAPI *FuncDirect3DCreate9)( UINT SDKVersion );
	FuncDirect3DCreate9 pDirect3DCreate9 = (FuncDirect3DCreate9)m_D3DDll.GetProcAddress("Direct3DCreate9");

	if( pDirect3DCreate9 == NULL )
		return E_FAIL;

	if( NULL == ( m_D3D = pDirect3DCreate9( D3D_SDK_VERSION ) ) )
		return E_FAIL;

	D3DPRESENT_PARAMETERS	d3dpp;
	if( FAILED( hr = DecideD3DPresentParameters( d3dpp ) ) )
		return hr;

	UINT iCurrentMonitor = GetMonitorNumber();
	DWORD	BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED;
	if( D3D_OK != ( hr = D3D()->CreateDevice( iCurrentMonitor, D3DDEVTYPE_HAL, NULL, BehaviorFlags, &d3dpp, &m_D3DDevice.p ) ) )
		return hr;

	m_ThreadID = ::GetCurrentThreadId();

	m_RenderTarget = NULL;
	if( FAILED( hr = D3DDevice()->GetRenderTarget( 0, &m_RenderTarget.p ) ) )
		return hr;

	if( FAILED( hr = InitializeDirect3DState() ) )
		return hr;

	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	Direct3Dのステートを初期化する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::InitializeDirect3DState()
{
	HRESULT	hr;
	D3DCAPS9	d3dcaps;
	if( FAILED( hr = D3DDevice()->GetDeviceCaps( &d3dcaps ) ) )
		return hr;

	if( d3dcaps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR ) {
		if( FAILED( hr = D3DDevice()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR ) ) )
			return hr;
	} else {
		if( FAILED( hr = D3DDevice()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT ) ) )
			return hr;
	}

	if( d3dcaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR ) {
		if( FAILED( hr = D3DDevice()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR ) ) )
			return hr;
	} else {
		if( FAILED( hr = D3DDevice()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT ) ) )
		return hr;
	}

	if( FAILED( hr = D3DDevice()->SetSamplerState( 0, D3DSAMP_ADDRESSU,  D3DTADDRESS_CLAMP ) ) )
		return hr;
	if( FAILED( hr = D3DDevice()->SetSamplerState( 0, D3DSAMP_ADDRESSV,  D3DTADDRESS_CLAMP ) ) )
		return hr;
	if( FAILED( hr = D3DDevice()->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) ) )
		return hr;
	if( FAILED( hr = D3DDevice()->SetRenderState( D3DRS_LIGHTING, FALSE ) ) )
		return hr;
	if( FAILED( hr = D3DDevice()->SetRenderState( D3DRS_ZENABLE, FALSE ) ) )
		return hr;
	if( FAILED( hr = D3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE ) ) )
		return hr;
	if( FAILED( hr = D3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 ) ) )
		return hr;
	if( FAILED( hr = D3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE ) ) )
		return hr;
	if( FAILED( hr = D3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE ) ) )
		return hr;

	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	Direct3Dデバイスの変更を通知する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::ChangeD3DDevice()
{
	HRESULT	hr;
	CAutoLock Lock(m_Lock);
	D3DDEVICE_CREATION_PARAMETERS	Parameters;
	if( FAILED( hr = D3DDevice()->GetCreationParameters(  &Parameters ) ) )
		return hr;

	CComPtr<IDirect3D9>			pD3D;
	if( FAILED( hr = D3DDevice()->GetDirect3D(&pD3D) ) )
		return hr;

	HMONITOR hMonitor = pD3D->GetAdapterMonitor(Parameters.AdapterOrdinal);
	if( FAILED( hr = AllocatorNotify()->ChangeD3DDevice( D3DDevice(), hMonitor ) ) )
		return hr;
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	Direct3Dデバイスを再構築する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::RebuildD3DDevice()
{
	HRESULT	hr;
	CAutoLock Lock(m_Lock);
	if( FAILED(hr = CreateD3D()) )
		return hr;
	if( FAILED(hr = ChangeD3DDevice() ) )
		return hr;
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	バックバッファのサイズを変更する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::ResizeBackbuffer()
{
	if( m_D3DDevice == NULL ) return E_FAIL;

	CAutoLock Lock(m_Lock);
	if( m_ThreadID != ::GetCurrentThreadId() ) {
		TVPAddLog( ttstr("krmovie warning :  may be cannot reset.") );
	}

	HRESULT	hr = D3DDevice()->TestCooperativeLevel();
	if( hr == D3DERR_DEVICENOTRESET ) {

		D3DPRESENT_PARAMETERS	d3dpp;
		if( FAILED( hr = DecideD3DPresentParameters( d3dpp ) ) )
			return hr;

		ReleaseSurfaces();
		hr = D3DDevice()->Reset(&d3dpp);
		if( hr == D3DERR_DEVICELOST ) {
			TVPAddLog( ttstr("krmovie error : Device lost. Cannot reset device.") );
		} else if( hr == D3DERR_DRIVERINTERNALERROR ) {
			TVPAddLog( ttstr("krmovie error : Device internal fatal error.") );
			AllocatorNotify()->NotifyEvent(EC_ERRORABORT,hr,0);
		} else if( hr == D3DERR_INVALIDCALL ) {
			TVPAddLog( ttstr("krmovie error : Invalid call.") );
		} else if( hr  == D3DERR_OUTOFVIDEOMEMORY ) {
			TVPAddLog( ttstr("krmovie error : Cannot allocate video memory.") );
		} else if( hr == E_OUTOFMEMORY  ) {
			TVPAddLog( ttstr("krmovie error : Cannot allocate memory.") );
		}
	} else {
		hr = E_FAIL;
	}

	return hr;
}
//----------------------------------------------------------------------------
//! @brief	  	子ウィンドウを生成する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CVMRCustomAllocatorPresenter9::CreateChildWindow()
{
	if( m_ChildWnd != NULL ) return S_OK;
	if( Owner()->OwnerWindow == NULL ) return E_FAIL;

	CAutoLock Lock(m_Lock);
	if( m_ChildAtom == 0 )
	{
		WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_PARENTDC | CS_VREDRAW | CS_HREDRAW, (WNDPROC)CVMRCustomAllocatorPresenter9::WndProc, 0L, 0L, Owner()->m_OwnerInst, NULL, NULL, NULL, NULL, _T("krmovie VMR9 Child Window Class"), NULL };
		m_ChildAtom = RegisterClassEx(&wcex);
		if( m_ChildAtom == 0 )
			return HRESULT_FROM_WIN32(GetLastError());
	}
	DWORD	atom = (DWORD)(0xFFFF & m_ChildAtom);
	if( (m_Rect.right - m_Rect.left) != 0 && (m_Rect.bottom - m_Rect.top) != 0 ) {
		RECT clientRect;
		CalcChildWindowSize( clientRect );
		m_ChildWnd = CreateWindow( _T("krmovie VMR9 Child Window Class"), _T("VMR9 child"), WS_CHILDWINDOW, clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, Owner()->OwnerWindow, NULL, Owner()->m_OwnerInst, NULL );
	} else {
		m_ChildWnd = CreateWindow( _T("krmovie VMR9 Child Window Class"), _T("VMR9 child"), WS_CHILDWINDOW, 0, 0, 320, 240, Owner()->OwnerWindow, NULL, Owner()->m_OwnerInst, NULL );
	}
	if( m_ChildWnd == NULL )
		return HRESULT_FROM_WIN32(GetLastError());

	::SetWindowLongPtr(m_ChildWnd,GWLP_USERDATA,(LONG_PTR)this);
	ShowWindow(m_ChildWnd,SW_SHOWDEFAULT);
	if( UpdateWindow(m_ChildWnd) == 0 )
		return HRESULT_FROM_WIN32(GetLastError());
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	子ウィンドウの位置を計算する
//!
//! 親ウィンドウのサイズを超えないような子ウィンドウの位置と大きさを求める
//! @param		childRect : 子ウィンドウの領域
//----------------------------------------------------------------------------
void CVMRCustomAllocatorPresenter9::CalcChildWindowSize( RECT& childRect )
{
	childRect = m_Rect;
	RECT	ownerRect;
	if( ::GetClientRect( Owner()->OwnerWindow, &ownerRect ) ) {
		int width = ownerRect.right - ownerRect.left;
		int height = ownerRect.bottom - ownerRect.top;

		if( (childRect.right - childRect.left) > width ) {
			if( childRect.left < 0 ) {
				childRect.left = 0;
			}
			if( (childRect.right - childRect.left) > width ) {
				childRect.right = childRect.left + width;
			}
		}
		if( (childRect.bottom - childRect.top) > height ) {
			if( childRect.top < 0 ) {
				childRect.top = 0;
			}
			if( (childRect.bottom - childRect.top) > height ) {
				childRect.bottom = childRect.top + height;
			}
		}
	}
	m_ChildRect = childRect;
}
//----------------------------------------------------------------------------
//! @brief	  	子ウィンドウを破棄する
//----------------------------------------------------------------------------
void CVMRCustomAllocatorPresenter9::DestroyChildWindow()
{
	CAutoLock Lock(m_Lock);
	if( m_ChildWnd != NULL )
	{
		::SetWindowLongPtr(m_ChildWnd,GWLP_USERDATA,0);
		DestroyWindow(m_ChildWnd);
	}
	m_ChildWnd = NULL;

}
//----------------------------------------------------------------------------
//! @brief	  	ウィンドウプロシージャ
//! @param		hWnd : ウィンドウハンドル
//! @param		msg : メッセージID
//! @param		wParam : パラメタ
//! @param		lParam : パラメタ
//! @return		エラーコード
//----------------------------------------------------------------------------
LRESULT WINAPI CVMRCustomAllocatorPresenter9::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CVMRCustomAllocatorPresenter9	*win = reinterpret_cast<CVMRCustomAllocatorPresenter9*>(::GetWindowLongPtr(hWnd,GWLP_USERDATA));
	if( win != NULL )
	{
		return win->Proc( hWnd, msg, wParam, lParam );
	}
	return DefWindowProc(hWnd,msg,wParam,lParam);
}
//----------------------------------------------------------------------------
//! @brief	  	ウィンドウプロシージャ
//! @param		hWnd : ウィンドウハンドル
//! @param		msg : メッセージID
//! @param		wParam : パラメタ
//! @param		lParam : パラメタ
//! @return		エラーコード
//----------------------------------------------------------------------------
LRESULT WINAPI CVMRCustomAllocatorPresenter9::Proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	if( msg == WM_PAINT ) {
		PAINTSTRUCT ps;
		HDC			hDC;
		hDC = BeginPaint(hWnd, &ps);
//		HGDIOBJ hBrush = ::GetStockObject(BLACK_BRUSH);
//		FillRect( hDC, &ps.rcPaint, (HBRUSH)hBrush );
		EndPaint(hWnd, &ps);
		return 0;
	} else if( msg == WM_DESTROY ) {
		ReleaseSurfaces();
		ReleaseD3D();
	} else if( msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST && Owner()->GetMessageDrainWindow() ) {
		return ::SendMessage( Owner()->GetMessageDrainWindow(), msg, wParam, lParam );
//		::PostMessage( Owner()->GetMessageDrainWindow(), msg, wParam, lParam );
//		return 0;
	}
	return DefWindowProc(hWnd,msg,wParam,lParam);
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオのサイズを設定する
//! @param		rect : 要求するサイズ
//----------------------------------------------------------------------------
void CVMRCustomAllocatorPresenter9::SetRect( RECT *rect )
{
	m_Rect = *rect;
	if( m_ChildWnd != NULL )
	{
//		int		width = m_Rect.right - m_Rect.left;
//		int		height = m_Rect.bottom - m_Rect.top;
		RECT clientRect;
		CalcChildWindowSize( clientRect );
		int		width = clientRect.right - clientRect.left;
		int		height = clientRect.bottom - clientRect.top;

		if( width > m_BackBufferSize.cx || height > m_BackBufferSize.cy ) {
			Reset();
		} else {
			if( MoveWindow( m_ChildWnd, clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, TRUE ) == 0 )
				ThrowDShowException(L"Failed to call MoveWindow.", HRESULT_FROM_WIN32(GetLastError()));

			m_SrcRect.left = m_SrcRect.top = 0;
			m_SrcRect.right = width;
			m_SrcRect.bottom = height;
			HRESULT	hr;
			if( FAILED(hr = UpdateVertex()) )
				ThrowDShowException(L"Failed to Update Vertex.", hr );
		}
	}
}
//----------------------------------------------------------------------------
//! @brief	  	ビデオの表示/非表示を設定する
//! @param		b : 表示/非表示
//----------------------------------------------------------------------------
void CVMRCustomAllocatorPresenter9::SetVisible( bool b )
{
	m_Visible = b;
	if( m_ChildWnd != NULL )
	{
		if( b == false )
			ShowWindow( m_ChildWnd, SW_HIDE );
		else
		{
			ShowWindow( m_ChildWnd, SW_SHOW );
			if( UpdateWindow( m_ChildWnd ) == 0 )
				ThrowDShowException(L"Failed to call ShowWindow.", HRESULT_FROM_WIN32(GetLastError()));
		}

		RECT clientRect;
		CalcChildWindowSize( clientRect );
		if( MoveWindow( m_ChildWnd, clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, TRUE ) == 0 )
			ThrowDShowException(L"Failed to call MoveWindow.", HRESULT_FROM_WIN32(GetLastError()));
	}
}
//----------------------------------------------------------------------------
//! @brief	  	Direct3D Deviceをリセットする
//----------------------------------------------------------------------------
void CVMRCustomAllocatorPresenter9::Reset()
{
	HRESULT	hr;
	if( Owner()->OwnerWindow != NULL )
	{
		CAutoLock Lock(m_Lock);
//		ReleaseSurfaces();
		DestroyChildWindow();
		if( FAILED(hr = CreateChildWindow() ) )
			ThrowDShowException(L"Failed to create window.", hr );

		if( FAILED(hr = ResizeBackbuffer()) )
		{
			if( FAILED(hr = RebuildD3DDevice()) )
				ThrowDShowException(L"Failed to rebuild Device.", hr );
		}
		m_RebuildingWindow = false;
	} else {
		m_RebuildingWindow = true;
	}
}

