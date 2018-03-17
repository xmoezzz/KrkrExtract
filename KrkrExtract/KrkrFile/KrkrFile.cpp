#include <Windows.h>
#include <string>
#include <map>
#include <stdio.h>

PVOID NTAPI AllocateMemoryRoutine(ULONG_PTR Size);
VOID  NTAPI FreeMemoryRoutine(PVOID Mem);

decltype(&AllocateMemoryRoutine) XmoeAllocateMemory = NULL;
decltype(&FreeMemoryRoutine)     XmoeFreeMemory     = NULL;


typedef struct FileInfoChunk
{
	std::wstring FileName;
	ULONG        Offset;
	ULONG64      Hash;
	ULONG        Size;

	FileInfoChunk& operator = (const FileInfoChunk& o)
	{
		FileName = o.FileName;
		Offset   = o.Offset;
		Size     = o.Size;
		Hash     = o.Hash;

		return *this;
	}


	friend bool operator >(const FileInfoChunk& lhs, const FileInfoChunk& rhs)
	{
		return lhs.Hash > rhs.Hash;
	}

	friend bool operator >=(const FileInfoChunk& lhs, const FileInfoChunk& rhs)
	{
		return lhs.Hash >= rhs.Hash;
	}

	friend bool operator <(const FileInfoChunk& lhs, const FileInfoChunk& rhs)
	{
		return lhs.Hash < rhs.Hash;
	}

	friend bool operator <=(const FileInfoChunk& lhs, const FileInfoChunk& rhs)
	{
		return lhs.Hash <= rhs.Hash;
	}

	friend bool operator ==(const FileInfoChunk& lhs, const FileInfoChunk& rhs)
	{
		return lhs.Hash == rhs.Hash;
	}

	friend bool operator != (const FileInfoChunk& lhs, const FileInfoChunk& rhs)
	{
		return lhs.Hash != rhs.Hash;
	}
}FileInfoChunk;

std::map<std::wstring, FileInfoChunk> FileHashMap;
FILE*            File; //use CreateFileW instead

std::wstring FileNameToLower(LPCWSTR FileName)
{
	std::wstring Result;

	for (ULONG i = 0; i < wcslen(FileName); i++)
	{
		if (FileName[i] <= 'Z' && FileName[i] >= 'A')
			Result += towlower(FileName[i]);
		else
			Result += FileName[i];
	}
	return Result;
}

#pragma comment(linker, "/EXPORT:XmoeInitFileSystem=_XmoeInitFileSystem@8,PRIVATE")
extern "C" __declspec(dllexport)
NTSTATUS NTAPI XmoeInitFileSystem(decltype(&AllocateMemoryRoutine) AllocateRoutine, decltype(&FreeMemoryRoutine) FreeRoutine)
{
	DWORD            ChunkOffset, FileSize, ChunkSize, iPos;
	PBYTE            ChunkData;
	FileInfoChunk    Info;

	if (!AllocateRoutine || !FreeRoutine)
		return STATUS_INVALID_PARAMETER;

	XmoeAllocateMemory = AllocateRoutine;
	XmoeFreeMemory     = FreeRoutine;

	File = _wfopen(L"Game.bin", L"rb");
	if (!File)
		return STATUS_INVALID_HANDLE;

	fseek(File, 0, SEEK_END);
	FileSize = ftell(File);
	rewind(File);
	fread(&ChunkOffset, 1, 4, File);
	fseek(File, ChunkOffset, SEEK_SET);

	ChunkSize = FileSize - ChunkOffset;
	ChunkData = (PBYTE)AllocateRoutine(ChunkSize);
	if (!ChunkData)
	{
		fclose(File);
		return STATUS_NO_MEMORY;
	}

	fread(ChunkData, 1, ChunkSize, File);

	iPos = 0;
	while (iPos < ChunkSize)
	{
		std::wstring FileName((LPCWSTR)(ChunkData + iPos));

		Info.FileName = FileName;
		Info.Offset = *(PDWORD)(ChunkData + iPos);
		iPos += 4;
		Info.Size = *(PDWORD)(ChunkData + iPos);
		iPos += 4;
		Info.Hash = *(PULONG64)(ChunkData + iPos);
		iPos += 8;

		std::wstring FileNameLower = FileNameToLower(Info.FileName.c_str());

		FileHashMap.insert(std::make_pair(FileNameLower, Info));
	}
	FreeRoutine(ChunkData);
	return 0;
}

#pragma comment(linker, "/EXPORT:XmoeQueryFile=_XmoeQueryFile@12,PRIVATE")
extern "C" __declspec(dllexport)
NTSTATUS NTAPI XmoeQueryFile(LPCWSTR FileName, PBYTE* FileBuffer, ULONG* FileSize)
{
	if (!FileName || !FileBuffer || !FileSize)
		return STATUS_INVALID_PARAMETER;

	*FileBuffer = NULL;
	*FileSize   = 0;

	auto Index = FileHashMap.find(FileNameToLower(FileName));
	if (Index == FileHashMap.end())
		return STATUS_INVALID_PARAMETER;

	fseek(File, Index->second.Offset, SEEK_SET);
	*FileBuffer = (PBYTE)XmoeAllocateMemory(Index->second.Size);
	if (!*FileBuffer)
		return STATUS_NO_MEMORY;

	*FileSize = Index->second.Size;
	fread(*FileBuffer, 1, Index->second.Size, File);

	return 0;
}



BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	switch (Reason)
	{
	case DLL_PROCESS_DETACH:
		if (File) fclose(File);
		break;
	}
	return TRUE;
}

