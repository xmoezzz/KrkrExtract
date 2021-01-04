/****************************************************************************/
/*! @file
@brief Demux output pin

デマルチプレクサの出力ピンを実装する
-----------------------------------------------------------------------------
	Copyright (C) 2005 T.Imoto <http://www.kaede-software.com>
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2005/10/22
@note
*****************************************************************************/

#ifndef __DEMUX_OUTPUT_PIN_H__
#define __DEMUX_OUTPUT_PIN_H__

#include "IDemuxReader.h"
#include <source.h>
#include "CMediaSeekingProxy.h"

//----------------------------------------------------------------------------
//! @brief 汎用Demux用出力ピン
//----------------------------------------------------------------------------
class CDemuxOutputPin : public CSourceStream
{
	CMediaSeekingProxy	m_SeekProxy;
	IOutputStream		*m_Stream;
	CCritSec			*m_Lock;

public:
	CDemuxOutputPin( TCHAR *szName, CSource *pFilter, HRESULT *pHr, LPCWSTR pszName, IMediaSeeking *pSeek, IOutputStream *outstream, CCritSec *lock );
	virtual ~CDemuxOutputPin();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv );

	virtual HRESULT GetMediaType( int iPosition, CMediaType *pmt );
	virtual HRESULT CheckMediaType( const CMediaType * pmt );
	virtual HRESULT DecideBufferSize( IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest );
	virtual HRESULT SetMediaType( const CMediaType *pmt );

	// IQualityControl Interface
	virtual STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q) { return S_OK; }

	// CSourceStream
	virtual HRESULT FillBuffer(IMediaSample *pSamp){return S_OK;};
	virtual HRESULT DoBufferProcessingLoop(void);

	HRESULT RetrieveBuffer( IMediaSample **pSample );

	// IPin method
	STDMETHODIMP Connect( IPin *pReceivePin, const AM_MEDIA_TYPE *pmt );

	// CBaseOutputPin method
	virtual HRESULT CompleteConnect( IPin *pReceivePin );
	virtual HRESULT DecideAllocator( IMemInputPin *pPin, IMemAllocator **pAlloc );


	friend class CDemuxSource;
};

#endif	// __DEMUX_OUTPUT_PIN_H__

