/****************************************************************************/
/*! @file
@brief Demux source filter

デマルチプレクサソースフィルタを実装する
-----------------------------------------------------------------------------
	Copyright (C) 2005 T.Imoto <http://www.kaede-software.com>
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2005/10/22
@note
*****************************************************************************/

#ifndef __DEMUX_SOURCE_H__
#define __DEMUX_SOURCE_H__

#include <tchar.h>
#include "IDemuxReader.h"
#include <source.h>
#include <ctlutil.h>

//----------------------------------------------------------------------------
//! @brief 汎用Demuxフィルタ
//----------------------------------------------------------------------------
class CDemuxSource : public CSource, public IAMFilterMiscFlags, public IMediaSeeking
{
public:
	CDemuxSource( LPUNKNOWN lpunk, HRESULT *phr, IDemuxReader *reader, CLSID clsid );
	virtual ~CDemuxSource();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv );

	// Methods of IAMFilterMiscFlags
	ULONG STDMETHODCALLTYPE GetMiscFlags(void);

	HRESULT OpenStream( IStream *stream );
	HRESULT Close() { return Reader()->Close(); }
	void ClearPins();

	STDMETHODIMP Pause();

	void UpdateFromSeek();
	HRESULT NewSegment();
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT DeliverEndOfStream(void);
	HRESULT DeliverEndFlush(void);
	HRESULT DeliverBeginFlush(void);
	
	// IMediaSeeking methods
	STDMETHODIMP IsFormatSupported(const GUID * pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
	STDMETHODIMP SetTimeFormat(const GUID * pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
	STDMETHODIMP GetTimeFormat(GUID *pFormat);
	STDMETHODIMP GetDuration(LONGLONG *pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG *pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
	STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
	STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );
	STDMETHODIMP ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat );
	STDMETHODIMP SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags, LONGLONG * pStop,  DWORD StopFlags );
	STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );
	STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );
	STDMETHODIMP SetRate( double dRate);
	STDMETHODIMP GetRate( double * pdRate);
	STDMETHODIMP GetPreroll(LONGLONG *pPreroll);

	// methods of Seeking
	HRESULT ChangeRate(void);
	HRESULT ChangeStart(void);
	HRESULT ChangeStop(void);

	ULONG GetNumberOfConnection();

private:

	CCritSec					m_crtFilterLock;
	IDemuxReader				*m_DemuxReader;
	IDemuxReader *Reader() { return m_DemuxReader; }
	
	CRefTime m_rtDuration;	//!< length of stream
	CRefTime m_rtStart;		//!< source will start here
	CRefTime m_rtStop;		//!< source will stop here
	double m_dRateSeeking;
	DWORD m_dwSeekingCaps;	// seeking capabilities

	friend class CDemuxSourceVideoOutputPin;
	friend class CDemuxSourceAudioOutputPin;
};



#endif	// __DEMUX_SOURCE_H__
