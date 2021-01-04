/****************************************************************************/
/*! @file
@brief DirectShowのHRESULTをメッセージに変える例外クラス

-----------------------------------------------------------------------------
	Copyright (C) 2004 T.Imoto <http://www.kaede-software.com>
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2004/08/16
@note
			2004/08/16	T.Imoto		
*****************************************************************************/

#ifndef __DSHOW_EXCEPTION_H__
#define __DSHOW_EXCEPTION_H__

#include <dshow.h>

//----------------------------------------------------------------------------
//! @brief DirectShowのHRESULTをメッセージに変える例外クラス
//! 
//! 文字列の初期化をこまめに行うので、使用時はエラー時のみに限り、
//! エラー処理のif文の中でインスタンス化するようにした方がよい。
//----------------------------------------------------------------------------
class DShowException
{
	TCHAR	m_ErrorMes[MAX_ERROR_TEXT_LEN];
	HRESULT	m_Hr;

public:
	DShowException( ) throw( );
	DShowException(const DShowException& right) throw( );
	DShowException( HRESULT hr ) throw( );
	DShowException& operator=(const DShowException& right) throw( );
	virtual ~DShowException() throw( );
	virtual const TCHAR *what( ) const throw( );
	void SetHResult( HRESULT hr ) throw( );
};


void ThrowDShowException(const tjs_char *comment, HRESULT hr);

#endif	// __DSHOW_EXCEPTION_H__
