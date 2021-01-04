/****************************************************************************/
/*! @file
@brief IMediaSeeking Proxy

IMediaSeekingの呼び出しを代理する。
ピンがソースフィルタのIMediaSeekingを使用したい場合などに使う
-----------------------------------------------------------------------------
	Copyright (C) 2005 T.Imoto <http://www.kaede-software.com>
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2005/10/22
@note
*****************************************************************************/


#ifndef __MEDIA_SEEKING_PROXY_H__
#define __MEDIA_SEEKING_PROXY_H__

#include <streams.h>
#include <assert.h>
#include <atlcomcli.h>

//! IMediaSeekingへの操作を単にアップストリームへ渡して処理する場合は、CPosPassThruを使用するが、
//! ソースフィルタのピンでIMediaSeekingを実装する必要がある場合、ソースフィルタのIMediaSeeking
//! インターフェイスを呼び出す事になることが多いと思われる。
//! CMediaSeekingProxyはこのような用途に使うためのクラス。
class CMediaSeekingProxy : public IMediaSeeking
{
private:
//	CComPtr<IMediaSeeking>	m_Delegate;
	IMediaSeeking*			m_Delegate;	// CComPtr で持つと自身を持つことになって開放できなくなることがある
	//----------------------------------------------------------------------------
	//! @brief	  	IMediaSeekingを取得する
	//! @return		IMediaSeekingインターフェイス
	//----------------------------------------------------------------------------
	IMediaSeeking *MediaSeeking()
	{
		assert( m_Delegate );
		return m_Delegate;
	}

	// delegateを設定しない生成やコピーを禁止する
	CMediaSeekingProxy();
	CMediaSeekingProxy(const CMediaSeekingProxy& copy);
	void operator =(const CMediaSeekingProxy& lhs);

public:
	CMediaSeekingProxy(IMediaSeeking* delegate);
	virtual ~CMediaSeekingProxy(void);
	void SetSeeker( IMediaSeeking *seeker );

	//Methods of IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, LPVOID *ppvObj );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	//IMediaSeeking Interface
	virtual STDMETHODIMP GetCapabilities(DWORD *pCapabilities);
	virtual STDMETHODIMP CheckCapabilities(DWORD *pCapabilities);
	virtual STDMETHODIMP IsFormatSupported(const GUID *pFormat);
	virtual STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
	virtual STDMETHODIMP SetTimeFormat(const GUID *pFormat);
	virtual STDMETHODIMP GetTimeFormat( GUID *pFormat);
	virtual STDMETHODIMP GetDuration(LONGLONG *pDuration);
	virtual STDMETHODIMP GetStopPosition(LONGLONG *pStop);
	virtual STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
	virtual STDMETHODIMP ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat);
	virtual STDMETHODIMP SetPositions(LONGLONG *pCurrent,DWORD dwCurrentFlags,LONGLONG *pStop,DWORD dwStopFlags);
	virtual STDMETHODIMP GetPositions(LONGLONG *pCurrent, LONGLONG *pStop);
	virtual STDMETHODIMP GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest);
	virtual STDMETHODIMP SetRate(double dRate);
	virtual STDMETHODIMP GetRate(double *dRate);
	virtual STDMETHODIMP GetPreroll(LONGLONG *pllPreroll);
	virtual STDMETHODIMP IsUsingTimeFormat(const GUID *pFormat);
};

#endif	// __MEDIA_SEEKING_PROXY_H__
