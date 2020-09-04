#pragma once

#include "KrkrHeaders.h"
#include "SafeMemory.h"
#include "Helper.h"
#include <memory>

struct Xp3ItemGeneral
{
	DWORD                  Magic;
	LARGE_INTEGER          ChunkSize;
	std::shared_ptr<BYTE>  Buffer;

	Xp3ItemGeneral()
	{
		Buffer = NULL;
		ChunkSize.QuadPart = 0;
	}

	~Xp3ItemGeneral()
	{
		ChunkSize.QuadPart = 0;
	}

	Xp3ItemGeneral& operator = (const Xp3ItemGeneral& Other)
	{
		this->Magic     = Other.Magic;
		this->ChunkSize = Other.ChunkSize;
		this->Buffer    = Other.Buffer;

		return *this;
	}

	NTSTATUS Dump()
	{
		NTSTATUS   Status;
		NtFileDisk File;
		wstring    HashName;

		if (!Buffer || ChunkSize.QuadPart == 0)
			return STATUS_UNSUCCESSFUL;

		GenMD5Code(Buffer.get(), ChunkSize.LowPart + sizeof(Magic) + sizeof(ChunkSize), HashName);

		HashName += L"_krkr.index";
		Status = File.Create(HashName.c_str());
		if (NT_FAILED(Status))
			return Status;

		File.Write(Buffer.get(), ChunkSize.LowPart + sizeof(Magic) + sizeof(ChunkSize));
		return File.Close();
	}
};


