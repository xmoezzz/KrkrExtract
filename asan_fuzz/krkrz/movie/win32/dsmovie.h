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

#ifndef __DSMOVIE_H_
#define __DSMOVIE_H_

//#define _CRTDBG_MAP_ALLOC

#include <assert.h>
#include <atlbase.h>
#include <streams.h>
#include "krmovie.h"
#include <vector>

class CIStreamProxy;
class CIStreamReader;
#ifdef ENABLE_THEORA
extern const GUID MEDIASUBTYPE_Ogg;
#endif
//----------------------------------------------------------------------------
//! @brief DirectShowクラス
//!
//! レイヤー描画、オーバーレイの基底クラス
//----------------------------------------------------------------------------
class tTVPDSMovie : public iTVPVideoOverlay
{
protected:
	ULONG		RefCount;
	HWND		OwnerWindow;
	bool		Visible;
	bool		Shutdown;
	RECT		Rect;

	DWORD		m_dwROTReg;	//!< 値はかぶらないように適切な物にすること
	bool		m_RegisteredROT;

	CIStreamProxy			*m_Proxy;
	CIStreamReader			*m_Reader;

	CComPtr<IGraphBuilder>	m_GraphBuilder;		//!< Graph Builder
	CComPtr<IMediaControl>	m_MediaControl;		//!< Media Control
	CComPtr<IMediaPosition>	m_MediaPosition;	//!< Media Postion
	CComPtr<IMediaSeeking >	m_MediaSeeking;		//!< Media Seeking
	CComPtr<IMediaEventEx>	m_MediaEventEx;		//!< Media Event
	CComPtr<IBasicVideo>	m_BasicVideo;		//!< Basic Video
	CComPtr<IBasicAudio>	m_BasicAudio;		//!< Basic Audio

	CComPtr<IAMStreamSelect>	m_StreamSelect;	//!< Stream selector

	struct StreamInfo {
		DWORD	groupNum;
		long	index;
	};
	std::vector<StreamInfo>	m_AudioStreamInfo;
	std::vector<StreamInfo>	m_VideoStreamInfo;
	//----------------------------------------------------------------------------
	//! @brief	  	IMediaSeekingを取得する
	//! @return		IMediaSeekingインターフェイス
	//----------------------------------------------------------------------------
	IMediaSeeking *MediaSeeking()
	{
		//assert( m_MediaSeeking.p );
		if( !m_MediaSeeking.p )
			assert( m_MediaSeeking.p );
		return m_MediaSeeking;
	}
	//----------------------------------------------------------------------------
	//! @brief	  	IMediaPositionを取得する
	//! @return		IMediaPositionインターフェイス
	//----------------------------------------------------------------------------
	IMediaPosition *Position()
	{
		assert( m_MediaPosition.p );
		return m_MediaPosition;
	}
	//----------------------------------------------------------------------------
	//! @brief	  	IMediaControlを取得する
	//! @return		IMediaControlインターフェイス
	//----------------------------------------------------------------------------
	IMediaControl *Controller()
	{ 
		assert( m_MediaControl.p );
		return m_MediaControl;
	}
	//----------------------------------------------------------------------------
	//! @brief	  	IMediaEventExを取得する
	//! @return		IMediaEventExインターフェイス
	//----------------------------------------------------------------------------
	IMediaEventEx *Event()
	{
		assert( m_MediaEventEx.p );
		return m_MediaEventEx;
	}
	//----------------------------------------------------------------------------
	//! @brief	  	IGraphBuilderを取得する
	//! @return		IGraphBuilderインターフェイス
	//----------------------------------------------------------------------------
	IGraphBuilder *GraphBuilder()
	{
		assert( m_GraphBuilder.p );
		return m_GraphBuilder;
	}
	//----------------------------------------------------------------------------
	//! @brief	  	IBasicVideoを取得する
	//! @return		IBasicVideoインターフェイス
	//----------------------------------------------------------------------------
	IBasicVideo *Video()
	{
		assert( m_BasicVideo.p );
		return m_BasicVideo;
	}
	//----------------------------------------------------------------------------
	//! @brief	  	IBasicAudioを取得する
	//!
	//! Audioは存在しない場合もあるので、NULLかどうか確認してから使用すること
	//! @return		IBasicAudioインターフェイス
	//----------------------------------------------------------------------------
	IBasicAudio *Audio()
	{
//		assert( m_BasicAudio.p );
		return m_BasicAudio;
	}
	//----------------------------------------------------------------------------
	//! @brief	  	IAMStreamSelectを取得する
	//!
	//! Audioを含むMPEGファイルでのみ使用可。NULLかどうか確認してから使用すること。
	//! @return		IAMStreamSelectインターフェイス
	//----------------------------------------------------------------------------
	IAMStreamSelect *StreamSelect()
	{
		return m_StreamSelect;
	}

	//----------------------------------------------------------------------------
	//! @brief	  	BigEndian <-> LittleEndian
	//! @param		l : 変換前
	//! @return		変換後
	//----------------------------------------------------------------------------
	inline unsigned long ChangeEndian32(unsigned long l)
	{
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
		return _byteswap_ulong(l);	// VC7.0以降
#else
#	if 1
		// ベタに書く
		unsigned long result;
		unsigned char *ps=(unsigned char *)&l;
		unsigned char *pd=(unsigned char *)&result;
		pd[0] = ps[3];
		pd[1] = ps[2];
		pd[2] = ps[1];
		pd[3] = ps[0];
		return result;
#	else
		// テンポラリ使わずに
		register unsigned char *p=(unsigned char *)&l;
		p[0]^=p[3];
		p[3]^=p[0];
		p[0]^=p[3];
		p[1]^=p[2];
		p[2]^=p[1];
		p[1]^=p[2];
		return l;
#	endif
#endif
	}

	HRESULT ConnectFilters( IBaseFilter* pFilterUpstream, IBaseFilter* pFilterDownstream );
	void BuildMPEGGraph( IBaseFilter *pRdr, IBaseFilter *pSrc );
	void ParseVideoType( CMediaType &mt, const wchar_t *type );
	bool IsWindowsMediaFile( const wchar_t *type ) const;
	void BuildWMVGraph( IBaseFilter *pRdr, IStream *pStream );
#ifdef ENABLE_THEORA
	void BuildTheoraGraph( IBaseFilter *pRdr, IBaseFilter *pSrc );
#endif
	void BuildPluginGraph( struct tTVPDSFilterHandlerType* handler, IBaseFilter *pRdr, IBaseFilter *pSrc );

	HRESULT __stdcall AddToROT( DWORD ROTreg );
	void __stdcall RemoveFromROT( DWORD ROTreg );

	void UtilDeleteMediaType( AM_MEDIA_TYPE *pmt );
	void DebugOutputPinMediaType( IPin *pPin );

	void UtilFreeMediaType( AM_MEDIA_TYPE& mt );
	HRESULT FindRenderer( const GUID *mediatype, IBaseFilter **ppFilter );
	HRESULT FindVideoRenderer( IBaseFilter **ppFilter );

	HRESULT GetPin( IBaseFilter * pFilter, PIN_DIRECTION dirrequired, int iNum, IPin **ppPin);
	IPin *GetInPin( IBaseFilter * pFilter, int nPin );
	IPin *GetOutPin( IBaseFilter * pFilter, int nPin );
	HRESULT CountFilterPins(IBaseFilter *pFilter, ULONG *pulInPins, ULONG *pulOutPins);

public:
	tTVPDSMovie();
	virtual ~tTVPDSMovie();

	virtual void __stdcall BuildGraph( HWND callbackwin, IStream *stream,
		const wchar_t * streamname, const wchar_t *type, unsigned __int64 size ) = 0;

	virtual void __stdcall AddRef();
	virtual void __stdcall Release();

	virtual void __stdcall ReleaseAll();

	virtual void __stdcall SetWindow(HWND window);
	virtual void __stdcall SetMessageDrainWindow(HWND window);
	virtual void __stdcall SetRect(RECT *rect);
	virtual void __stdcall SetVisible(bool b);

	virtual void __stdcall Play();
	virtual void __stdcall Stop();
	virtual void __stdcall Pause();
	virtual void __stdcall Rewind();

	virtual void __stdcall SetPosition(unsigned __int64 tick);
	virtual void __stdcall GetPosition(unsigned __int64 *tick);
	virtual void __stdcall GetStatus(tTVPVideoStatus *status);
	virtual void __stdcall GetEvent(long *evcode, LONG_PTR *param1, LONG_PTR *param2, bool *got);
	virtual void __stdcall FreeEventParams(long evcode, LONG_PTR param1, LONG_PTR param2);

	virtual void __stdcall SetFrame( int f );
	virtual void __stdcall GetFrame( int *f );
	virtual void __stdcall GetFPS( double *f );
	virtual void __stdcall GetNumberOfFrame( int *f );
	virtual void __stdcall GetTotalTime( __int64 *t );

	virtual void __stdcall GetFrontBuffer( BYTE **buff );
	virtual void __stdcall SetVideoBuffer( BYTE *buff1, BYTE *buff2, long size );

	virtual void __stdcall SetStopFrame( int frame );
	virtual void __stdcall GetStopFrame( int *frame );
	virtual void __stdcall SetDefaultStopFrame();

	virtual void __stdcall GetVideoSize( long *width, long *height );
	virtual HRESULT __stdcall GetAvgTimePerFrame( REFTIME *pAvgTimePerFrame );

	virtual void __stdcall SetPlayRate( double rate );
	virtual void __stdcall GetPlayRate( double *rate );

	virtual void __stdcall SetAudioBalance( long balance );
	virtual void __stdcall GetAudioBalance( long *balance );
	virtual void __stdcall SetAudioVolume( long volume );
	virtual void __stdcall GetAudioVolume( long *volume );

	virtual void __stdcall GetNumberOfAudioStream( unsigned long *streamCount );
	virtual void __stdcall SelectAudioStream( unsigned long num );
	virtual void __stdcall GetEnableAudioStreamNum( long *num );
	virtual void __stdcall DisableAudioStream( void );

	virtual void __stdcall GetNumberOfVideoStream( unsigned long *streamCount );
	virtual void __stdcall SelectVideoStream( unsigned long num );
	virtual void __stdcall GetEnableVideoStreamNum( long *num );

	virtual void __stdcall SetMixingBitmap( HDC hdc, RECT *dest, float alpha );
	virtual void __stdcall ResetMixingBitmap();

	virtual void __stdcall SetMixingMovieAlpha( float a );
	virtual void __stdcall GetMixingMovieAlpha( float *a );
	virtual void __stdcall SetMixingMovieBGColor( unsigned long col );
	virtual void __stdcall GetMixingMovieBGColor( unsigned long *col );
	virtual void __stdcall PresentVideoImage();


	virtual void __stdcall GetContrastRangeMin( float *v );
	virtual void __stdcall GetContrastRangeMax( float *v );
	virtual void __stdcall GetContrastDefaultValue( float *v );
	virtual void __stdcall GetContrastStepSize( float *v );
	virtual void __stdcall GetContrast( float *v );
	virtual void __stdcall SetContrast( float v );

	virtual void __stdcall GetBrightnessRangeMin( float *v );
	virtual void __stdcall GetBrightnessRangeMax( float *v );
	virtual void __stdcall GetBrightnessDefaultValue( float *v );
	virtual void __stdcall GetBrightnessStepSize( float *v );
	virtual void __stdcall GetBrightness( float *v );
	virtual void __stdcall SetBrightness( float v );

	virtual void __stdcall GetHueRangeMin( float *v );
	virtual void __stdcall GetHueRangeMax( float *v );
	virtual void __stdcall GetHueDefaultValue( float *v );
	virtual void __stdcall GetHueStepSize( float *v );
	virtual void __stdcall GetHue( float *v );
	virtual void __stdcall SetHue( float v );

	virtual void __stdcall GetSaturationRangeMin( float *v );
	virtual void __stdcall GetSaturationRangeMax( float *v );
	virtual void __stdcall GetSaturationDefaultValue( float *v );
	virtual void __stdcall GetSaturationStepSize( float *v );
	virtual void __stdcall GetSaturation( float *v );
	virtual void __stdcall SetSaturation( float v );

private:
	void __stdcall SelectStream( unsigned long num, std::vector<StreamInfo> &si );
	void __stdcall GetEnableStreamNum( long *num, std::vector<StreamInfo> &si );
};

#endif
