#include "StreamHolderXP3.h"

StreamHolderXP3::StreamHolderXP3(LPVOID Buffer, ULONG BufferSize) :
mRtcBufferSize(0),
mRtcBuffer(nullptr),
mStream(nullptr)
{
	mRtcBuffer = (PBYTE)Buffer;
	mRtcBufferSize = BufferSize;

	mStream = (MemoryStream*)AllocateMemoryP(sizeof(MemoryStream));
	mStream->Init(mRtcBuffer, mRtcBufferSize);
}

StreamHolderXP3::~StreamHolderXP3()
{
	if (mRtcBuffer)
		FreeMemoryP(mRtcBuffer);

	if (mStream)
		FreeMemoryP(mStream);

	mRtcBuffer = nullptr;
	mRtcBufferSize = 0;
}

ULONG64 WINAPI StreamHolderXP3::Seek(LONG64 offset, LONG  whence)
{
	return mStream->Seek(offset, whence);
}


ULONG64 WINAPI StreamHolderXP3::Read(LPVOID ReadBuffer, ULONG ReadSize)
{
	return mStream->Read(ReadBuffer, 1, ReadSize);
}

ULONG64 WINAPI StreamHolderXP3::Write(LPVOID Buffer, ULONG WriteSize)
{
	return mStream->Write(Buffer, 1, WriteSize);
}

ULONG64 WINAPI StreamHolderXP3::GetSize()
{
	return mStream->GetMemorySize();
}
