/****************************************************************************/
/*! @file
@brief Windows Media Reader

Windows Mediaファイルを読み込む
-----------------------------------------------------------------------------
	Copyright (C) 2005 T.Imoto <http://www.kaede-software.com>
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2005/10/22
@note
*****************************************************************************/

#ifndef __WM_READER_H__
#define __WM_READER_H__

#include <windows.h>
#include <tchar.h>
#include <streams.h>
#include <dshow.h>
#include <atlcomcli.h>
#include <assert.h>
#include <wmsdk.h>
#include <wmdxva.h>
#include <vector>
#include "IDemuxReader.h"
#include "CDLLLoader.h"
class CWMReader;

//----------------------------------------------------------------------------
//! @brief WM出力ストリーム
//----------------------------------------------------------------------------
class CWMOutput : public IOutputStream, public CUnknown
{
	CWMReader	*m_Reader;
	WORD		m_StreamNum;

	bool IsEnable() { return m_StreamNum != 0; }
	HRESULT SetStream();

	CWMReader *Reader()
	{
		assert( m_Reader );
		return m_Reader;
	}
	IWMSyncReader *WMReader();

public:
	CWMOutput( CWMReader *reader ) : CUnknown(NAME("WM Output"),NULL), m_Reader(reader), m_StreamNum(0) {}
	void SetStreamNumber( WORD num ) { m_StreamNum = num; }
	WORD GetStreamNumber() { return m_StreamNum; }
	virtual ~CWMOutput();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv );

	virtual HRESULT GetMediaType( int iPosition, CMediaType *pmt );
	virtual HRESULT SetMediaType( const CMediaType *pmt );
	virtual HRESULT GetNeedBufferSize( long &buffers, long &bufsize );
	virtual HRESULT SetDecidedBufferSize( long buffers, long bufsize, long aling, long prefix );
	virtual HRESULT GetNextSample( IMediaSample **pSample );
	virtual HRESULT SetAllocator( IMemAllocator *alloc );
	virtual bool IsDXVASubtype( const AM_MEDIA_TYPE *pmt );

	friend class CWMReader;
};

//----------------------------------------------------------------------------
//! @brief WMリーダー
//!
//! このクラスはCOMではない。CDemuxSourceによって保持され、CDemuxSourceが開放を行う
//! 利用時は、newし、そのポインタをCDemuxSource生成時に渡す。
//! 詳しくは tTVPDSMovie::BuildWMVGraph を参照のこと。
//----------------------------------------------------------------------------
class CWMReader : public IDemuxReader
{
	CComPtr<IWMSyncReader>	m_WMReader;
	CComPtr<IWMHeaderInfo>	m_HeaderInfo;
	CWMOutput			m_AudioOut;
	CWMOutput			m_VideoOut;
	CDLLLoader			m_WmvDll;

	QWORD		m_StartTime;
	QWORD		m_EndTime;
	double		m_Rate;

private:
	IWMSyncReader *Reader(void)
	{
		assert( m_WMReader.p );
		return m_WMReader;
	}
	IWMHeaderInfo *HeaderInf(void)
	{
		assert( m_HeaderInfo.p );
		return m_HeaderInfo;
	}
	HRESULT GetStreamNumbers( IWMProfile* pProfile );
	HRESULT SetRange(void);
	HRESULT SetStream(void);

public:
	CWMReader();
	virtual ~CWMReader();

	// method of IDemuxReader
	virtual int GetNumberOfOutput(void);
	virtual IOutputStream* GetOutputStream( int num );

	virtual HRESULT OpenStream( IStream *stream );
	virtual HRESULT Close(void);

	virtual QWORD GetDuration(void);
	virtual QWORD GetNumberOfFrame(void);
	virtual DWORD GetVideoWidth(void);
	virtual DWORD GetVideoHeight(void);
	virtual DWORD GetVideoFrameRate(void);

	virtual HRESULT SetStartTime( QWORD start );
	virtual HRESULT SetEndTime( QWORD end );
	virtual HRESULT SetRate( double rate );

	virtual HRESULT OnStart();

	// 
	QWORD GetAttributeQWORD( LPCWSTR name );
	DWORD GetAttributeDWORD( LPCWSTR name );

	friend class CWMOutput;
};


#endif // __WM_READER_H__
