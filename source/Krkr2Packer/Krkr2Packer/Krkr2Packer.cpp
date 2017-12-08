// M2Packer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <string>
#include <fstream>
#include <vector>
#include "Common.h"
#include "zlib\zlib.h"

#pragma comment(lib,"zlib.lib")

using std::wstring;
using std::fstream;
using std::string;
using std::vector;

void ErrorExit(const wchar_t* Info)
{
	MessageBoxW(NULL, Info, L"Krkr2Packer : Error", MB_OK);
	ExitProcess(-1);
}

void Error(const wchar_t* Info)
{
	MessageBoxW(NULL, Info, L"Krkr2Packer : Error", MB_OK);
}

vector<wstring> Info;

void WINAPI PackFiles(wstring& BasePath);

//exe config path
int wmain(int argc, wchar_t* argv[])
{
	SetConsoleTitleW(L"Krkr2Packer");
	if (argc != 3)
	{
		MessageBoxW(NULL, L"exe config path", L"Krkr2Packer", MB_OK);
		wprintf(L"Invaild Param [ParamCount : %d]\n", argc);
		return -1;
	}

	char szConfigName[MAX_PATH] = { 0 };
	WideCharToMultiByte(CP_ACP, 0, argv[1], lstrlenW(argv[1]), szConfigName, MAX_PATH, NULL, NULL);
	fstream fin(szConfigName);
	if (!fin)
	{
		ErrorExit(L"Couldn't open config file!");
	}
	string ReadLine;
	while (getline(fin, ReadLine))
	{
		if (ReadLine.length() == 0)
		{
			continue;
		}
		else
		{
			WCHAR StringInfo[1024] = { 0 };
			MultiByteToWideChar(CP_UTF8, 0, ReadLine.c_str(), ReadLine.length(), StringInfo, 1024);
			Info.push_back(wstring(StringInfo));
			ReadLine.clear();
		}
	}
	fin.close();

	wstring WideBasePath(argv[2]);

	PackFiles(WideBasePath);
	return 0;
}


void WINAPI PackFiles(wstring& BasePath)
{
	BOOL                    Result;
	HANDLE                  hHeap, hFile, hFileXP3;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize, BytesTransfered;
	WCHAR                   szPath[MAX_PATH];
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset;
	SMyXP3Index             *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	KRKR2_XP3_HEADER        XP3Header = { { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 } };


	static WCHAR* pszOutput = L"Krkr2Packed.xp3";
	hFileXP3 = CreateFileW(pszOutput,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFileXP3 == INVALID_HANDLE_VALUE)
	{
		ErrorExit(L"Couldn't open a handle for output file.");
	}

	hHeap = GetProcessHeap();
	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(hHeap, 0, BufferSize);
	lpCompressBuffer = HeapAlloc(hHeap, 0, CompressedSize);
	pXP3Index = (SMyXP3Index *)HeapAlloc(hHeap, 0, sizeof(*pXP3Index) * Info.size());
	pIndex = pXP3Index;

	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	Offset.QuadPart = BytesTransfered;

	const PWCHAR FormatString = L"Processing %d/%d [%s]";
	for (ULONG i = 0; i < Info.size(); ++pIndex, i++)
	{

		WCHAR TitleBuffer[512] = { 0 };
		wsprintfW(TitleBuffer, FormatString, i + 1, Info.size(), Info[i].c_str());
		SetConsoleTitleW(TitleBuffer);

		ZeroMemory(pIndex, sizeof(*pIndex));
		CopyMemory(pIndex->file.Magic, CHUNK_MAGIC_FILE, 4);
		CopyMemory(pIndex->info.Magic, CHUNK_MAGIC_INFO, 4);
		CopyMemory(pIndex->time.Magic, CHUNK_MAGIC_TIME, 4);
		CopyMemory(pIndex->segm.Magic, CHUNK_MAGIC_SEGM, 4);
		CopyMemory(pIndex->adlr.Magic, CHUNK_MAGIC_ADLR, 4);
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);

		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;
		pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring FullName = BasePath + L"\\";
		FullName += Info[i];
		CreateDirectoryW(FullName.c_str(), NULL);
		hFile = CreateFileW(FullName.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			wprintf(L"Couldn't open %s\n", Info[i].c_str());
		}

		GetFileSizeEx(hFile, &Size);
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(hHeap, 0, lpBuffer, BufferSize);
		}

		Result = ReadFile(hFile, lpBuffer, Size.LowPart, &BytesTransfered, NULL);
		CloseHandle(hFile);
		if (!Result || BytesTransfered != Size.LowPart)
		{
			wprintf(L"Couldn't open %s\n", Info[i].c_str());
			continue;
		}

		pIndex->segm.segm->Offset = Offset;
		pIndex->info.FileName = Info[i];
		pIndex->info.FileNameLength = Info[i].length();

		pIndex->adlr.Hash = 0;
		pIndex->adlr.Hash = adler32(1/*adler32(0, 0, 0)*/, (Bytef *)lpBuffer, BytesTransfered);

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered;
		pIndex->info.OriginalSize.LowPart = BytesTransfered;

		FILETIME Time1, Time2;
		GetFileTime(hFile, &(pIndex->time.FileTime), &Time1, &Time2);

		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;
		//DecryptWorker(EncryptOffset, lpBuffer, BytesTransfered, pIndex->adlr.Hash);

		//pIndex->info.EncryptedFlag = 0x80000000;
		pIndex->info.EncryptedFlag = 0x00000000;

		
		pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
		sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1)*2 +
		sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
		sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		

		//compress
		if (false)
		{
			if (Size.LowPart > CompressedSize)
			{
				CompressedSize = Size.LowPart;
				lpCompressBuffer = HeapReAlloc(hHeap, 0, lpCompressBuffer, CompressedSize);
			}
			if (Size.LowPart * 2 > BufferSize)
			{
				BufferSize = Size.LowPart * 2;
				lpBuffer = HeapReAlloc(hHeap, 0, lpBuffer, BufferSize);
			}

			pIndex->segm.segm->bZlib = 1;
			CopyMemory(lpCompressBuffer, lpBuffer, Size.LowPart);
			BytesTransfered = BufferSize;
			compress2((PBYTE)lpBuffer, &BytesTransfered, (PBYTE)lpCompressBuffer, Size.LowPart, Z_BEST_COMPRESSION);
		}

		pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered;
		pIndex->info.ArchiveSize.LowPart = BytesTransfered;
		Offset.QuadPart += BytesTransfered;

		WriteFile(hFileXP3, lpBuffer, BytesTransfered, &BytesTransfered, NULL);
	}

	XP3Header.IndexOffset = Offset;

	// generate index, calculate index size first
	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < Info.size(); ++i, ++pIndex)
	{
		Size.LowPart += 
			sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->file.ChunkSize) + MagicLength +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash); //adlr
	}

	if (Size.LowPart > CompressedSize)
	{
		CompressedSize = Size.LowPart;
		lpCompressBuffer = HeapReAlloc(hHeap, 0, lpCompressBuffer, CompressedSize);
	}
	if (Size.LowPart * 2 > BufferSize)
	{
		BufferSize = Size.LowPart * 2;
		lpBuffer = HeapReAlloc(hHeap, 0, lpBuffer, BufferSize);
	}

	// generate index to lpCompressBuffer
	pIndex = pXP3Index;
	pbIndex = (PBYTE)lpCompressBuffer;
	for (ULONG i = 0; i < Info.size(); ++pIndex, i++)
	{
		ULONG n = sizeof(DWORD);
		CopyMemory(pbIndex, pIndex->file.Magic, n);
		pbIndex += n;
		n = 8;
		CopyMemory(pbIndex, &(pIndex->file.ChunkSize), n);
		pbIndex += n;

		n = sizeof(DWORD);
		CopyMemory(pbIndex, pIndex->time.Magic, n);
		pbIndex += n;
		n = 8;
		CopyMemory(pbIndex, &(pIndex->time.ChunkSize), n);
		pbIndex += n;
		n = sizeof(pIndex->time.FileTime);
		CopyMemory(pbIndex, &(pIndex->time.FileTime), n);
		pbIndex += n;

		n = sizeof(DWORD);
		CopyMemory(pbIndex, pIndex->adlr.Magic, n);
		pbIndex += n;
		n = 8;
		CopyMemory(pbIndex, &(pIndex->adlr.ChunkSize), n);
		pbIndex += n;
		n = 4;
		CopyMemory(pbIndex, &(pIndex->adlr.Hash), n);
		pbIndex += n;

		n = 4;
		CopyMemory(pbIndex, &(pIndex->segm.Magic), n);
		pbIndex += n;
		n = 8;
		CopyMemory(pbIndex, &(pIndex->segm.ChunkSize), n);
		pbIndex += n;
		n = 4;
		CopyMemory(pbIndex, &(pIndex->segm.segm[0].bZlib), n);
		pbIndex += n;
		n = 8;
		CopyMemory(pbIndex, &(pIndex->segm.segm[0].Offset), n);
		pbIndex += n;
		CopyMemory(pbIndex, &(pIndex->segm.segm[0].OriginalSize), n);
		pbIndex += n;
		CopyMemory(pbIndex, &(pIndex->segm.segm[0].ArchiveSize), n);
		pbIndex += n;

		n = 4;
		CopyMemory(pbIndex, &(pIndex->info.Magic), n);
		pbIndex += n;
		n = 8;
		CopyMemory(pbIndex, &(pIndex->info.ChunkSize), n);
		pbIndex += n;
		n = 4;
		CopyMemory(pbIndex, &(pIndex->info.EncryptedFlag), n);
		pbIndex += n;
		n = 8;
		CopyMemory(pbIndex, &(pIndex->info.OriginalSize), n);
		pbIndex += n;
		CopyMemory(pbIndex, &(pIndex->info.ArchiveSize), n);
		pbIndex += n;
		n = 2;
		CopyMemory(pbIndex, &(pIndex->info.FileNameLength), n);
		pbIndex += n;
		n = (pIndex->info.FileName.length() + 1) * 2;
		CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		pbIndex += n;
	}

	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = Size.LowPart;
	IndexHeader.ArchiveSize.LowPart = BufferSize;
	BufferSize = Size.LowPart;
	compress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
	IndexHeader.ArchiveSize.HighPart = 0;

	WriteFile(hFileXP3, &IndexHeader, sizeof(IndexHeader), &BytesTransfered, NULL);
	WriteFile(hFileXP3, lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered, NULL);
	Offset.QuadPart = 0;
	SetFilePointerEx(hFileXP3, Offset, NULL, FILE_BEGIN);
	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	CloseHandle(hFileXP3);

	SetConsoleTitleW(L"Krkr2Packer");

	HeapFree(hHeap, 0, lpBuffer);
	HeapFree(hHeap, 0, lpCompressBuffer);
	HeapFree(hHeap, 0, pXP3Index);
}
