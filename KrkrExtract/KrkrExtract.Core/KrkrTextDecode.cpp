#include "zlib.h"
#include "Decoder.h"
#include "SafeMemory.h"


ForceInline ULONG_PTR IsUtf8String(PBYTE str, ULONG_PTR len)
{
	ULONG_PTR i = 0;
	ULONG_PTR continuation_bytes = 0;
	
	while (i < len)
	{
		if (str[i] <= 0x7F) {
			continuation_bytes = 0;
		}
		else if (str[i] >= 0xC0 /*11000000*/ && str[i] <= 0xDF /*11011111*/) {
			continuation_bytes = 1;
		}
		else if (str[i] >= 0xE0 /*11100000*/ && str[i] <= 0xEF /*11101111*/) {
			continuation_bytes = 2;
		}
		else if (str[i] >= 0xF0 /*11110000*/ && str[i] <= 0xF4 /* Cause of RFC 3629 */) {
			continuation_bytes = 3;
		}
		else {
			return i + 1;
		}

		i += 1;
		while (i < len && continuation_bytes > 0
			&& str[i] >= 0x80
			&& str[i] <= 0xBF)
		{
			i += 1;
			continuation_bytes -= 1;
		}
		if (continuation_bytes != 0)
			return i + 1;
	}
	return 0;
}

NTSTATUS DecodeText(KrkrClientProxyer* Proxyer, PCWSTR FileName, PBYTE Buffer, ULONG Size)
{
	NTSTATUS           Status;
	UCHAR              CryptMode;
	UINT64             Compressed, Uncompressed;
	ULONG              OutSize;
	INT                ZStatus;
	NtFileDisk         File;
	WORD               Bom;
	WCHAR              CurrentChar;
	PWSTR              CurrentBuffer;

	if (Size <= 2)
		return STATUS_BUFFER_TOO_SMALL;

	if (Buffer[0] != 0xfe || Buffer[1] != 0xfe)
		return STATUS_NOT_SUPPORTED;

	CryptMode = Buffer[2];

	switch (CryptMode)
	{
	case 2:
	{// compressed text stream
		Compressed = *(UINT64*)&Buffer[5];
		Uncompressed = *(UINT64*)&Buffer[13];
		if (Compressed != (DWORD)Compressed || Uncompressed != (DWORD)Uncompressed)
		{
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"Text[2] : size is too large (c : %llx, d : %llx)",
				Compressed,
				Uncompressed);
			return STATUS_NOT_SUPPORTED;
		}

		auto DecompressBuffer = AllocateMemorySafeP<BYTE>((ULONG)Uncompressed);
		if (!DecompressBuffer)
		{
			Proxyer->TellServerLogOutput(LogLevel::LOG_WARN, L"Text[2] : Failed to allocate memory (decompress)");
			return STATUS_NO_MEMORY;
		}

		OutSize = (ULONG)Uncompressed;
		ZStatus = uncompress(DecompressBuffer.get(), &OutSize, Buffer + 21, (uLongf)Compressed);
		if (ZStatus != Z_OK || OutSize != (uLongf)Uncompressed)
		{
			Proxyer->TellServerLogOutput(LogLevel::LOG_WARN, L"Text[2] : Failed to uncompress");
			return STATUS_UNSUCCESSFUL;
		}

		Status = File.Create(FileName);
		if (NT_FAILED(Status))
			return Status;

		if (IsUtf8String(DecompressBuffer.get(), OutSize) != OutSize)
		{
			Bom = 0xfeff;
			File.Write((PBYTE)&Bom, 2);
			File.Write(DecompressBuffer.get(), OutSize);
			File.Close();
		}
		else
		{
			File.Write(DecompressBuffer.get(), OutSize);
			File.Close();
		}
	}
	break;

	case 1:
	{
		if (Size <= 3)
		{
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"Text[1] : size is too small (%lx)",
				Size);
			return STATUS_BUFFER_TOO_SMALL;
		}

		auto OutBuffer = AllocateMemorySafeP<WCHAR>((Size - 5) + 2);
		if (!OutBuffer)
		{
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"Text[1] : failed allocate memory for decoding");
			return STATUS_NO_MEMORY;
		}

		OutBuffer.get()[0] = 0xfeff;

		CurrentBuffer = (WCHAR *)&Buffer[5];
		for (ULONG i = 0; i < (Size - 5) / 2; i++)
		{
			CurrentChar = CurrentBuffer[i];
			CurrentChar = ((CurrentChar & 0xaaaaaaaa) >> 1) | ((CurrentChar & 0x55555555) << 1);
			OutBuffer.get()[i + 1] = CurrentChar;
		}

		Status = File.Create(FileName);
		if (NT_FAILED(Status))
			return Status;

		File.Write(OutBuffer.get(), (Size - 5) + 2);
		File.Close();
	}
	break;

	case 0:
	{
		if (Size <= 3)
		{
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"Text[0] : size is too small (%lx)",
				Size
			);
			return STATUS_BUFFER_TOO_SMALL;
		}

		auto OutBuffer = AllocateMemorySafeP<WCHAR>((Size - 5) + 2);
		if (!OutBuffer)
		{
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_WARN,
				L"Text[0] : failed allocate memory for decoding"
			);
			return STATUS_NO_MEMORY;
		}
		
		OutBuffer.get()[0] = 0xfeff;

		CurrentBuffer = (WCHAR *)&Buffer[5];
		for (ULONG i = 0; i < Size - 5; i++)
		{
			CurrentChar = CurrentBuffer[i];
			if (CurrentChar >= 0x20) {
				OutBuffer.get()[i + 1] = CurrentChar ^ (((CurrentChar & 0xfe) << 8) ^ 1);
			}
		}
 
		Status = File.Open(FileName);
		if (NT_FAILED(Status))
			return Status;

		File.Write(OutBuffer.get(), (Size - 5) + 2);
		File.Close();
	}
	break;
	
	default:
		Proxyer->TellServerLogOutput(LogLevel::LOG_WARN, L"Text : Not supported crypto mode : %d", CryptMode);
		return STATUS_NOT_SUPPORTED;
	}

	return STATUS_SUCCESS;
}



