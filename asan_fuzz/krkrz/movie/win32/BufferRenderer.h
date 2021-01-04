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

#ifndef __VIDEO_BUFFER_RENDER_H__
#define __VIDEO_BUFFER_RENDER_H__

#include "IBufferRenderer.h"
#include "IRendererBufferAccess.h"
#include "IRendererBufferVideo.h"

#define EC_UPDATE		(EC_USER+1)

class TBufferRendererAllocator;
class TBufferRendererInputPin;
class TBufferRenderer;

//----------------------------------------------------------------------------
//! @brief Buffer Rendererのアロケーター
//----------------------------------------------------------------------------
class TBufferRendererAllocator : public CBaseAllocator
{
private:
	CMediaSample	*m_pMediaSample;	//!< 現在のメディアサンプルへのポインタ
	TBufferRenderer	*m_pRenderer;		//!< このアロケーターを持つレンダーへのポインタ

protected:
	void Free(void);
	HRESULT Alloc(void);

public:
	TBufferRendererAllocator( TBufferRenderer *, TCHAR *, LPUNKNOWN, HRESULT * );
	TBufferRendererAllocator( TCHAR *, LPUNKNOWN, HRESULT * );
	virtual ~TBufferRendererAllocator();

	// オーバーライド
	STDMETHODIMP SetProperties( ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual );

	// ダブルバッファリング用ポインタ差し替えメソッド
	void SetPointer( IMediaSample *media, BYTE *ptr );
	void SetPointer( BYTE *ptr ) { SetPointer(m_pMediaSample,ptr); };
};

//----------------------------------------------------------------------------
//! @brief Buffer Rendererのインプットピン
//----------------------------------------------------------------------------
class TBufferRendererInputPin : public CRendererInputPin
{
private:
	TBufferRenderer	*m_pRenderer;		//!< このピンを持つレンダーへのポインタ
	CCritSec		*m_pInterfaceLock;	//!< ロック用リソース
	bool			m_ActiveAllocator;	//!< このピンを持つレンダーのアロケーターが使われているかどうか

public:
	TBufferRendererInputPin( TBufferRenderer *pRenderer, CCritSec *pInterfaceLock, HRESULT *phr, LPCWSTR name );
	virtual ~TBufferRendererInputPin();
	bool ActiveAllocator( void ) const;

	//オーバーライド
	STDMETHODIMP GetAllocator( IMemAllocator **ppAllocator );
	STDMETHODIMP NotifyAllocator( IMemAllocator * pAllocator, BOOL bReadOnly );

	// ダブルバッファリング用ポインタ差し替えメソッド
	void SetPointer( IMediaSample *media, BYTE *ptr );
	void SetPointer( BYTE *ptr );
};

//----------------------------------------------------------------------------
//! @brief バッファヘレンダリングする
//----------------------------------------------------------------------------
class TBufferRenderer : public CBaseVideoRenderer, public IRendererBufferAccess, public IRendererBufferVideo
{
	friend class TBufferRendererInputPin;
	friend class TBufferRendererAllocator;

private:
	long	m_VideoWidth;		//!< ビデオの幅
	long	m_VideoHeight;		//!< ビデオの高さ
	long	m_VideoPitch;		//!< ビデオのピッチ(1行のバイト数)
	BYTE	*m_Buffer[2];		//!< レンダリングするバッファへのポインタ
	bool	m_IsBufferOwner[2];	//!< バッファがこのクラスに割り当てられたものかどうかを保持
	int		m_FrontBuffer;		//!< 現在のフロントバッファがどちらかを保持
	LONG	m_StartFrame;
	LONG	m_StopFrame;

	TBufferRendererInputPin		m_InputPin;		//!< 入力ピン
	TBufferRendererAllocator	m_Allocator;	//!< メモリの割り当て

	CCritSec		m_BufferLock;	//!< バッファへアクセスする時にロックする
	CMediaType		m_MtIn;			//!< Source connection media type

public:
	static CUnknown * WINAPI CreateInstance( LPUNKNOWN, HRESULT * );
	TBufferRenderer( TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr );
	~TBufferRenderer();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void **ppv );

	HRESULT OnStartStreaming(void);
	void OnRenderStart( IMediaSample *pMediaSample );

// IRendererBufferAccess
	STDMETHOD( SetFrontBuffer )( BYTE *buff, long *size);
	STDMETHOD( SetBackBuffer ) ( BYTE *buff, long *size);
	STDMETHOD( GetFrontBuffer )( BYTE **buff, long *size);
	STDMETHOD( GetBackBuffer ) ( BYTE **buff, long *size);

// IRendererBufferVideo
	virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AvgTimePerFrame( 
			/* [retval][out] */ REFTIME *pAvgTimePerFrame);
	virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_VideoWidth( 
			/* [retval][out] */ long *pVideoWidth);
	virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_VideoHeight( 
			/* [retval][out] */ long *pVideoHeight);

protected:
	// オーバーライド
	// DShowが勝手にコールする
	HRESULT CheckMediaType( const CMediaType *pmt );		// Format acceptable?
	HRESULT DoRenderSample( IMediaSample *pMediaSample );	// New video sample
	HRESULT SetMediaType( const CMediaType *pmt );			// Video format notification
	
	void SwapBuffer( IMediaSample *pSample );

	void AllocFrontBuffer( size_t size );
	void AllocBackBuffer( size_t size );

	void FreeFrontBuffer();
	void FreeBackBuffer();

	bool IsAllocatedFrontBuffer() { return ((m_FrontBuffer==1) ? (m_Buffer[1] != NULL) : (m_Buffer[0] != NULL)); }
	bool IsAllocatedBackBuffer() { return ((m_FrontBuffer==1) ? (m_Buffer[0] != NULL) : (m_Buffer[1] != NULL)); }
	
	long GetBufferSize() { return (m_VideoHeight * m_VideoPitch); }

	void SetFrontBuffer( BYTE *buff );
	void SetBackBuffer( BYTE *buff );
	BYTE *GetFrontBuffer();
	BYTE *GetBackBuffer();

	//! ダブルバッファリング用ポインタ差し替えメソッド
	void SetPointer( IMediaSample *media, BYTE *ptr )
	{
		m_InputPin.SetPointer( media, ptr );
	}
	void SetPointer( BYTE *ptr )
	{
		m_InputPin.SetPointer( ptr );
	}
};


#endif	// __VIDEO_BUFFER_RENDER_H__
