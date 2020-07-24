#include "PsbDecompression.h"
#include <zlib.h>
#include "lz4.h"
#include "lz4frame.h"

#ifndef LZ4_MAGIC
#define LZ4_MAGIC (ULONG)0x184D2204
#endif



unsigned char *getDataFromLz4(const unsigned char *buff, unsigned long &size)
{
	if (size <= 10 || *(PDWORD)buff != LZ4_MAGIC) return NULL;

	unsigned long uncompsize = *(unsigned long*)&buff[4];

	LZ4F_decompressionContext_t context;
	LZ4F_frameInfo_t frameInfo;
	LZ4F_errorCode_t err;
	size_t srcSize = size;
	unsigned char *uncompr_data;
	size_t pos, dstPos;
	size_t dstSize;

	err = LZ4F_createDecompressionContext(&context, LZ4F_VERSION);
	if (LZ4F_isError(err))
	{
		return NULL;
	}

	err = LZ4F_getFrameInfo(context, &frameInfo, buff, &srcSize);
	if (LZ4F_isError(err))
	{
		return NULL;
	}

	pos = srcSize;
	dstPos = 0;
	uncompr_data = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, LoDword(frameInfo.contentSize));

	do
	{
		dstSize = LoDword(frameInfo.contentSize) - dstPos;
		srcSize = size - pos;

		err = LZ4F_decompress(context, &uncompr_data[dstPos], &dstSize, &buff[pos], &srcSize, NULL);

		if (LZ4F_isError(err))
		{
			HeapFree(GetProcessHeap(), 0, uncompr_data);
			return nullptr;
		}

		dstPos += dstSize;
		pos += srcSize;
	} while (err);

	size = LoDword(frameInfo.contentSize);
	return uncompr_data;
}


std::shared_ptr<BYTE> GetDataFromMDF(_In_ PBYTE Buffer, _Inout_ ULONG &Size)
{
	ULONG OriginalSize, DecompressedSize;

	if (Size <= 10 || *(PDWORD)Buffer == TAG3("mdf")) 
		return NULL;

	OriginalSize     = *(PULONG)&Buffer[4];
	DecompressedSize = OriginalSize;
	auto OriginalBuffer = AllocateMemorySafeP<BYTE>(OriginalSize);
	if (!OriginalBuffer)
		return NULL;

	if (Z_OK != uncompress(OriginalBuffer.get(), &DecompressedSize, Buffer + 8, Size - 8) || DecompressedSize != OriginalSize)
		return NULL;
	
	Size = OriginalSize;
	return OriginalBuffer;
}

std::shared_ptr<BYTE> GetDataFromLZ4(_In_ PBYTE Buffer, _Inout_ ULONG &Size)
{
	LZ4F_decompressionContext_t Context;
	LZ4F_frameInfo_t            FrameInfo;
	LZ4F_errorCode_t            Status;
	size_t                      SrcSize;
	size_t                      Offset, DstPos;
	size_t                      DstSize;

	SrcSize = Size;
	if (Size <= 10 || *(PDWORD)Buffer != LZ4_MAGIC) 
		return NULL;

	//ULONG Uncompsize = *(PULONG)&Buffer[4];

	Status = LZ4F_createDecompressionContext(&Context, LZ4F_VERSION);
	if (LZ4F_isError(Status))
		return NULL;

	Status = LZ4F_getFrameInfo(Context, &FrameInfo, Buffer, &SrcSize);
	if (LZ4F_isError(Status))
		return NULL;

	Offset = SrcSize;
	DstPos = 0;
	auto OriginalBuffer = AllocateMemorySafeP<BYTE>(LoDword(FrameInfo.contentSize));

	do
	{
		DstSize = LoDword(FrameInfo.contentSize) - DstPos;
		SrcSize = Size - Offset;

		Status = LZ4F_decompress(Context, &OriginalBuffer.get()[DstPos], &DstSize, &Buffer[Offset], &SrcSize, NULL);

		if (LZ4F_isError(Status))
			return NULL;

		DstPos += DstSize;
		Offset += SrcSize;
	} while (Status);

	Size = LoDword(FrameInfo.contentSize);
	return OriginalBuffer;
}

std::shared_ptr<BYTE> GetDataFromMDF(_In_ std::shared_ptr<BYTE> Buffer, _Inout_ ULONG &Size)
{
	return GetDataFromMDF(Buffer.get(), Size);
}

std::shared_ptr<BYTE> GetDataFromLZ4(_In_ std::shared_ptr<BYTE> Buffer, _Inout_ ULONG &Size)
{
	return GetDataFromLZ4(Buffer.get(), Size);
}

