#include "my.h"
#include <gdiplus.h>  
#include "tp_stub.h"

#pragma comment(lib, "gdiplus")

class tTJSBinaryStream2
{
private:
public:
	//-- must implement
	virtual tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence) = 0;
	/* if error, position is not changed */

	//-- optionally to implement
	virtual tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size) = 0;
	/* returns actually read size */

	virtual tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size) = 0;
	/* returns actually written size */

	virtual void TJS_INTF_METHOD SetEndOfStorage();
	// the default behavior is raising a exception
	/* if error, raises exception */

	//-- should re-implement for higher performance
	virtual tjs_uint64 TJS_INTF_METHOD GetSize() = 0;

	virtual ~tTJSBinaryStream2() { ; }

	tjs_uint64 GetPosition();

	void SetPosition(tjs_uint64 pos);

	void ReadBuffer(void *buffer, tjs_uint read_size);
	void WriteBuffer(const void *buffer, tjs_uint write_size);

	tjs_uint64 ReadI64LE(); // reads little-endian integers
	tjs_uint32 ReadI32LE();
	tjs_uint16 ReadI16LE();
};


void TJS_INTF_METHOD tTJSBinaryStream2::SetEndOfStorage()
{
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTJSBinaryStream2::GetSize()
{
	tjs_uint64 orgpos = GetPosition();
	tjs_uint64 size = Seek(0, TJS_BS_SEEK_END);
	Seek(orgpos, SEEK_SET);
	return size;
}
//---------------------------------------------------------------------------
tjs_uint64 tTJSBinaryStream2::GetPosition()
{
	return Seek(0, SEEK_CUR);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream2::SetPosition(tjs_uint64 pos)
{
	Seek(pos, TJS_BS_SEEK_SET);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream2::ReadBuffer(void *buffer, tjs_uint read_size)
{
	Read(buffer, read_size);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream2::WriteBuffer(const void *buffer, tjs_uint write_size)
{
	Write(buffer, write_size);
}
//---------------------------------------------------------------------------
tjs_uint64 tTJSBinaryStream2::ReadI64LE()
{
	tjs_uint64 temp;
	ReadBuffer(&temp, 8);
	return temp;
}
//---------------------------------------------------------------------------
tjs_uint32 tTJSBinaryStream2::ReadI32LE()
{
	tjs_uint32 temp;
	ReadBuffer(&temp, 4);
	return temp;
}
//---------------------------------------------------------------------------
tjs_uint16 tTJSBinaryStream2::ReadI16LE()
{
	tjs_uint16 temp;
	ReadBuffer(&temp, 2);
	return temp;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders  
	UINT  size = 0;         // size of the image encoder array in bytes  

	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	if (pImageCodecInfo == NULL)
		return -1; 

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (lstrcmpW(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			HeapFree(GetProcessHeap(), 0, pImageCodecInfo);
			return j; 
		}
	}

	HeapFree(GetProcessHeap(), 0, pImageCodecInfo);
	return -1;
}


class tTVPMemoryStream : public tTJSBinaryStream2
{
protected:
	void * Block;
	tjs_uint Size;
	tjs_uint AllocSize;
	tjs_uint CurrentPos;

public:
	tTVPMemoryStream();
	tTVPMemoryStream(const void * block, tjs_uint size);
	~tTVPMemoryStream();

	tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence);

	tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size);
	tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size);
	void TJS_INTF_METHOD SetEndOfStorage();

	tjs_uint64 TJS_INTF_METHOD GetSize() { return Size; }

	// non-tTJSBinaryStream based methods
	void * GetInternalBuffer()  const { return Block; }
	void Clear(void);
	void SetSize(tjs_uint size);

protected:
	void Init();

protected:
	virtual void * Alloc(size_t size);
	virtual void * Realloc(void *orgblock, size_t size);
	virtual void Free(void *block);
};


tTVPMemoryStream::tTVPMemoryStream()
{
	Init();
}
//---------------------------------------------------------------------------
tTVPMemoryStream::tTVPMemoryStream(const void * block, tjs_uint size)
{
	Init();
	Block = (void*)block;
	Size = size;
	AllocSize = size;
	CurrentPos = 0;
}
//---------------------------------------------------------------------------
tTVPMemoryStream::~tTVPMemoryStream()
{
	if (Block) Free(Block);
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPMemoryStream::Seek(tjs_int64 offset, tjs_int whence)
{
	tjs_int64 newpos;
	switch (whence)
	{
	case TJS_BS_SEEK_SET:
		if (offset >= 0)
		{
			if (offset <= Size) CurrentPos = static_cast<tjs_uint>(offset);
		}
		return CurrentPos;

	case TJS_BS_SEEK_CUR:
		if ((newpos = offset + (tjs_int64)CurrentPos) >= 0)
		{
			tjs_uint np = (tjs_uint)newpos;
			if (np <= Size) CurrentPos = np;
		}
		return CurrentPos;

	case TJS_BS_SEEK_END:
		if ((newpos = offset + (tjs_int64)Size) >= 0)
		{
			tjs_uint np = (tjs_uint)newpos;
			if (np <= Size) CurrentPos = np;
		}
		return CurrentPos;
	}
	return CurrentPos;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPMemoryStream::Read(void *buffer, tjs_uint read_size)
{
	if (CurrentPos + read_size >= Size)
	{
		read_size = Size - CurrentPos;
	}

	memcpy(buffer, (tjs_uint8*)Block + CurrentPos, read_size);

	CurrentPos += read_size;

	return read_size;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPMemoryStream::Write(const void *buffer, tjs_uint write_size)
{
	tjs_uint newpos = CurrentPos + write_size;
	if (newpos >= AllocSize)
	{
		// exceeds AllocSize
		tjs_uint onesize;
		if (AllocSize < 64 * 1024) onesize = 4 * 1024;
		else if (AllocSize < 512 * 1024) onesize = 16 * 1024;
		else if (AllocSize < 4096 * 1024) onesize = 256 * 1024;
		else onesize = 2024 * 1024;
		AllocSize += onesize;

		if (CurrentPos + write_size >= AllocSize) // still insufficient ?
		{
			AllocSize = CurrentPos + write_size;
		}

		Block = Realloc(Block, AllocSize);

		if (AllocSize && !Block)
			return 0;
	}

	memcpy((tjs_uint8*)Block + CurrentPos, buffer, write_size);

	CurrentPos = newpos;

	if (CurrentPos > Size) Size = CurrentPos;

	return write_size;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPMemoryStream::SetEndOfStorage()
{
	Size = CurrentPos;
	AllocSize = Size;
	Block = Realloc(Block, Size);
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Clear(void)
{
	if (Block) Free(Block);
	Init();
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::SetSize(tjs_uint size)
{
	if (Size > size)
	{
		// decrease
		Size = size;
		AllocSize = size;
		Block = Realloc(Block, size);
		if (CurrentPos > Size) CurrentPos = Size;
		if (size && !Block)
			return;
	}
	else
	{
		// increase
		AllocSize = size;
		Size = size;
		Block = Realloc(Block, size);
		if (size && !Block)
			return;

	}
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Init()
{
	Block = NULL;
	Size = 0;
	AllocSize = 0;
	CurrentPos = 0;
}
//---------------------------------------------------------------------------
void * tTVPMemoryStream::Alloc(size_t size)
{
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}
//---------------------------------------------------------------------------
void * tTVPMemoryStream::Realloc(void *orgblock, size_t size)
{
	return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, orgblock, size);
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Free(void *block)
{
	HeapFree(GetProcessHeap(), 0, block);
}

class tTVPIStreamAdapter2 : public IStream
{
private:
	tTJSBinaryStream2 *Stream;
	ULONG RefCount;

public:
	tTVPIStreamAdapter2(tTJSBinaryStream2 *ref);
	/*
	the stream passed by argument here is freed by this instance'
	destruction.
	*/

	~tTVPIStreamAdapter2();


	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
		void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

	// ISequentialStream
	HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
	HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb,
		ULONG *pcbWritten);

	// IStream
	HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove,
		DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
	HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb,
		ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
	HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
	HRESULT STDMETHODCALLTYPE Revert(void);
	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset,
		ULARGE_INTEGER cb, DWORD dwLockType);
	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset,
		ULARGE_INTEGER cb, DWORD dwLockType);
	HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag);
	HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm);

	void ClearStream() {
		Stream = NULL;
	}
};



tTVPIStreamAdapter2::tTVPIStreamAdapter2(tTJSBinaryStream2 *ref)
{
	Stream = ref;
	RefCount = 1;
}
//---------------------------------------------------------------------------
tTVPIStreamAdapter2::~tTVPIStreamAdapter2()
{
	delete Stream;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::QueryInterface(REFIID riid,
	void **ppvObject)
{
	if (!ppvObject) return E_INVALIDARG;

	*ppvObject = NULL;
	if (RtlCompareMemory(&riid, &IID_IUnknown, 16) == 16)
		*ppvObject = (IUnknown*)this;
	else if (RtlCompareMemory(&riid, &IID_ISequentialStream, 16) == 16)
		*ppvObject = (ISequentialStream*)this;
	else if (RtlCompareMemory(&riid, &IID_IStream, 16) == 16)
		*ppvObject = (IStream*)this;

	if (*ppvObject)
	{
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE tTVPIStreamAdapter2::AddRef(void)
{
	return ++RefCount;
}
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE tTVPIStreamAdapter2::Release(void)
{
	if (RefCount == 1)
	{
		delete this;
		return 0;
	}
	else
	{
		return --RefCount;
	}
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	ULONG read;
	read = Stream->Read(pv, cb);
	if (pcbRead) *pcbRead = read;
	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Write(const void *pv, ULONG cb,
	ULONG *pcbWritten)
{
	return E_FAIL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Seek(LARGE_INTEGER dlibMove,
	DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	switch (dwOrigin)
	{
	case STREAM_SEEK_SET:
		if (plibNewPosition)
			(*plibNewPosition).QuadPart =
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_SET);
		else
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_SET);
		break;
	case STREAM_SEEK_CUR:
		if (plibNewPosition)
			(*plibNewPosition).QuadPart =
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_CUR);
		else
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_CUR);
		break;
	case STREAM_SEEK_END:
		if (plibNewPosition)
			(*plibNewPosition).QuadPart =
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_END);
		else
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_END);
		break;
	default:
		return E_FAIL;
	}
	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::SetSize(ULARGE_INTEGER libNewSize)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::CopyTo(IStream *pstm, ULARGE_INTEGER cb,
	ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Commit(DWORD grfCommitFlags)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Revert(void)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::LockRegion(ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb, DWORD dwLockType)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::UnlockRegion(ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb, DWORD dwLockType)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	if (pstatstg)
	{
		ZeroMemory(pstatstg, sizeof(*pstatstg));

		if (!(grfStatFlag &  STATFLAG_NONAME))
		{
			// anyway returns an empty string
			LPWSTR str = (LPWSTR)CoTaskMemAlloc(sizeof(*str));
			if (str == NULL) return E_OUTOFMEMORY;
			*str = L'\0';
			pstatstg->pwcsName = str;
		}

		// type
		pstatstg->type = STGTY_STREAM;

		pstatstg->cbSize.QuadPart = Stream->GetSize();
		pstatstg->grfMode = STGM_DIRECT | STGM_READWRITE | STGM_SHARE_DENY_WRITE;
		pstatstg->grfLocksSupported = 0;

	}
	else
	{
		return E_INVALIDARG;
	}

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Clone(IStream **ppstm)
{
	return E_NOTIMPL;
}

static BOOL  g_InitGdiPlus = FALSE;
static CLSID g_EncoderClsid;
static CLSID g_EncoderClsidJPG;

VOID InitGdiPlus()
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	GetEncoderClsid(L"image/png", &g_EncoderClsid);
	GetEncoderClsid(L"image/jpeg", &g_EncoderClsidJPG);
}

NTSTATUS Bmp2PNG(PBYTE Buffer, ULONG Size, PCWSTR Path)
{
	if (!g_InitGdiPlus)
		InitGdiPlus();

	IStream* Stream;

	Stream = new tTVPIStreamAdapter2(new tTVPMemoryStream(Buffer, Size));

	Gdiplus::Image* Img = new Gdiplus::Image(Stream);
	Gdiplus::Status GStatus = Img->Save(Path, &g_EncoderClsid, NULL);
	delete Img;
	delete Stream;

	return STATUS_SUCCESS;
}


NTSTATUS Bmp2JPG(PBYTE Buffer, ULONG Size, PCWSTR Path)
{
	if (!g_InitGdiPlus)
		InitGdiPlus();

	ULONG    Quality;
	IStream* Stream;
	Gdiplus::EncoderParameters JPGEncoderParameters;

	Stream = new tTVPIStreamAdapter2(new tTVPMemoryStream(Buffer, Size));

	JPGEncoderParameters.Count = 1;
	JPGEncoderParameters.Parameter[0].Guid = Gdiplus::EncoderQuality;
	JPGEncoderParameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
	JPGEncoderParameters.Parameter[0].NumberOfValues = 1;
	Quality = 100;
	JPGEncoderParameters.Parameter[0].Value = &Quality;

	Gdiplus::Image* Img = new Gdiplus::Image(Stream);
	Gdiplus::Status GStatus = Img->Save(Path, &g_EncoderClsidJPG, &JPGEncoderParameters);
	delete Img;
	delete Stream;

	return STATUS_SUCCESS;
}

NTSTATUS Image2JPG(PBYTE Buffer, ULONG Size, PCWSTR Path)
{
	return Bmp2JPG(Buffer, Size, Path);
}
