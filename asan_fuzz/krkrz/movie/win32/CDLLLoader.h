/****************************************************************************/
/*! @file
@brief DLL Loader

-----------------------------------------------------------------------------
	Copyright (C) 2005 T.Imoto ( http://www.kaede-software.com/ )
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2005/12/22
@note
*****************************************************************************/

#include <windows.h>

#ifndef __DLL_LOADER_H__
#define __DLL_LOADER_H__

//----------------------------------------------------------------------------
//! @brief DLLを読み込むためのクラス
//----------------------------------------------------------------------------
class CDLLLoader
{
	HMODULE		m_hModule;

public:
	CDLLLoader() : m_hModule(NULL) {}
	CDLLLoader(LPCTSTR lpFileName)
	{
		m_hModule = ::LoadLibrary( lpFileName );
	}
	~CDLLLoader() { Free(); }
	bool IsLoaded() { return( m_hModule != NULL ); }
	bool Load( LPCTSTR lpFileName )
	{
		Free();
		m_hModule = ::LoadLibrary( lpFileName );
		return( m_hModule != NULL );
	}
	void Free()
	{
		if( m_hModule )
			::FreeLibrary( m_hModule );
		m_hModule = NULL;
	}
	HRESULT GetLastError() { return HRESULT_FROM_WIN32( ::GetLastError() ); }

	//! @param lpProcName : 関数名
	FARPROC GetProcAddress( LPCSTR lpProcName )
	{
		return ::GetProcAddress( m_hModule, lpProcName );
	}
};



#endif // __DLL_LOADER_H__

