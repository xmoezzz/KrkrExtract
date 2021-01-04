/****************************************************************************/
/*! @file
@brief Demux Reader

ファイルからデータを読み込むデマルチプレクサ用のインターフェイス
-----------------------------------------------------------------------------
	Copyright (C) 2005 T.Imoto <http://www.kaede-software.com>
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2005/10/22
@note
			2005/10/22	T.Imoto		
*****************************************************************************/

#ifndef __DEMUX_READER_H__
#define __DEMUX_READER_H__

#include <windows.h>
//#include <dshow.h>
#include <tchar.h>
#include <streams.h>
#include <dshow.h>
#include <wmsdk.h>
#include <wmdxva.h>

//----------------------------------------------------------------------------
//! @brief Demuxの出力ストリーム インターフェイス
//----------------------------------------------------------------------------
struct IOutputStream
{
	IOutputStream(){}
	virtual ~IOutputStream(){}
	virtual HRESULT GetMediaType( int iPosition, CMediaType *pmt ) = 0;
	virtual HRESULT SetMediaType( const CMediaType *pmt ) = 0;
	virtual HRESULT GetNeedBufferSize( long &buffers, long &bufsize ) = 0;
	virtual HRESULT SetDecidedBufferSize( long buffers, long bufsize, long aling, long prefix ) = 0;
	virtual HRESULT GetNextSample( IMediaSample **pSample ) = 0;

	virtual HRESULT SetAllocator( IMemAllocator *alloc ) { return S_OK; }

	virtual bool IsDXVASubtype( const AM_MEDIA_TYPE *pmt ) { return false; }
};
//----------------------------------------------------------------------------
//! @brief Demux Reader インターフェイス
//----------------------------------------------------------------------------
struct IDemuxReader
{
	IDemuxReader(){}
	virtual ~IDemuxReader(){}

	virtual int GetNumberOfOutput(void) = 0;
	virtual IOutputStream* GetOutputStream( int num ) = 0;

	virtual HRESULT OpenStream( IStream *stream ) = 0;
	virtual HRESULT Close(void) = 0;

	virtual QWORD GetDuration(void) = 0;
	virtual QWORD GetNumberOfFrame(void) = 0;
	virtual DWORD GetVideoWidth(void) = 0;
	virtual DWORD GetVideoHeight(void) = 0;
	virtual DWORD GetVideoFrameRate(void) = 0;

	virtual HRESULT SetStartTime( QWORD start ) = 0;
	virtual HRESULT SetEndTime( QWORD end ) = 0;
	virtual HRESULT SetRate( double rate ) = 0;

	virtual HRESULT OnStart() = 0;
};


#endif // __DEMUX_READER_H__
