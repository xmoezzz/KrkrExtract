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

#include "tjsCommHead.h"
#include <assert.h>
#include <streams.h>
#include "CDemuxSource.h"
#include "CDemuxOutputPin.h"

// {68D40D07-25DB-4c49-AAAA-327DF25387AA}
static const GUID CLSID_DemuxSource = { 0x68d40d07, 0x25db, 0x4c49, { 0xaa, 0xaa, 0x32, 0x7d, 0xf2, 0x53, 0x87, 0xaa } };


//----------------------------------------------------------------------------
//! @brief	  	CDemuxSource constructor
//! @param		lpunk : 集成された所有者オブジェクトへのポインタ。
//! @param		phr : HRESULT 値へのポインタ。
//! @param		reader : このフィルタのリーダー
//! @param		clsid : このフィルタのクラスID
//----------------------------------------------------------------------------
CDemuxSource::CDemuxSource( LPUNKNOWN lpunk, HRESULT *phr, IDemuxReader *reader, CLSID clsid )
: CSource( NAME("Demux Source"), lpunk, clsid, phr ), m_DemuxReader(reader)//, m_cRef(0)
{
	m_rtStop = _I64_MAX / 2;
	m_rtDuration = m_rtStop;
	m_dRateSeeking = 1.0;
	m_dwSeekingCaps = AM_SEEKING_CanSeekForwards | AM_SEEKING_CanSeekBackwards | AM_SEEKING_CanSeekAbsolute
		| AM_SEEKING_CanGetStopPos | AM_SEEKING_CanGetDuration;
}
//----------------------------------------------------------------------------
//! @brief	  	CDemuxSource destructor
//----------------------------------------------------------------------------
CDemuxSource::~CDemuxSource()
{
	delete m_DemuxReader;
}
//----------------------------------------------------------------------------
//! @brief	  	要求されたインターフェイスを返す
//! @param		riid : インターフェイスのIID
//! @param		ppv : インターフェイスを返すポインターへのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP CDemuxSource::NonDelegatingQueryInterface( REFIID riid, void ** ppv )
{
	if( riid == IID_IMediaSeeking ) {
		return GetInterface( static_cast<IMediaSeeking*>( this ), ppv );
	} else if( riid == IID_IAMFilterMiscFlags ) {
		return GetInterface( static_cast<IAMFilterMiscFlags*>( this ), ppv );
	} else {
		return CSource::NonDelegatingQueryInterface(riid, ppv);
	}
}
//----------------------------------------------------------------------------
//! @brief	  	再生したいストリームを開く
//! @param		stream : ムービーへのストリーム
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::OpenStream( IStream *stream )
{
	CAutoLock lock(&m_crtFilterLock);

	HRESULT hr;
	if( (hr = Reader()->OpenStream(stream)) != S_OK )
		return hr;

	m_rtDuration = Reader()->GetDuration();
	m_rtStop = m_rtDuration;

	ClearPins();
	for( int i = 0; i < Reader()->GetNumberOfOutput(); ++i )
	{
		IOutputStream	*outStream = Reader()->GetOutputStream(i);
		assert( outStream );
		CDemuxOutputPin *pin = new CDemuxOutputPin( NAME("Demux Source"), this, &hr, L"Demux output pin", this, outStream, &m_crtFilterLock );
//		pin->AddRef();
		// AddPinはnewした段階でコールされるので、明示的に追加する必要はない
	}
	return hr;
}
//----------------------------------------------------------------------------
//! @brief	  	再生レートが変更された時にコールされる
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::ChangeRate(void)
{
	{	// クリティカル セクション ロックのスコープ。
		CAutoLock cAutoLock(&m_crtFilterLock);
		if( m_dRateSeeking <= 0 ) {
			m_dRateSeeking = 1.0;  // 適切な値にリセットする。
			return E_FAIL;
		}
	}
	UpdateFromSeek();
	return Reader()->SetRate( m_dRateSeeking );
}
//----------------------------------------------------------------------------
//! @brief	  	再生が開始された時にコールされる
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::ChangeStart(void)
{
	UpdateFromSeek();
	if( m_rtStart > m_rtDuration  )
		m_rtStart = m_rtDuration;	// 丸め

	return Reader()->SetStartTime( m_rtStart );
}
//----------------------------------------------------------------------------
//! @brief	  	再生が停止された時にコールされる
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::ChangeStop(void)
{
	UpdateFromSeek();
	return Reader()->SetEndTime(m_rtStop);
}
//----------------------------------------------------------------------------
//! @brief	  	保持しているピンをすべて削除する
//! @return		エラーコード
//----------------------------------------------------------------------------
void CDemuxSource::ClearPins()
{
	while( GetPinCount() > 0 )
		delete GetPin(GetPinCount()-1);
}
//----------------------------------------------------------------------------
//! @brief	  	フィルタをポーズする
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP CDemuxSource::Pause()
{
	{
		CAutoLock lock(&m_crtFilterLock);
		if( m_State == State_Stopped )
		{
			HRESULT	hr = Reader()->OnStart();
			if( FAILED(hr) )
				return hr;
			NewSegment();
		}
	}
	return CSource::Pause();
}
//----------------------------------------------------------------------------
//! @brief	  	シークからの変更を反映する
//! @return		エラーコード
//----------------------------------------------------------------------------
void CDemuxSource::UpdateFromSeek()
{
	DeliverBeginFlush();
	Stop();
	DeliverEndFlush();
}
//----------------------------------------------------------------------------
//! @brief	  	新たに再生が開始されたことを通知する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::NewSegment()
{
	return DeliverNewSegment(m_rtStart, m_rtStop, m_dRateSeeking);
}
//----------------------------------------------------------------------------
//! @brief	  	新たに再生が開始されたことをダウンストリームへ通知する
//! @param		tStart : 開始時間
//! @param		tStop : 停止時間
//! @param		dRate : 再生レート
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	int cPins = GetPinCount();
	for( int c = 0; c < cPins; c++ )
	{
		CBaseOutputPin *pPin = reinterpret_cast<CBaseOutputPin*>(GetPin(c));
		if( pPin->IsConnected() ) {
			HRESULT hr = pPin->DeliverNewSegment(tStart,tStop,dRate);
			if (FAILED(hr)) {
				return hr;
			}
		}
	}
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	ストリームが終端に達したことをダウンストリームへ通知する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::DeliverEndOfStream(void)
{
	int cPins = GetPinCount();
	for( int c = 0; c < cPins; c++ )
	{
		CBaseOutputPin *pPin = reinterpret_cast<CBaseOutputPin*>(GetPin(c));
		if( pPin->IsConnected() ) {
			HRESULT hr = pPin->DeliverEndOfStream();
			if (FAILED(hr)) {
				return hr;
			}
		}
	}
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	フラッシュ処理の終了をダウンストリームへ要求する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::DeliverEndFlush(void)
{
	int cPins = GetPinCount();
	for( int c = 0; c < cPins; c++ )
	{
		CBaseOutputPin *pPin = reinterpret_cast<CBaseOutputPin*>(GetPin(c));
		if( pPin->IsConnected() ) {
			HRESULT hr = pPin->DeliverEndFlush();
			if (FAILED(hr)) {
				return hr;
			}
		}
	}
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	フラッシュ処理の開始をダウンストリームへ要求する
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::DeliverBeginFlush(void)
{
	int cPins = GetPinCount();
	for( int c = 0; c < cPins; c++ )
	{
		CBaseOutputPin *pPin = reinterpret_cast<CBaseOutputPin*>(GetPin(c));
		if( pPin->IsConnected() ) {
			HRESULT hr = pPin->DeliverBeginFlush();
			if (FAILED(hr)) {
				return hr;
			}
		}
	}
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	フィルタタイプを取得する
//! @return		フィルタタイプ
//----------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE CDemuxSource::GetMiscFlags(void)
{
	return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}
//----------------------------------------------------------------------------
//! @brief	  	現在接続されているピンの数を取得する
//! @return		接続数
//----------------------------------------------------------------------------
ULONG CDemuxSource::GetNumberOfConnection()
{
	ULONG	result = 0;
	int cPins = GetPinCount();
	for( int c = 0; c < cPins; c++ )
	{
		CBaseOutputPin *pPin = reinterpret_cast<CBaseOutputPin*>(GetPin(c));
		if( pPin->IsConnected() ) {
			++result;
		}
	}
	return result;
}
//----------------------------------------------------------------------------
//! @brief	  	タイムフォーマットがサポートされているかどうか確認する
//! @param		pFormat : タイムフォーマット
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::IsFormatSupported(const GUID * pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	// only seeking in time (REFERENCE_TIME units) is supported
	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}
//----------------------------------------------------------------------------
//! @brief	  	ストリームの優先タイム フォーマットを取得する
//! @param		pFormat : タイムフォーマット
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::QueryPreferredFormat(GUID *pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	*pFormat = TIME_FORMAT_MEDIA_TIME;
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	タイムフォーマットを設定する
//! @param		pFormat : タイムフォーマット
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::SetTimeFormat(const GUID * pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	// nothing to set; just check that it's TIME_FORMAT_TIME
	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : E_INVALIDARG;
}
//----------------------------------------------------------------------------
//! @brief	  	指定したタイム フォーマットが現在使われているフォーマットかどうかを確認する
//! @param		pFormat : タイムフォーマット
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::IsUsingTimeFormat(const GUID * pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}
//----------------------------------------------------------------------------
//! @brief	  	現在のタイム フォーマットを取得する
//! @param		pFormat : タイムフォーマット
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::GetTimeFormat(GUID *pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	*pFormat = TIME_FORMAT_MEDIA_TIME;
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	ストリームの時間幅を取得する
//! @param		pDuration : 長さ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::GetDuration(LONGLONG *pDuration)
{
	CheckPointer(pDuration, E_POINTER);
	CAutoLock lock(&m_crtFilterLock);
	*pDuration = m_rtDuration;
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	ストリームの停止時間を取得する
//! @param		pStop : 停止時間
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::GetStopPosition(LONGLONG *pStop)
{
	CheckPointer(pStop, E_POINTER);
	CAutoLock lock(&m_crtFilterLock);
	*pStop = m_rtStop;
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	ストリームの現在時間を取得する
//! @param		pCurrent : 現在時間
//! @return		未サポート
//----------------------------------------------------------------------------
HRESULT CDemuxSource::GetCurrentPosition(LONGLONG *pCurrent)
{
	// GetCurrentPosition is typically supported only in renderers and
	// not in source filters.
	return E_NOTIMPL;
}
//----------------------------------------------------------------------------
//! @brief	  	シーク能力を取得する
//! @param		pCapabilities : シーク能力
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::GetCapabilities( DWORD * pCapabilities )
{
	CheckPointer(pCapabilities, E_POINTER);
	*pCapabilities = m_dwSeekingCaps;
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	指定したシーク能力をストリームが持っているかどうかを問い合わせる
//! @param		pCapabilities : シーク能力
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::CheckCapabilities( DWORD * pCapabilities )
{
	CheckPointer(pCapabilities, E_POINTER);
	// make sure all requested capabilities are in our mask
	return (~m_dwSeekingCaps & *pCapabilities) ? S_FALSE : S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	1 つのタイム フォーマットから別のタイム フォーマットに変換する
//! @param		pTarget : 変換されたタイムを受け取る変数へのポインタ
//! @param		pTargetFormat : ターゲット フォーマットのタイム フォーマット GUID へのポインタ。NULL の場合は、現在のフォーマットが使われる
//! @param		Source : 変換するタイム値
//! @param		pSourceFormat : 変換するフォーマットのタイム フォーマット GUID へのポインタ。NULL の場合は、現在のフォーマットが使われる
//! @return		エラーコード
//! @note	TIME_FORMAT_MEDIA_TIME以外はサポートしていない
//----------------------------------------------------------------------------
HRESULT CDemuxSource::ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat )
{
	CheckPointer(pTarget, E_POINTER);
	// format guids can be null to indicate current format

	// since we only support TIME_FORMAT_MEDIA_TIME, we don't really
	// offer any conversions.
	if(pTargetFormat == 0 || *pTargetFormat == TIME_FORMAT_MEDIA_TIME)
	{
		if(pSourceFormat == 0 || *pSourceFormat == TIME_FORMAT_MEDIA_TIME)
		{
			*pTarget = Source;
			return S_OK;
		}
	}
	return E_INVALIDARG;
}
//----------------------------------------------------------------------------
//! @brief	  	現在位置と停止位置を設定する
//! @param		pCurrent : 現在位置を指定する変数へのポインタ、現在のタイム フォーマットの単位
//! @param		CurrentFlags : 位置を指定するためのフラグのビットごとの組み合わせ
//! @param		pStop : 終了タイムを指定する変数へのポインタ、現在のタイム フォーマットの単位
//! @param		StopFlags : 位置を指定するためのフラグのビットごとの組み合わせ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags, LONGLONG * pStop,  DWORD StopFlags )
{
	DWORD StopPosBits = StopFlags & AM_SEEKING_PositioningBitsMask;
	DWORD StartPosBits = CurrentFlags & AM_SEEKING_PositioningBitsMask;

	if(StopFlags) {
		CheckPointer(pStop, E_POINTER);
		// accept only relative, incremental, or absolute positioning
		if(StopPosBits != StopFlags) {
			return E_INVALIDARG;
		}
	}

	if(CurrentFlags) {
		CheckPointer(pCurrent, E_POINTER);
		if(StartPosBits != AM_SEEKING_AbsolutePositioning &&
			StartPosBits != AM_SEEKING_RelativePositioning) {
			return E_INVALIDARG;
		}
	}

	{	// scope for autolock
		CAutoLock lock(&m_crtFilterLock);

		// set start position
		if(StartPosBits == AM_SEEKING_AbsolutePositioning) {
			m_rtStart = *pCurrent;
		} else if(StartPosBits == AM_SEEKING_RelativePositioning) {
			m_rtStart += *pCurrent;
		}

		// set stop position
		if(StopPosBits == AM_SEEKING_AbsolutePositioning) {
			m_rtStop = *pStop;
		} else if(StopPosBits == AM_SEEKING_IncrementalPositioning) {
			m_rtStop = m_rtStart + *pStop;
		} else if(StopPosBits == AM_SEEKING_RelativePositioning) {
			m_rtStop = m_rtStop + *pStop;
		}
	}

	HRESULT hr = S_OK;
	if(SUCCEEDED(hr) && StopPosBits) {
		hr = ChangeStop();
	}
	if(StartPosBits) {
		hr = ChangeStart();
	}
	return hr;
}
//----------------------------------------------------------------------------
//! @brief	  	現在の位置と停止位置を取得する
//! @param		pCurrent : 開始位置を受け取る変数へのポインタ
//! @param		pStop : 停止位置を受け取る変数へのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
	if(pCurrent) {
		*pCurrent = m_rtStart;
	}
	if(pStop) {
		*pStop = m_rtStop;
	}
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	シークが有効なタイムの範囲を取得する
//! @param		pEarliest : シークが有効な最も早いタイムを受け取る変数へのポインタ
//! @param		pLatest : シークが有効な最も遅いタイムを受け取る変数へのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
	if(pEarliest) {
		*pEarliest = 0;
	}
	if(pLatest) {
		CAutoLock lock(&m_crtFilterLock);
		*pLatest = m_rtDuration;
	}
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	再生レートを設定する
//! @param		dRate : 再生レート
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::SetRate( double dRate)
{
	{
		CAutoLock lock(&m_crtFilterLock);
		m_dRateSeeking = dRate;
	}
	return ChangeRate();
}
//----------------------------------------------------------------------------
//! @brief	  	再生レートを取得する
//! @param		dRate : 再生レート
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::GetRate( double * pdRate)
{
	CheckPointer(pdRate, E_POINTER);
	CAutoLock lock(&m_crtFilterLock);
	*pdRate = m_dRateSeeking;
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	開始位置の前にキューに入るデータの量を取得する
//! @param		pPreroll : プリロール タイムを受け取る変数へのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
HRESULT CDemuxSource::GetPreroll(LONGLONG *pPreroll)
{
	CheckPointer(pPreroll, E_POINTER);
	*pPreroll = 0;
	return S_OK;
}
