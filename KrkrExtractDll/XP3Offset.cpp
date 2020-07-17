#include "XP3Offset.h"

HRESULT WINAPI FindEmbededXp3OffsetSlow(const WCHAR* FileName, PLARGE_INTEGER Offset)
{
	HRESULT         Status;
	BYTE            Buffer[0x10000], *Xp3Signature;
	LARGE_INTEGER   BytesRead;
	WinFile         file;

	BytesRead.QuadPart = 0;

	BYTE Mark[] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER XP3Header = { Mark, (ULONG64)0 };

	Status = file.Open(FileName, WinFile::FileRead);
	
	Status = file.Seek(0, FILE_BEGIN);
	if (!KERNEL32_SUCCESS(Status))
	{
		file.Release();
		return Status;
	}

	Status = file.Read(Buffer, sizeof(IMAGE_DOS_HEADER));
	if (!KERNEL32_SUCCESS(Status))
	{
		file.Release();
		return Status;
	}

	if (((PIMAGE_DOS_HEADER)Buffer)->e_magic != IMAGE_DOS_SIGNATURE)
	{
		file.Release();
		return S_FALSE;
	}

	Status = file.Seek(0x10, FILE_BEGIN);
	if (!KERNEL32_SUCCESS(Status))
	{
		file.Release();
		return Status;
	}

	for (LONG64 FileSize = file.GetSize64(); FileSize > 0; FileSize -= sizeof(Buffer))
	{
		Status = file.Read(Buffer, sizeof(Buffer));
		if (!KERNEL32_SUCCESS(Status))
		{
			file.Release();
			return Status;
		}

		if (BytesRead.QuadPart < 0x10)
		{
			file.Release();
			return S_FALSE;
		}

		Xp3Signature = Buffer;
		for (ULONG_PTR Count = sizeof(Buffer) / 0x10; Count; Xp3Signature += 0x10, --Count)
		{
			if (sizeof(XP3Header.Magic) != RtlCompareMemory(Xp3Signature, XP3Header.Magic, sizeof(XP3Header.Magic)))
			{
				Offset->QuadPart = file.GetCurrentPos64() - sizeof(Buffer)+(Xp3Signature - Buffer);
				file.Release();
				return S_OK;
			}
		}
	}

	file.Release();
	return S_FALSE;
}
