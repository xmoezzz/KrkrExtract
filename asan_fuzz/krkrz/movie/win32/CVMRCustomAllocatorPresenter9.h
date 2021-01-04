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

#ifndef __VMR_CUSTOM_ALLOCATOR_PRESENTER_H__
#define __VMR_CUSTOM_ALLOCATOR_PRESENTER_H__

#include <atlbase.h>
#include <streams.h>
#include <d3d9.h>
#include <vmr9.h>
#include <assert.h>
#include <ctlutil.h>
#include <vector>
#include "CDLLLoader.h"

class tTVPDSMixerVideoOverlay;

//----------------------------------------------------------------------------
//! @brief VMR9のレンダーレスモード用アロケータとプレゼンタ
//----------------------------------------------------------------------------
class CVMRCustomAllocatorPresenter9 : public CUnknown, public IVMRSurfaceAllocator9, public IVMRImagePresenter9
{
	HWND		m_ChildWnd;
	static ATOM		m_ChildAtom;

	bool		m_Visible;
	RECT		m_Rect;			//!< 指定されたムービーの表示矩形領域を保持

	CComPtr<IDirect3D9>			m_D3D;
	CComPtr<IDirect3DDevice9>	m_D3DDevice;
	CComPtr<IVMRSurfaceAllocatorNotify9>	m_VMR9SurfAllocNotify;
	std::vector<CComPtr<IDirect3DSurface9> >     m_Surfaces;
	CDLLLoader					m_D3DDll;
	SIZE						m_VideoSize;
	SIZE						m_BackBufferSize;

	DWORD_PTR	m_dwUserID;
	
	tTVPDSMixerVideoOverlay	*m_Owner;
	CCritSec				*m_Lock;
	bool					m_RebuildingWindow;

	CComPtr<IDirect3DTexture9>	m_Texture;
    CComPtr<IDirect3DSurface9>	m_RenderTarget;
    CComPtr<IDirect3DVertexBuffer9> m_VertexBuffer;
	RECT						m_SrcRect;		//!< 実際に描画される領域の大きさを保持、m_ChildRect から割り出せるけど…
	RECT						m_ChildRect;	//!< 実際に描画される矩形領域を保持

	DWORD	m_ThreadID;

	struct VideoVertex
	{
		float		x, y, z, w;
		float		tu, tv;
	};
	VideoVertex		m_Vtx[4];

	IVMRSurfaceAllocatorNotify9 *AllocatorNotify()
	{
		assert( m_VMR9SurfAllocNotify.p );
		return m_VMR9SurfAllocNotify;
	}
	IDirect3D9 *D3D()
	{
		assert( m_D3D.p );
		return m_D3D;
	}
	IDirect3DDevice9 *D3DDevice()
	{
		assert(m_D3DDevice.p);
		return m_D3DDevice;
	}
	tTVPDSMixerVideoOverlay* Owner()
	{
		assert( m_Owner );
		return m_Owner;
	}
public:
	CVMRCustomAllocatorPresenter9( tTVPDSMixerVideoOverlay* owner, CCritSec &lock );
	virtual ~CVMRCustomAllocatorPresenter9();

	// Methods of IUnknown
	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv );

	// methods for IVMRSurfaceAllocator9
	HRESULT STDMETHODCALLTYPE InitializeDevice( DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers );
	HRESULT STDMETHODCALLTYPE TerminateDevice( DWORD_PTR dwID );
	HRESULT STDMETHODCALLTYPE GetSurface( DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9 **lplpSurface );
	HRESULT STDMETHODCALLTYPE AdviseNotify( IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify );

	// methods for IVMRImagePresenter9
	HRESULT STDMETHODCALLTYPE StartPresenting( DWORD_PTR dwUserID );
	HRESULT STDMETHODCALLTYPE StopPresenting( DWORD_PTR dwUserID ) { return S_OK; }
	HRESULT STDMETHODCALLTYPE PresentImage( DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo );

	// window procedure
	static LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
	LRESULT WINAPI Proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	void Initialize();
	void SetRect(RECT *rect);
	void SetVisible(bool b);
	void Reset();
	void PresentVideoImage();

protected:
	HRESULT CreateD3D();
	HRESULT ChangeD3DDevice();
	HRESULT RebuildD3DDevice();
	HRESULT ResizeBackbuffer();

	HRESULT CreateChildWindow();
	void DestroyChildWindow();

	void ReleaseAll();
	HRESULT ReleaseD3D();
	HRESULT ReleaseSurfaces();
	HRESULT PresentHelper( VMR9PresentationInfo *lpPresInfo );
	UINT GetMonitorNumber();

	HRESULT DrawVideoPlane( IDirect3DDevice9* device, IDirect3DTexture9* tex );
	HRESULT WindowSizeChanged( UINT w, UINT h );
	HRESULT DecideD3DPresentParameters( D3DPRESENT_PARAMETERS& d3dpp );
	HRESULT InitializeDirect3DState();
	HRESULT UpdateVertex();
	HRESULT CreateVertexBuffer( int texWidth, int texHeight );

	void CalcChildWindowSize( RECT& childRect );
};


#endif // __VMR_CUSTOM_ALLOCATOR_PRESENTER_H__

