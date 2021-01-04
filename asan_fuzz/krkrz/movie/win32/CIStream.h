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

#ifndef __CISTREM_H__
#define __CISTREM_H__

#include <objidl.h>
//#include <streams.h>	// 本来はあった方がいいけど、ワーニングが出るのでコメントアウト。ソースのインクルード順で回避？
#include "asyncio.h"
#include "asyncrdr.h"


//---------------------------------------------------------------------------
// CIStreamProxy : IStream to CAsyncStream proxy
//---------------------------------------------------------------------------
class CIStreamProxy : public CAsyncStream
{
public:
    CIStreamProxy(IStream *refstream, unsigned __int64 size) :
        m_RefStream(refstream),
        m_Size(size)
    {
		m_RefStream->AddRef();
    }

	~CIStreamProxy()
	{
		m_RefStream->Release();
	}

	HRESULT SetPointer(LONGLONG llPos)
    {
		ULARGE_INTEGER result;
		LARGE_INTEGER to;
		to.QuadPart = llPos;
		HRESULT hr = m_RefStream->Seek(to, STREAM_SEEK_SET, &result); 
		if(SUCCEEDED(hr) && llPos != (LONGLONG)result.QuadPart) return E_FAIL;
		return hr;
    }
    
	HRESULT Read(PBYTE pbBuffer,
                 DWORD dwBytesToRead,
                 BOOL bAlign,
                 LPDWORD pdwBytesRead)
    {
        CAutoLock lck(&m_csLock);

		ULONG  bytesread;
		HRESULT hr = m_RefStream->Read(pbBuffer, dwBytesToRead, &bytesread);
		*pdwBytesRead = bytesread;
		return hr;
    }

    LONGLONG Size(LONGLONG *pSizeAvailable)
    {
        *pSizeAvailable = m_Size;
        return m_Size;
    }
    
	DWORD Alignment()
    {
        return 1;
    }
    
	void Lock()
    {
        m_csLock.Lock();
    }
    
	void Unlock()
    {
        m_csLock.Unlock();
    }

private:
    CCritSec		 m_csLock;
	IStream	 *		 m_RefStream;
	unsigned __int64 m_Size;
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// CIStreamReader 
//---------------------------------------------------------------------------
class CIStreamReader : public CAsyncReader
{
public:
    STDMETHODIMP Register()
    {
        return S_OK;
    }
    STDMETHODIMP Unregister()
    {
        return S_OK;
    }
    CIStreamReader(CIStreamProxy *pStream, CMediaType *pmt, HRESULT *phr) :
        CAsyncReader(NAME("IStream Reader"), NULL, pStream, phr)
    {
        m_mt = *pmt;
    }
	LPCWSTR Name() { return m_pName; }
};
//---------------------------------------------------------------------------

#endif	// __CISTREM_H__
