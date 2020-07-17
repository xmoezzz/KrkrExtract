#pragma once

#include "my.h"
#include "BaseStream.h"

class StreamHolderXP3
{
private:
	DWORD         mRtcBufferSize;
	PBYTE         mRtcBuffer;
	MemoryStream* mStream;

public:
	StreamHolderXP3(LPVOID Buffer, ULONG BufferSize);
	~StreamHolderXP3();

	ULONG64 WINAPI Seek(LONG64 Offset, LONG  Whence);
	ULONG64 WINAPI Read(LPVOID ReadBuffer, ULONG ReadSize);
	ULONG64 WINAPI Write(LPVOID buffer, ULONG WriteSize);
	ULONG64 WINAPI GetSize();
};


