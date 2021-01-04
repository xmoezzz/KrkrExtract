#include "tjsCommHead.h"
//---------------------------------------------------------------------------
#include "tjsUtils.h"
#include "MsgIntf.h"
#include "BitmapBitsAlloc.h"
#include "SysInitIntf.h"
#include "EventIntf.h"
#include "DebugIntf.h"

class BasicAllocator : public iTVPMemoryAllocator
{
public:
	BasicAllocator() {
		TVPAddLog( TJS_W("(info) Use malloc for Bitmap") );
	}
	void* allocate( size_t size ) { return malloc(size); }	// Windowsでは ::HeapAlloc( _get_heap_handle(), 0, size ); と同じはず
	void free( void* mem ) { ::free( mem ); }
};
#ifdef WIN32
class GlobalAllocAllocator : public iTVPMemoryAllocator
{
public:
	GlobalAllocAllocator() {
		TVPAddLog( TJS_W("(info) Use GlobalAlloc allocater for Bitmap") );
	}
	void* allocate( size_t size ) { return GlobalAlloc(GMEM_FIXED,size); }
	void free( void* mem ) { GlobalFree((HGLOBAL)mem ); }
};
class HeapAllocAllocator : public iTVPMemoryAllocator
{
	static const DWORD HeapFlag = 0;

	HANDLE HeapHandle;
public:
	HeapAllocAllocator() : HeapHandle(NULL) {
		tTJSVariant val;
		tjs_uint64 size = 0;
		if(TVPGetCommandLine(TJS_W("-bitmapheapsize"), &val)) {
			ttstr str(val);
			if(str == TJS_W("auto")) {
				size = 0;
			} else {
				size = (tjs_int64)val;
				if( size == 0 ) {
					HeapHandle = ::HeapCreate( HeapFlag, 0, 0 );
				}
				size *= 1024*1024;
			}
		}
		if( HeapHandle == NULL ) {
			if( size == 0 ) {
				MEMORYSTATUSEX status = { sizeof(MEMORYSTATUSEX) };
				::GlobalMemoryStatusEx(&status);
				if( status.ullAvailVirtual < status.ullTotalPhys ) {
					size = status.ullAvailVirtual / 2;
				} else {
					size = status.ullTotalPhys / 2;
				}
			}
			while( HeapHandle == NULL && size > (1024*1024) ) {
				HeapHandle = ::HeapCreate( HeapFlag, (SIZE_T)size, 0 );
				if( HeapHandle == NULL ) {
					size /= 2;
				}
			} 
		}

		if( HeapHandle ) {
			ULONG HeapInformation = 2;
			BOOL lfhenable = ::HeapSetInformation( HeapHandle, HeapCompatibilityInformation, &HeapInformation, sizeof(HeapInformation) );
		}
		TVPAddLog( TJS_W("(info) Use separate heap allocater for Bitmap") );
	}
	virtual ~HeapAllocAllocator() {
		if( HeapHandle ) ::HeapDestroy(HeapHandle);
		HeapHandle = NULL;
	}
	void* allocate( size_t size ) {
		if( HeapHandle == NULL ) return NULL;
		void* result = ::HeapAlloc( HeapHandle, HeapFlag, size );
		if( result == NULL ) {
			::HeapCompact( HeapHandle, HeapFlag );	// try compact
			result = ::HeapAlloc( HeapHandle, HeapFlag, size ); // retry
		}
		return result;
	}
	void free( void* mem ) {
		if( HeapHandle ) {
			BOOL ret = ::HeapFree( HeapHandle, HeapFlag, mem );
			::HeapCompact( HeapHandle, HeapFlag );
		}
	}
};
class ProcessHeapAllocAllocator : public iTVPMemoryAllocator
{
public:
	ProcessHeapAllocAllocator() {
		TVPAddLog( TJS_W("(info) Use Process HeadAlloc allocater for Bitmap") );
	}
	void* allocate( size_t size ) {
		void* result = ::HeapAlloc( ::GetProcessHeap(), 0, size );
		if( result == NULL ) {
			::HeapCompact( ::GetProcessHeap(), 0 );	// try compact
			result = ::HeapAlloc( ::GetProcessHeap(), 0, size ); // retry
		}
		return result;
	}
	void free( void* mem ) {
		::HeapFree(::GetProcessHeap(), 0, mem);
	}
};
#endif

iTVPMemoryAllocator* tTVPBitmapBitsAlloc::Allocator = NULL;
tTJSCriticalSection tTVPBitmapBitsAlloc::AllocCS;

void tTVPBitmapBitsAlloc::InitializeAllocator() {
	if( Allocator == NULL ) {
		tTJSVariant val;
		if(TVPGetCommandLine(TJS_W("-bitmapallocator"), &val)) {
			ttstr str(val);
#ifdef WIN32
			if(str == TJS_W("globalalloc"))
				Allocator = new GlobalAllocAllocator();
			else if(str == TJS_W("separateheap"))
				Allocator = new HeapAllocAllocator();
			else if(str == TJS_W("processheap"))
				Allocator = new ProcessHeapAllocAllocator();
			else    // malloc
#endif
				Allocator = new BasicAllocator();
		} else {
#ifdef WIN32
			//Allocator = new GlobalAllocAllocator();
			Allocator = new ProcessHeapAllocAllocator();
#else
			Allocator = new BasicAllocator();
#endif
		}
	}
}
void tTVPBitmapBitsAlloc::FreeAllocator() {
	if( Allocator ) delete Allocator;
	Allocator = NULL;
}
static tTVPAtExit
	TVPUninitMessageLoad(TVP_ATEXIT_PRI_CLEANUP, tTVPBitmapBitsAlloc::FreeAllocator);

extern void TVPHeapDump();
void* tTVPBitmapBitsAlloc::Alloc( tjs_uint size, tjs_uint width, tjs_uint height ) {
	if(size == 0) return NULL;
	tTJSCriticalSectionHolder Lock(AllocCS);	// Lock

	InitializeAllocator();
	tjs_uint8 * ptrorg, * ptr;
	tjs_uint allocbytes = 16 + size + sizeof(tTVPLayerBitmapMemoryRecord) + sizeof(tjs_uint32)*2;

	ptr = ptrorg = (tjs_uint8*)Allocator->allocate(allocbytes);
	if(!ptr) {
		// Do GC
		TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MAX);
#ifdef WIN32
		// Do compact CRT and Global Heap
		HANDLE hHeap = ::GetProcessHeap();
		if( hHeap ) {
			::HeapCompact( hHeap, 0 );
		}
		HANDLE hCrtHeap = (HANDLE)_get_heap_handle();
		if( hCrtHeap && hCrtHeap != hHeap ) {
			::HeapCompact( hCrtHeap, 0 );
		}
#endif
		ptr = ptrorg = (tjs_uint8*)Allocator->allocate(allocbytes);
		if(!ptr) {
			TVPHeapDump();
			TVPThrowExceptionMessage(TVPCannotAllocateBitmapBits,
				TJS_W("at TVPAllocBitmapBits"), ttstr((tjs_int)allocbytes) + TJS_W("(") +
				ttstr((int)width) + TJS_W("x") + ttstr((int)height) + TJS_W(")"));
		}
	}
	// align to a paragraph ( 16-bytes )
	ptr += 16 + sizeof(tTVPLayerBitmapMemoryRecord);
	*reinterpret_cast<tTJSPointerSizedInteger*>(&ptr) >>= 4;
	*reinterpret_cast<tTJSPointerSizedInteger*>(&ptr) <<= 4;

	tTVPLayerBitmapMemoryRecord * record =
		(tTVPLayerBitmapMemoryRecord*)
		(ptr - sizeof(tTVPLayerBitmapMemoryRecord) - sizeof(tjs_uint32));

	// fill memory allocation record
	record->alloc_ptr = (void *)ptrorg;
	record->size = size;
	record->sentinel_backup1 = rand() + (rand() << 16);
	record->sentinel_backup2 = rand() + (rand() << 16);

	// set sentinel
	*(tjs_uint32*)(ptr - sizeof(tjs_uint32)) = ~record->sentinel_backup1;
	*(tjs_uint32*)(ptr + size              ) = ~record->sentinel_backup2;
		// Stored sentinels are nagated, to avoid that the sentinel backups in
		// tTVPLayerBitmapMemoryRecord becomes the same value as the sentinels.
		// This trick will make the detection of the memory corruption easier.
		// Because on some occasions, running memory writing will write the same
		// values at first sentinel and the tTVPLayerBitmapMemoryRecord.

	// return buffer pointer
	return ptr;
}
void tTVPBitmapBitsAlloc::Free( void* ptr ) {
	if(ptr)
	{
		tTJSCriticalSectionHolder Lock(AllocCS);	// Lock

		// get memory allocation record pointer
		tjs_uint8 *bptr = (tjs_uint8*)ptr;
		tTVPLayerBitmapMemoryRecord * record =
			(tTVPLayerBitmapMemoryRecord*)
			(bptr - sizeof(tTVPLayerBitmapMemoryRecord) - sizeof(tjs_uint32));

		// check sentinel
		if(~(*(tjs_uint32*)(bptr - sizeof(tjs_uint32))) != record->sentinel_backup1)
			TVPThrowExceptionMessage( TVPLayerBitmapBufferUnderrunDetectedCheckYourDrawingCode );
		if(~(*(tjs_uint32*)(bptr + record->size      )) != record->sentinel_backup2)
			TVPThrowExceptionMessage( TVPLayerBitmapBufferOverrunDetectedCheckYourDrawingCode );

		Allocator->free( record->alloc_ptr );
	}
}

