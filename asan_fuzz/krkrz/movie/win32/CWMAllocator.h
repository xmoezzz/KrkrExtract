/****************************************************************************/
/*! @file
@brief DirectShow allocator wrapper for Windows media format

DirectShowのアロケーターをラップしたWindows Media Format SDK用のアロケーター
-----------------------------------------------------------------------------
	Copyright (C) 2005 T.Imoto <http://www.kaede-software.com>
-----------------------------------------------------------------------------
@author		T.Imoto
@date		2005/10/28
@note
			2005/10/28	T.Imoto		
*****************************************************************************/

#ifndef __WINDOWS_MEDIA_ALLOCATOR__DSHOW_WRAPPER_H__
#define __WINDOWS_MEDIA_ALLOCATOR__DSHOW_WRAPPER_H__

#include <assert.h>
#include <windows.h>
//#include <dshow.h>
#include <atlcomcli.h>
#include <wmsdk.h>
#include <streams.h>
#include <dshow.h>

//----------------------------------------------------------------------------
//! @brief WMV用アロケーター
//----------------------------------------------------------------------------
class CWMAllocator : public IWMReaderAllocatorEx, public CUnknown
{
	CComPtr<IMemAllocator>	m_MemAlloc;

	IMemAllocator *Allocator()
	{
		assert( m_MemAlloc.p );
		return m_MemAlloc;
	}

public:
	CWMAllocator( IMemAllocator *alloc )
		: CUnknown(NAME("WM Allocator"),NULL), m_MemAlloc(alloc)
	{}
	virtual ~CWMAllocator();

	// Methods of IUnknown
	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv );

	// Methods of INSSBuffer
	STDMETHODIMP AllocateForStreamEx( WORD wStreamNum, DWORD cbBuffer, INSSBuffer **ppBuffer,
		DWORD dwFlags, QWORD cnsSampleTime, QWORD cnsSampleDuration, void *pvContext );

	STDMETHODIMP AllocateForOutputEx( DWORD dwOutputNum, DWORD cbBuffer, INSSBuffer **ppBuffer,
		DWORD dwFlags, QWORD cnsSampleTime, QWORD cnsSampleDuration, void *pvContext );
};

#endif // __WINDOWS_MEDIA_ALLOCATOR__DSHOW_WRAPPER_H__
