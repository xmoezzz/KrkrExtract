#include "TVPStream.h"
#include "WinFile.h"

class BinaryStream : public tTJSBinaryStream
{
private:
	
	WinFile File;
public:
	//-- must implement
	tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence);
	/* if error, position is not changed */

	//-- optionally to implement
	tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size);
	/* returns actually read size */

	tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size);
	/* returns actually written size */

	tjs_uint64 TJS_INTF_METHOD GetSize();

	~BinaryStream();
	BinaryStream(const ttstr& name, tjs_uint32 flags);
};

tjs_uint64 TJS_INTF_METHOD BinaryStream::Seek(tjs_int64 offset, tjs_int whence)
{
	DWORD Result = 0;
	File.Seek(offset, whence, Result);
	return Result;
}

tjs_uint TJS_INTF_METHOD BinaryStream::Read(void *buffer, tjs_uint read_size)
{
	DWORD Result = 0;
	File.Read((PBYTE)buffer, read_size, Result);
	return Result;
}

tjs_uint TJS_INTF_METHOD BinaryStream::Write(const void *buffer, tjs_uint write_size)
{
	DWORD Result = 0;
	File.Write((PBYTE)buffer, write_size);
	return Result;
}

tjs_uint64 TJS_INTF_METHOD BinaryStream::GetSize()
{
	return File.GetSize64();
}


BinaryStream::BinaryStream(const ttstr& name, tjs_uint32 flags)
{
	HRESULT ResultCode = S_OK;
	switch (flags)
	{
	case TJS_BS_READ:
		ResultCode = File.Open(name.c_str(), WinFile::FileRead);
		break;
	case TJS_BS_WRITE:
		ResultCode = File.Open(name.c_str(), WinFile::FileWrite);
		break;
	default:
		MessageBoxW(NULL, L"Flag must be Read-flag or Write-flag", L"Error", MB_OK);
		ExitProcess(-1);
		break;
	}
	if (FAILED(ResultCode))
	{
		MessageBoxW(NULL, L"Failed to craete file", L"Error", MB_OK);
		ExitProcess(-1);
	}
}

BinaryStream::~BinaryStream()
{
	File.Release();
}

//=============================================

tTJSBinaryStream * TVPCreateStream(const ttstr & _name, tjs_uint32 flags)
{
	return new BinaryStream(_name, flags);
}
