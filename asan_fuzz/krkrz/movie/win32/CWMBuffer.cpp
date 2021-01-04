/****************************************************************************/
/*! @file
@brief DirectShow media sample wrapper for Windows media format

DirectShowのMedia SampleをラップしたWindows Media Format SDK用のBuffer
-----------------------------------------------------------------------------
	Copyright (C) 2005 T.Imoto <http://www.kaede-software.com>
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2005/10/28
@note
*****************************************************************************/

#include "CWMBuffer.h"

//----------------------------------------------------------------------------
//! @brief	  	要求されたインターフェイスを返す
//! @param		riid : インターフェイスのIID
//! @param		ppv : インターフェイスを返すポインターへのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP CWMBuffer::NonDelegatingQueryInterface( REFIID riid, void ** ppv )
{
	if( IID_INSSBuffer == riid ) {
		return GetInterface(static_cast<INSSBuffer*>(this), ppv);
	} else if( riid == IID_IMediaSample ) {
		return GetInterface(static_cast<IMediaSample*>(m_Sample), ppv);
	} else {
		return CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}
}
//----------------------------------------------------------------------------
//! @brief	  	バッファを取得する
//! @param		ppdwBuffer : ポインタのポインタ
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP CWMBuffer::GetBuffer( BYTE **ppdwBuffer )
{
	return Sample()->GetPointer( ppdwBuffer );
}
//----------------------------------------------------------------------------
//! @brief	  	バッファとサイズを取得する
//! @param		ppdwBuffer : ポインタのポインタ
//! @param		pdwLength : サイズ
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP CWMBuffer::GetBufferAndLength( BYTE **ppdwBuffer, DWORD *pdwLength )
{
	*pdwLength = static_cast<DWORD>( Sample()->GetActualDataLength() );
	return Sample()->GetPointer( ppdwBuffer );
}
//----------------------------------------------------------------------------
//! @brief	  	データが格納されている実サイズを取得する
//! @param		pdwLength : サイズ
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP CWMBuffer::GetLength( DWORD *pdwLength )
{
	*pdwLength = static_cast<DWORD>( Sample()->GetActualDataLength() );
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	バッファのサイズを取得する
//! @param		pdwLength : サイズ
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP CWMBuffer::GetMaxLength( DWORD *pdwLength )
{
	*pdwLength = static_cast<DWORD>( Sample()->GetSize() );
	return S_OK;
}
//----------------------------------------------------------------------------
//! @brief	  	サイズを設定する
//! @param		dwLength : サイズ
//! @return		エラーコード
//----------------------------------------------------------------------------
STDMETHODIMP CWMBuffer::SetLength( DWORD dwLength )
{
	return Sample()->SetActualDataLength( static_cast<long>( dwLength ) );
}

