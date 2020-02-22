#include "my.h"
#include "ml.h"
#include "KrkrPacker.h"
#include "XP3Parser.h"
#include "KrkrExtend.h"
#include "Adler32Stream.h"
#include "mt64.h"

static WCHAR* PackingFormatString = L"Packing[%d/%d]";

static KrkrPacker* LocalKrkrPacker = NULL;

wstring ToLowerString(LPCWSTR lpString)
{
	wstring Result;

	for (LONG_PTR i = 0; i < lstrlenW(lpString); i++)
		Result += (WCHAR)CHAR_LOWER(lpString[i]);
	
	return Result;
}


KrkrPacker::KrkrPacker() :
	KrkrPackType(PackInfo::UnknownPack),
	M2Hash(0),
	DecryptionKey(0),
	pfProc(NULL),
	XP3EncryptionFlag(TRUE),
	hThread((HANDLE)-1),
	ThreadId(0),
	InfoNameZeroEnd(TRUE),
	M2NameZeroEnd(TRUE)
{
	KrkrPackType = PackInfo::UnknownPack;
	M2Hash = 0x23333333;
	DecryptionKey = 0;
	pfProc = nullptr;
	XP3EncryptionFlag = TRUE;
	hThread = INVALID_HANDLE_VALUE;
	ThreadId = 0;
	InfoNameZeroEnd = TRUE;
	M2NameZeroEnd = TRUE;

	RtlZeroMemory(&SenrenBankaInfo, sizeof(SenrenBankaInfo));

	pfProc = GlobalData::GetGlobalData()->pfGlobalXP3Filter;

	if (GlobalData::GetGlobalData()->DebugOn)
		PrintConsoleW(L"Filter Addr : 0x%08x\n", pfProc);
}


VOID NTAPI KrkrPacker::Init()
{
	KrkrPackType = PackInfo::UnknownPack;
	DecryptionKey = 0;
	pfProc = GlobalData::GetGlobalData()->pfGlobalXP3Filter;
	XP3EncryptionFlag = TRUE;
	hThread = INVALID_HANDLE_VALUE;
	ThreadId = 0;
	InfoNameZeroEnd = TRUE;
	M2NameZeroEnd = TRUE;
	GlobalData::GetGlobalData()->CurrentTempFileName = L"KrkrzTempWorker.xp3";
	RtlZeroMemory(&SenrenBankaInfo, sizeof(SenrenBankaInfo));

	ThreadId = 0;
}

KrkrPacker::~KrkrPacker()
{
}


/////////////////////////////////////Common functions////////////////////////////////////////////

#pragma pack(1)
typedef struct _SYSTEM_HANDLE
{
	ULONG       ProcessId;
	BYTE        ObjectTypeNumber;
	BYTE        Flags;
	USHORT      Handle;
	PVOID       Object;
	ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, *PSYSTEM_HANDLE;
#pragma pack()


auto AppendUnicodeString(LPWSTR lpwsString, PUNICODE_STRING UnicodeString)->BOOL
{
	ULONG Size;

	if (!lpwsString || !UnicodeString)
		return FALSE;

	Size = lstrlenW(lpwsString);
	RtlCopyMemory(&lpwsString[Size], UnicodeString->Buffer, UnicodeString->Length * 2);
	return TRUE;
};


auto AppendString(LPWSTR lpwsString, LPCWSTR String)->BOOL
{
	ULONG Size;

	if (!lpwsString || !String)
		return FALSE;

	Size = lstrlenW(lpwsString);
	lstrcpyW(&lpwsString[Size], String);
	return TRUE;
};


//DON't use this function...
//f**k Windows, it's very hard to get handles in UserMode, because kernel will
//try to marshal everything we need then copy to UserLand
auto DisPlayProcessFileHandle(wstring& InfoList)->NTSTATUS
{
	NTSTATUS                       Status;
	POBJECT_TYPE_INFORMATION       ObjectTypeInfo;
	PSYSTEM_HANDLE_INFORMATION     HandleInfo;
	ULONG                          HandleInfoSize;
	SYSTEM_HANDLE_TABLE_ENTRY_INFO Handle;
	HANDLE                         DupHandle, ProcessHandle;
	WCHAR                          Buffer[0x1000];
	BYTE                           ObjectNameBuffer[0x1000];
	UNICODE_STRING                 FileTag;
	PUNICODE_STRING                ObjectNameInfo;
	ULONG                          ReturnLength;

	ProcessHandle  = OpenProcess(PROCESS_DUP_HANDLE, FALSE, GetCurrentProcessId());
	HandleInfoSize = 0x10000;
	HandleInfo     = (PSYSTEM_HANDLE_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, HandleInfoSize);

	while ((Status = NtQuerySystemInformation(
		SystemHandleInformation,
		HandleInfo,
		HandleInfoSize,
		NULL
		)) == STATUS_INFO_LENGTH_MISMATCH)
	{
		HandleInfo = (PSYSTEM_HANDLE_INFORMATION)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, HandleInfo, HandleInfoSize *= 2);
		if (HandleInfo == NULL)
			return STATUS_NO_MEMORY;
	}

	for (ULONG i = 0; i < HandleInfo->NumberOfHandles; i++)
	{
		Handle = HandleInfo->Handles[i];
		if (Handle.UniqueProcessId != GetCurrentProcessId())
			continue;

		DupHandle = 0;
		Status = NtDuplicateObject(ProcessHandle, (HANDLE)Handle.HandleValue, GetCurrentProcess(), &DupHandle, 0, 0, 0);
		if (NT_FAILED(Status))
		{
			PrintConsoleW(L"NtDuplicateObject failed (%08x)\n", Status);
			continue;
		}

		ObjectTypeInfo = (decltype(ObjectTypeInfo))HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x1000);
		Status = NtQueryObject(DupHandle, ObjectTypeInformation, (PVOID)ObjectTypeInfo, 0x1000, NULL);
		if (NT_FAILED(Status))
		{
			NtClose(DupHandle);
			PrintConsoleW(L"NtQueryObject failed (%08x)\n", Status);
			continue;
		}

		RtlInitUnicodeString(&FileTag, L"File");
		if (RtlCompareUnicodeString(&ObjectTypeInfo->TypeName, &FileTag, FALSE) != 0)
			continue;

		if (Handle.GrantedAccess == 0x0012019f)
		{
			wsprintfW(Buffer, L"[%08x] (none)\n", Handle.HandleValue);
			InfoList += Buffer;
			NtClose(DupHandle);
			continue;
		}

		Status = NtQueryObject(DupHandle, ObjectNameInformation, ObjectNameBuffer, 0x1000, &ReturnLength);
		if (NT_FAILED(Status))
		{
			wsprintfW(Buffer, L"[%08x] (none)\n", Handle.HandleValue);
			InfoList += Buffer;
			NtClose(DupHandle);
			continue;
		}

		ObjectNameInfo = (PUNICODE_STRING)ObjectNameBuffer;
		if (ObjectNameInfo->Length)
		{
			wsprintfW(Buffer, L"[%08x] ", Handle.HandleValue);
			AppendUnicodeString(Buffer, ObjectNameInfo);
			AppendString(Buffer, L"\n");
		}
		else
		{
			wsprintfW(Buffer, L"[%08x] (unnamed)\n", Handle.HandleValue);
		}

		InfoList += Buffer;
		NtClose(DupHandle);
		HeapFree(GetProcessHeap(), 0, ObjectTypeInfo);
		ObjectTypeInfo = NULL;
	}

	HeapFree(GetProcessHeap(), 0, HandleInfo);
	return STATUS_SUCCESS;
};


/////////////////////////////////////////////////////////////////////////////////////////////////
//                                     Packer
/////////////////////////////////////////////////////////////////////////////////////////////////
VOID WINAPI KrkrPacker::DecryptWorker(ULONG EncryptOffset, PBYTE pBuffer, ULONG BufferSize, ULONG Hash)
{
	tTVPXP3ExtractionFilterInfo Info(0, pBuffer, BufferSize, Hash);
	if (pfProc != nullptr)
	{
		pfProc(&Info);
	}
}

NTSTATUS NTAPI KrkrPacker::GetSenrenBankaPackInfo(PBYTE IndexData, ULONG IndexSize, NtFileDisk& file)
{
	NTSTATUS                Status;
	ULONG                   iPos;
	PBYTE                   CompressedBuffer, IndexBuffer;
	ULONG                   DecompSize, DecodeSize;
	BOOL                    RawFailed;
	GlobalData*             Handle;


	Handle = GlobalData::GetGlobalData();
	iPos   = 0;

	SenrenBankaInfo.Magic = *(PDWORD)(IndexData + iPos);
	iPos += 4;
	SenrenBankaInfo.ChunkSize.QuadPart = *(PULONG64)(IndexData + iPos);
	iPos += 8;
	SenrenBankaInfo.Offset.QuadPart = *(PULONG64)(IndexData + iPos);
	iPos += 8;
	SenrenBankaInfo.OriginalSize = *(PDWORD)(IndexData + iPos);
	iPos += 4;
	SenrenBankaInfo.ArchiveSize = *(PDWORD)(IndexData + iPos);
	iPos += 4;
	SenrenBankaInfo.LengthOfProduct = *(PUSHORT)(IndexData + iPos);
	iPos += 2;
	RtlZeroMemory(SenrenBankaInfo.ProductName, countof(SenrenBankaInfo.ProductName) * sizeof(WCHAR));
	RtlCopyMemory(SenrenBankaInfo.ProductName, (IndexData + iPos), SenrenBankaInfo.LengthOfProduct * sizeof(WCHAR));

	DecompSize       = SenrenBankaInfo.OriginalSize;
	IndexBuffer      = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SenrenBankaInfo.OriginalSize);
	CompressedBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SenrenBankaInfo.ArchiveSize);

	if (!IndexBuffer || !CompressedBuffer)
	{
		MessageBoxW(Handle->MainWindow, L"Insufficient memory", L"KrkrExtract", MB_OK);

		if (IndexBuffer)      HeapFree(GetProcessHeap(), 0, IndexBuffer);
		if (CompressedBuffer) HeapFree(GetProcessHeap(), 0, CompressedBuffer);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	file.Seek(SenrenBankaInfo.Offset, FILE_BEGIN);
	Status = file.Read(CompressedBuffer, SenrenBankaInfo.ArchiveSize);
	if (NT_FAILED(Status))
	{
		HeapFree(GetProcessHeap(), 0, IndexBuffer);
		HeapFree(GetProcessHeap(), 0, CompressedBuffer);

		MessageBoxW(Handle->MainWindow, L"Couldn't decompress the special block", L"KrkrExtract", MB_OK | MB_ICONERROR);
		return Status;
	}


	RawFailed = FALSE;
	if (!Handle->IsSpcialChunkEncrypted)
	{
		if ((DecompSize = uncompress((PBYTE)IndexBuffer, (PULONG)&DecompSize,
			(PBYTE)CompressedBuffer, SenrenBankaInfo.ArchiveSize)) != Z_OK)
		{
			RawFailed = TRUE;
		}
	}
	else
	{
		if (Handle->SpecialChunkDecoder)
		{
			DecodeSize = 0x100;
			if (DecodeSize < 0x100)
				DecodeSize = SenrenBankaInfo.ArchiveSize;

			Handle->SpecialChunkDecoder(CompressedBuffer, CompressedBuffer, DecodeSize);
			DecompSize = SenrenBankaInfo.OriginalSize;
			if ((DecompSize = uncompress((PBYTE)IndexBuffer, (PULONG)&DecompSize,
				(PBYTE)CompressedBuffer, SenrenBankaInfo.ArchiveSize)) != Z_OK)
			{
				RawFailed = TRUE;
			}
		}
		else
		{
			RawFailed = TRUE;
		}
	}

	if (RawFailed)
	{
		if (Handle->DebugOn)
			PrintConsoleW(L"Failed to gather information(at Compressed block)\n");

		MessageBoxW(Handle->MainWindow, L"Failed to decompress special chunk", L"KrkrExtract", MB_OK);
		return STATUS_DATA_ERROR;
	}

	M2SubChunkMagic = *(PDWORD)IndexBuffer;

	HeapFree(GetProcessHeap(), 0, IndexBuffer);
	HeapFree(GetProcessHeap(), 0, CompressedBuffer);

	return STATUS_SUCCESS;
}


NTSTATUS NTAPI KrkrPacker::DetactPackFormat(LPCWSTR lpFileName)
{
	NTSTATUS                Status;
	ULONG                   Count;
	GlobalData*             Handle;
	NtFileDisk              File;
	KRKR2_XP3_HEADER        XP3Header;
	KRKR2_XP3_DATA_HEADER   DataHeader;
	LARGE_INTEGER           BeginOffset;
	ULONG                   OldHash, NewHash;
	IStream*                Stream;
	STATSTG                 Stat;

	Count = 0;
	Handle = GlobalData::GetGlobalData();

	Status = File.Open(lpFileName);
	if (NT_FAILED(Status))
	{
		wstring Info(L"Couldn't open : \n");
		Info += lpFileName;
		Info += L"\nFor Guessing XP3 Package Type!";
		MessageBoxW(Handle->MainWindow, Info.c_str(), L"KrkrExtract", MB_OK);
		return Status;
	}

	BeginOffset.QuadPart = 0;

	File.Read((PBYTE)(&XP3Header), sizeof(XP3Header));


	if ((*(PUSHORT)XP3Header.Magic) == IMAGE_DOS_SIGNATURE)
	{
		Status = FindEmbededXp3OffsetSlow(File, &BeginOffset);

		if (NT_FAILED(Status))
		{
			MessageBoxW(Handle->MainWindow, L"No a Built-in Package\n", L"KrkrExtract", MB_OK);
			File.Close();
			return Status;
		}

		File.Seek(BeginOffset, FILE_BEGIN);
		File.Read((PBYTE)(&XP3Header), sizeof(XP3Header));
	}
	else
	{
		BeginOffset.QuadPart = 0;
	}

	if (RtlCompareMemory(StaticXP3V2Magic, XP3Header.Magic, sizeof(StaticXP3V2Magic)) != sizeof(StaticXP3V2Magic))
	{
		MessageBoxW(Handle->MainWindow, L"No a XP3 Package!", L"KrkrExtract", MB_OK);
		File.Close();
		return STATUS_INVALID_PARAMETER;
	}

	ULONG64 CompresseBufferSize = 0x1000;
	ULONG64 DecompressBufferSize = 0x1000;
	PBYTE pCompress = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (ULONG)CompresseBufferSize);
	PBYTE pDecompress = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (ULONG)DecompressBufferSize);
	DataHeader.OriginalSize = XP3Header.IndexOffset;


	ULONG Result = PackInfo::UnknownPack;
	do
	{
		LARGE_INTEGER Offset;

		Offset.QuadPart = DataHeader.OriginalSize.QuadPart + BeginOffset.QuadPart;
		File.Seek(Offset.LowPart, FILE_BEGIN);
		Status = File.Read((PBYTE)(&DataHeader), sizeof(DataHeader));
		if (NT_FAILED(Status))
		{
			MessageBoxW(Handle->MainWindow, L"Couldn't Read Index Header", L"KrkrExtract", MB_OK);
			HeapFree(GetProcessHeap(), 0, pCompress);
			HeapFree(GetProcessHeap(), 0, pDecompress);
			File.Close();
			return STATUS_UNSUCCESSFUL;
		}

		if (DataHeader.ArchiveSize.HighPart != 0 || DataHeader.ArchiveSize.LowPart == 0)
			continue;

		if (DataHeader.ArchiveSize.LowPart > CompresseBufferSize)
		{
			CompresseBufferSize = DataHeader.ArchiveSize.LowPart;
			pCompress = (PBYTE)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pCompress, (ULONG)CompresseBufferSize);
		}

		if ((DataHeader.bZlib & 7) == 0)
		{
			Offset.QuadPart = -8;
			File.Seek(Offset, FILE_CURRENT);
		}

		File.Read(pCompress, DataHeader.ArchiveSize.LowPart);
		BOOL EncodeMark = DataHeader.bZlib & 7;

		if (EncodeMark == FALSE)
		{
			if (DataHeader.ArchiveSize.LowPart > DecompressBufferSize)
			{
				DecompressBufferSize = DataHeader.ArchiveSize.LowPart;
				pDecompress = (PBYTE)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pDecompress, (ULONG)DecompressBufferSize);
			}
			CopyMemory(pDecompress, pCompress, DataHeader.ArchiveSize.LowPart);
			DataHeader.OriginalSize.LowPart = DataHeader.ArchiveSize.LowPart;
		}
		else
		{
			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleW(L"Index : Zlib Data\n");

			if (DataHeader.OriginalSize.LowPart > DecompressBufferSize)
			{
				DecompressBufferSize = DataHeader.OriginalSize.LowPart;
				pDecompress = (PBYTE)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pDecompress, (ULONG)DecompressBufferSize);
			}

			DataHeader.OriginalSize.HighPart = DataHeader.OriginalSize.LowPart;
			if (uncompress((PBYTE)pDecompress, (PULONG)&DataHeader.OriginalSize.HighPart,
				(PBYTE)pCompress, DataHeader.ArchiveSize.LowPart) == Z_OK)
			{
				DataHeader.OriginalSize.LowPart = DataHeader.OriginalSize.HighPart;
			}
			else
			{
				return STATUS_UNSUCCESSFUL;
			}
		}

		if (IsCompatXP3(pDecompress, DataHeader.OriginalSize.LowPart, &Handle->M2ChunkMagic))
		{
			KrkrPackType = FindChunkMagicFirst(pDecompress, DataHeader.OriginalSize.LowPart);

			switch (DetectCompressedChunk(pDecompress, DataHeader.OriginalSize.LowPart))
			{
			case TRUE:
				Result = InitIndexFileFirst(pDecompress, DataHeader.OriginalSize.LowPart);
				if (Handle->pfGlobalXP3Filter)
					KrkrPackType = PackInfo::NormalPack;
				break;

			case FALSE:
				Result = InitIndexFile_SenrenBanka(pDecompress, DataHeader.OriginalSize.LowPart, File);
				GetSenrenBankaPackInfo(pDecompress, DataHeader.OriginalSize.LowPart, File);
				KrkrPackType = PackInfo::KrkrZ_SenrenBanka;
				break;
			}
		}
		else
		{
			Result = InitIndex_NekoVol0(pDecompress, DataHeader.OriginalSize.LowPart);
			KrkrPackType = PackInfo::KrkrZ_V2;
		}

		if (KrkrPackType == PackInfo::UnknownPack)
		{
			HeapFree(GetProcessHeap(), 0, pCompress);
			HeapFree(GetProcessHeap(), 0, pDecompress);
			File.Close();

			MessageBoxW(Handle->MainWindow, L"Unknown Pack Type", L"KrkrExtract", MB_OK | MB_ICONERROR);
			return STATUS_UNSUCCESSFUL;
		}

	} while (DataHeader.bZlib & 0x80);


	PrintConsoleW(L"Packer Type %d\n", KrkrPackType);

	//
	if (KrkrPackType == PackInfo::NormalPack && Handle->pfGlobalXP3Filter == NULL)
	{
		Handle->ItemVector[Handle->ItemVector.size() / 2].info.FileName;
		Stream = TVPCreateIStream(ttstr(Handle->ItemVector[Handle->ItemVector.size() / 2].info.FileName.c_str()), TJS_BS_READ);
		OldHash = Handle->ItemVector[Handle->ItemVector.size() / 2].adlr.Hash;
		if (Stream)
		{
			Stream->Stat(&Stat, STATFLAG_DEFAULT);
			NewHash = adler32IStream(0, Stream, Stat.cbSize);

			if (OldHash != NewHash)
				KrkrPackType = PackInfo::NormalPack_NoExporter;
		}
	}

	HeapFree(GetProcessHeap(), 0, pCompress);
	HeapFree(GetProcessHeap(), 0, pDecompress);
	File.Close();

	return STATUS_SUCCESS;;
}


VOID WINAPI KrkrPacker::InternalReset()
{
	GlobalData*   Handle;

	Handle = GlobalData::GetGlobalData();

	RtlZeroMemory(&SenrenBankaInfo, sizeof(SenrenBankaInfo));

	PackChunkList.clear();
	M2ChunkList.clear();
	FileList.clear();
	Handle->Reset();
}


NTSTATUS WINAPI KrkrPacker::DoNormalPack(LPCWSTR lpBasePack, LPCWSTR lpGuessPack, LPCWSTR OutName)
{
	NTSTATUS                Status;
	NtFileDisk              File, FileXP3;
	GlobalData*             Handle;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset, BytesTransfered;
	SMyXP3IndexNormal       *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, 0);

	FileList.clear();
	IterFiles(lpBasePack);

	Handle = GlobalData::GetGlobalData();

	Status = FileXP3.Create(OutName);
	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Couldn't open a handle for temporary output file.", L"KrkrExtract", MB_OK);
		return Status;
	}

	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
	lpCompressBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedSize);
	pXP3Index = (SMyXP3IndexNormal *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	if (!lpBuffer || !lpCompressBuffer || !pXP3Index)
	{
		MessageBoxW(Handle->MainWindow, L"Insufficient memory to make package!!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		FileXP3.Close();
		
		if (lpBuffer)         HeapFree(GetProcessHeap(), 0, lpBuffer);
		if (lpCompressBuffer) HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		if (pXP3Index)        HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	Offset.QuadPart = BytesTransfered.QuadPart;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		WCHAR OutInfo[MAX_PATH];
		wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
		SetWindowTextW(Handle->MainWindow, OutInfo);
		Handle->SetProcess(Handle->MainWindow, (ULONG)(((float)(i + 1) / (float)FileList.size()) * 100.0), i, FileList.size());

		ZeroMemory(pIndex, sizeof(*pIndex));
		*(PDWORD)(pIndex->file.Magic) = CHUNK_MAGIC_FILE;
		*(PDWORD)(pIndex->info.Magic) = CHUNK_MAGIC_INFO;
		*(PDWORD)(pIndex->time.Magic) = CHUNK_MAGIC_TIME;
		*(PDWORD)(pIndex->segm.Magic) = CHUNK_MAGIC_SEGM;
		*(PDWORD)(pIndex->adlr.Magic) = CHUNK_MAGIC_ADLR;
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		//
		pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring FullName = lpBasePack;
		FullName += L"\\";
		FullName += FileList[i];

		Status = File.Open(FullName.c_str());
		if (NT_FAILED(Status))
		{
			if (Handle->DebugOn)
				PrintConsoleW(L"Couldn't open %s\n", FullName.c_str());
		}

		File.GetSize(&Size);
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
		}

		File.Read(lpBuffer, Size.LowPart, &BytesTransfered);
		
		if (BytesTransfered.LowPart != Size.LowPart)
		{
			wstring InfoW(L"Dummy write :\n[Failed to read]\n");
			InfoW += FullName;
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			File.Close();
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_IO_DEVICE_ERROR;
		}

		pIndex->segm.segm->Offset = Offset;

		pIndex->info.FileName = FileList[i];
		pIndex->info.FileNameLength = (USHORT)FileList[i].size();

		pIndex->adlr.Hash = adler32(1, (Bytef *)lpBuffer, BytesTransfered.LowPart);

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.OriginalSize.LowPart = BytesTransfered.LowPart;

		FILETIME Time1, Time2;
		GetFileTime(File.GetHandle(), &(pIndex->time.FileTime), &Time1, &Time2);
		File.Close();


		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;
		DecryptWorker(EncryptOffset.LowPart, (PBYTE)lpBuffer, BytesTransfered.LowPart, pIndex->adlr.Hash);
		if (XP3EncryptionFlag && pfProc)
			pIndex->info.EncryptedFlag = 0x80000000;
		else
			pIndex->info.EncryptedFlag = 0;

		pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if (!InfoNameZeroEnd)
			pIndex->file.ChunkSize.QuadPart -= 2;

		pIndex->segm.segm->bZlib = 0;

		pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.ArchiveSize.LowPart = BytesTransfered.LowPart;
		Offset.QuadPart += BytesTransfered.QuadPart;

		FileXP3.Write(lpBuffer, BytesTransfered.LowPart, &BytesTransfered);
	}

	XP3Header.IndexOffset = Offset;

	// generate index, calculate index size first
	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		Size.LowPart +=
			sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->file.ChunkSize) + MagicLength +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if (!InfoNameZeroEnd)
			Size.LowPart -= 2;
	}

	if (Size.LowPart > CompressedSize)
	{
		CompressedSize = Size.LowPart;
		lpCompressBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpCompressBuffer, CompressedSize);
	}
	if (Size.LowPart * 2 > BufferSize)
	{
		BufferSize = Size.LowPart * 2;
		lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
	}

	// generate index to lpCompressBuffer
	pIndex = pXP3Index;
	pbIndex = (PBYTE)lpCompressBuffer;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		DWORD n = sizeof(DWORD);
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

		if (InfoNameZeroEnd)
		{
			n = (pIndex->info.FileName.length() + 1) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		else
		{
			n = (pIndex->info.FileName.length()) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		pbIndex += n;
	}

	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = Size.LowPart;
	IndexHeader.ArchiveSize.LowPart = BufferSize;
	BufferSize = Size.LowPart;
	compress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
	IndexHeader.ArchiveSize.HighPart = 0;

	FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
	FileXP3.Write(lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	Offset.QuadPart = 0;
	FileXP3.Seek(Offset, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	FileXP3.Close();

	HeapFree(GetProcessHeap(), 0, lpBuffer);
	HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
	HeapFree(GetProcessHeap(), 0, pXP3Index);

	MessageBoxW(Handle->MainWindow, L"Making Package : Successful", L"KrkrExtract", MB_OK);
	return STATUS_SUCCESS;
}


NTSTATUS NTAPI KrkrPacker::DoNormalPackEx(LPCWSTR lpBasePack, LPCWSTR GuessPackage, LPCWSTR OutName)
{
	NTSTATUS                Status;
	NtFileDisk              File, FileXP3;
	GlobalData*             Handle;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize, StrLen;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset, BytesTransfered;
	SMyXP3IndexNormal       *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, 0);
	tTJSVariant             ExecResult;

	FileList.clear();
	IterFiles(lpBasePack);

	Handle = GlobalData::GetGlobalData();

	Status = DoDummyNormalPackExFirst(lpBasePack);
	if (NT_FAILED(Status))
		return Status;

	if (Handle->DebugOn)
		PrintConsoleW(L"Packing files...\n");

	TVPExecuteScript(ttstr(L"Storages.addAutoPath(System.exePath + \"" + ttstr(Handle->CurrentTempFileName.c_str()) + L"\" + \">\");"), &ExecResult);
	
#if 0
	if (ExecResult.AsInteger() == FALSE)
	{
		MessageBoxW(Handle->MainWindow, L"Script exec error.(add)", L"KrkrExtract", MB_OK);
		return STATUS_UNSUCCESSFUL;
	}
#endif

	Status = FileXP3.Create(OutName);
	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Couldn't create a handle for output file.", L"KrkrExtract", MB_OK);
		return Status;
	}

	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
	lpCompressBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedSize);
	pXP3Index = (SMyXP3IndexNormal *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	if (!lpBuffer || !lpCompressBuffer || !pXP3Index)
	{
		MessageBoxW(Handle->MainWindow, L"Insufficient memory to make package!!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		FileXP3.Close();

		if (lpBuffer)         HeapFree(GetProcessHeap(), 0, lpBuffer);
		if (lpCompressBuffer) HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		if (pXP3Index)        HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	Offset.QuadPart = BytesTransfered.QuadPart;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		WCHAR OutInfo[MAX_PATH];
		wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
		SetWindowTextW(Handle->MainWindow, OutInfo);
		Handle->SetProcess(Handle->MainWindow, (ULONG)(((float)(i + 1) / (float)FileList.size()) * 100.0), i, FileList.size());

		ZeroMemory(pIndex, sizeof(*pIndex));
		*(PDWORD)(pIndex->file.Magic) = CHUNK_MAGIC_FILE;
		*(PDWORD)(pIndex->info.Magic) = CHUNK_MAGIC_INFO;
		*(PDWORD)(pIndex->time.Magic) = CHUNK_MAGIC_TIME;
		*(PDWORD)(pIndex->segm.Magic) = CHUNK_MAGIC_SEGM;
		*(PDWORD)(pIndex->adlr.Magic) = CHUNK_MAGIC_ADLR;
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		//
		pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring DummyName = FileList[i] + L".dummy";

		ttstr FullName(L"file://./");

		StrLen = lstrlenW(lpBasePack);

		for (ULONG Index = 0; Index < StrLen; Index++)
		{
			if (lpBasePack[Index] == L'\\' || lpBasePack[Index] == L'/')
				FullName += L"/";
			else if (lpBasePack[Index] == L':')
			{
				i++;
				FullName += CHAR_LOWER(lpBasePack[Index]);
			}
			else
				FullName += lpBasePack[Index];
		}

		if (lpBasePack[StrLen - 1] != L'\\' && lpBasePack[StrLen - 1] != L'/')
			FullName += L"/";

		FullName += Handle->CurrentTempFileName.c_str();
		FullName += L"/";

		FullName += DummyName.c_str();

		IStream* Stream = TVPCreateIStream(FullName, TJS_BS_READ);
		if (Stream == NULL)
		{
			if (Handle->DebugOn)
				PrintConsoleW(L"Couldn't open %s\n", DummyName);

			wstring InfoW(L"Couldn't open :\n");
			InfoW += FileList[i];
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_UNSUCCESSFUL;
		}


		STATSTG t;
		Stream->Stat(&t, STATFLAG_DEFAULT);

		Size.QuadPart = t.cbSize.QuadPart;
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
		}
		BytesTransfered.QuadPart = 0;
		Stream->Read(lpBuffer, Size.LowPart, &BytesTransfered.LowPart);

		if (BytesTransfered.LowPart != Size.LowPart)
		{
			wstring InfoW(L"Couldn't read :\n");
			InfoW += FileList[i];
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_UNSUCCESSFUL;
		}

		pIndex->segm.segm->Offset = Offset;

		pIndex->info.FileName = FileList[i];
		pIndex->info.FileNameLength = (USHORT)FileList[i].length();

		pIndex->adlr.Hash = adler32(1, (Bytef *)lpBuffer, BytesTransfered.LowPart);

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.OriginalSize.LowPart = BytesTransfered.LowPart;

		FILETIME Time1, Time2;
		GetFileTime(File.GetHandle(), &(pIndex->time.FileTime), &Time1, &Time2);
		File.Close();


		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;
		DecryptWorker(EncryptOffset.LowPart, (PBYTE)lpBuffer, BytesTransfered.LowPart, pIndex->adlr.Hash);
		if (XP3EncryptionFlag && pfProc)
			pIndex->info.EncryptedFlag = 0x80000000;
		else
			pIndex->info.EncryptedFlag = 0;

		pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if (!InfoNameZeroEnd)
			pIndex->file.ChunkSize.QuadPart -= 2;

		pIndex->segm.segm->bZlib = 0;

		pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.ArchiveSize.LowPart = BytesTransfered.LowPart;
		Offset.QuadPart += BytesTransfered.QuadPart;

		FileXP3.Write(lpBuffer, BytesTransfered.LowPart, &BytesTransfered);
	}

	XP3Header.IndexOffset = Offset;

	// generate index, calculate index size first
	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		Size.LowPart +=
			sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->file.ChunkSize) + MagicLength +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if (!InfoNameZeroEnd)
			Size.LowPart -= 2;
	}

	if (Size.LowPart > CompressedSize)
	{
		CompressedSize = Size.LowPart;
		lpCompressBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpCompressBuffer, CompressedSize);
	}
	if (Size.LowPart * 2 > BufferSize)
	{
		BufferSize = Size.LowPart * 2;
		lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
	}

	// generate index to lpCompressBuffer
	pIndex = pXP3Index;
	pbIndex = (PBYTE)lpCompressBuffer;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		DWORD n = sizeof(DWORD);
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

		if (InfoNameZeroEnd)
		{
			n = (pIndex->info.FileName.length() + 1) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		else
		{
			n = (pIndex->info.FileName.length()) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		pbIndex += n;
	}

	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = Size.LowPart;
	IndexHeader.ArchiveSize.LowPart = BufferSize;
	BufferSize = Size.LowPart;
	compress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
	IndexHeader.ArchiveSize.HighPart = 0;

	FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
	FileXP3.Write(lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	Offset.QuadPart = 0;
	FileXP3.Seek(Offset, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	FileXP3.Close();

	HeapFree(GetProcessHeap(), 0, lpBuffer);
	HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
	HeapFree(GetProcessHeap(), 0, pXP3Index);

	TVPExecuteScript(ttstr(L"Storages.removeAutoPath(System.exePath + \"" +  ttstr(Handle->CurrentTempFileName.c_str()) + L"\" + \">\");"));

	if (Handle->CurrentTempHandle != 0 && Handle->CurrentTempHandle != INVALID_HANDLE_VALUE)
		NtClose(Handle->CurrentTempHandle);

	InterlockedExchangePointer(&(Handle->CurrentTempHandle), INVALID_HANDLE_VALUE);

	Status = DeleteFileW(Handle->CurrentTempFileName.c_str());
	if (NT_FAILED(Status))
	{
		wstring OutputInfo;
		OutputInfo =  L"Making Package : Successful!\nBut you must relaunch this game\nand delete \"";
		OutputInfo += Handle->CurrentTempFileName;
		OutputInfo += L"\" to make the next package!!!";
		MessageBoxW(Handle->MainWindow, 
			OutputInfo.c_str(),
			L"KrkrExtract (Important Infomation!!)", MB_OK);
	}
	else
	{
		MessageBoxW(Handle->MainWindow, L"Making Package : Successful", L"KrkrExtract", MB_OK);
	}
	Handle->CurrentTempFileName = L"KrkrzTempWorker.xp3";
	return STATUS_SUCCESS;
}


NTSTATUS NTAPI KrkrPacker::DoDummyNormalPackExFirst(LPCWSTR lpBasePack)
{
	NTSTATUS                Status;
	NtFileDisk              File, FileXP3;
	GlobalData*             Handle;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset, BytesTransfered;
	SMyXP3IndexNormal       *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, 0);
	WCHAR                   CurTempFileName[MAX_PATH];
	ULONG64                 RandNum;

	Handle = GlobalData::GetGlobalData();

	RandNum = genrand64_int64();
	RtlZeroMemory(CurTempFileName, countof(CurTempFileName) * 2);
	wsprintfW(CurTempFileName, L"KrkrzTempWorker_%08x%08x.xp3", HiDword(RandNum), LoDword(RandNum));
	Handle->CurrentTempFileName = CurTempFileName;

	Status = FileXP3.Create(Handle->CurrentTempFileName.c_str());
	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Couldn't open a handle for temporary output file.", L"KrkrExtract", MB_OK);
		return Status;
	}

	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
	lpCompressBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedSize);
	pXP3Index = (SMyXP3IndexNormal *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	if (!lpBuffer || !lpCompressBuffer || !pXP3Index)
	{
		MessageBoxW(Handle->MainWindow, L"Insufficient memory to make package!!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		FileXP3.Close();

		if (lpBuffer)         HeapFree(GetProcessHeap(), 0, lpBuffer);
		if (lpCompressBuffer) HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		if (pXP3Index)        HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	Offset.QuadPart = BytesTransfered.QuadPart;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		WCHAR OutInfo[MAX_PATH];
		wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
		SetWindowTextW(Handle->MainWindow, OutInfo);
		Handle->SetProcess(Handle->MainWindow, (ULONG)(((float)(i + 1) / (float)FileList.size()) * 100.0), i, FileList.size());

		ZeroMemory(pIndex, sizeof(*pIndex));
		*(PDWORD)(pIndex->file.Magic) = CHUNK_MAGIC_FILE;
		*(PDWORD)(pIndex->info.Magic) = CHUNK_MAGIC_INFO;
		*(PDWORD)(pIndex->time.Magic) = CHUNK_MAGIC_TIME;
		*(PDWORD)(pIndex->segm.Magic) = CHUNK_MAGIC_SEGM;
		*(PDWORD)(pIndex->adlr.Magic) = CHUNK_MAGIC_ADLR;
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		//
		pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring FullName = lpBasePack;
		FullName += L"\\";
		FullName += FileList[i];

		Status = File.Open(FullName.c_str());
		if (NT_FAILED(Status))
		{
			if (Handle->DebugOn) 
				PrintConsoleW(L"Couldn't open %s\n", FullName.c_str());
		}

		File.GetSize(&Size);
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
		}

		File.Read(lpBuffer, Size.LowPart, &BytesTransfered);

		if (BytesTransfered.LowPart != Size.LowPart)
		{
			wstring InfoW(L"Dummy write :\n[Failed to read]\n");
			InfoW += FullName;
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			File.Close();
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_IO_DEVICE_ERROR;
		}

		pIndex->segm.segm->Offset = Offset;

		pIndex->info.FileName = FileList[i] + L".dummy";
		pIndex->info.FileNameLength = (USHORT)(FileList[i].size() + lstrlenW(L".dummy"));

		pIndex->adlr.Hash = adler32(1, (Bytef *)lpBuffer, BytesTransfered.LowPart);

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.OriginalSize.LowPart = BytesTransfered.LowPart;

		FILETIME Time1, Time2;
		GetFileTime(File.GetHandle(), &(pIndex->time.FileTime), &Time1, &Time2);
		File.Close();


		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;
		DecryptWorker(EncryptOffset.LowPart, (PBYTE)lpBuffer, BytesTransfered.LowPart, pIndex->adlr.Hash);
		if (XP3EncryptionFlag && pfProc)
			pIndex->info.EncryptedFlag = 0x80000000;
		else
			pIndex->info.EncryptedFlag = 0;

		pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if (!InfoNameZeroEnd)
			pIndex->file.ChunkSize.QuadPart -= 2;

		pIndex->segm.segm->bZlib = 0;

		pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.ArchiveSize.LowPart = BytesTransfered.LowPart;
		Offset.QuadPart += BytesTransfered.QuadPart;

		FileXP3.Write(lpBuffer, BytesTransfered.LowPart, &BytesTransfered);
	}

	XP3Header.IndexOffset = Offset;

	// generate index, calculate index size first
	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		Size.LowPart +=
			sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->file.ChunkSize) + MagicLength +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if (!InfoNameZeroEnd)
			Size.LowPart -= 2;
	}

	if (Size.LowPart > CompressedSize)
	{
		CompressedSize = Size.LowPart;
		lpCompressBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpCompressBuffer, CompressedSize);
	}
	if (Size.LowPart * 2 > BufferSize)
	{
		BufferSize = Size.LowPart * 2;
		lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
	}

	// generate index to lpCompressBuffer
	pIndex = pXP3Index;
	pbIndex = (PBYTE)lpCompressBuffer;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		ULONG_PTR n = sizeof(DWORD);
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

		if (InfoNameZeroEnd)
		{
			n = (pIndex->info.FileName.length() + 1) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		else
		{
			n = (pIndex->info.FileName.length()) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		pbIndex += n;
	}

	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = Size.LowPart;
	IndexHeader.ArchiveSize.LowPart = BufferSize;
	BufferSize = Size.LowPart;
	compress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
	IndexHeader.ArchiveSize.HighPart = 0;

	FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
	FileXP3.Write(lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	Offset.QuadPart = 0;
	FileXP3.Seek(Offset, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	FileXP3.Close();

	HeapFree(GetProcessHeap(), 0, lpBuffer);
	HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
	HeapFree(GetProcessHeap(), 0, pXP3Index);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI KrkrPacker::IterFiles(LPCWSTR lpPath)
{
	NTSTATUS         Status;
	HANDLE           hFind;
	GlobalData*      Handle;
	WCHAR            FilePath[MAX_PATH];
	WIN32_FIND_DATAW FindFileData;

	Handle = GlobalData::GetGlobalData();

	RtlZeroMemory(&FindFileData, sizeof(WIN32_FIND_DATAW));
	RtlZeroMemory(FilePath, countof(FilePath) * sizeof(WCHAR));

	wsprintfW(FilePath, L"%s%s", lpPath, L"\\*.*");

	hFind = FindFirstFileW(FilePath, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return STATUS_NO_SUCH_FILE;

	Status = STATUS_SUCCESS;

	do
	{
		if (!lstrcmpW(FindFileData.cFileName, L".") || !lstrcmpW(FindFileData.cFileName, L".."))
			continue;
		else if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		else
		{
			if (Handle->DebugOn)
				PrintConsoleW(L"Found file : %s\n", FindFileData.cFileName);

			FileList.push_back(FindFileData.cFileName);
		}
	} while (FindNextFileW(hFind, &FindFileData));

	return Status;
}


VOID FormatPathNormal(wstring& PackageName, ttstr& out)
{
	out.Clear();
	out = L"file://./";
	for (ULONG iPos = 0; iPos < PackageName.length(); iPos++)
	{
		if (PackageName[iPos] == L':')
			continue;
		else if (PackageName[iPos] == L'\\')
			out += L'/';
		else
			out += PackageName[iPos];
	}
	out += L'>';
}


NTSTATUS WINAPI KrkrPacker::DoM2DummyPackFirst(LPCWSTR lpBasePack)
{
	NTSTATUS                Status;
	GlobalData*             Handle;
	NtFileDisk              File, FileXP3;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset, BytesTransfered;
	SMyXP3IndexM2            *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, (ULONG64)0);
	WCHAR                   CurTempFileName[MAX_PATH];
	ULONG64                 RandNum;

	Handle = GlobalData::GetGlobalData();

	RandNum = genrand64_int64();
	RtlZeroMemory(CurTempFileName, countof(CurTempFileName) * 2);
	wsprintfW(CurTempFileName, L"KrkrzTempWorker_%08x%08x.xp3", HiDword(RandNum), LoDword(RandNum));
	Handle->CurrentTempFileName = CurTempFileName;

	Status = FileXP3.Create(Handle->CurrentTempFileName.c_str());
	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Couldn't create a handle for temporary output file.", L"KrkrExtract", MB_OK);
		return Status;
	}

	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
	lpCompressBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedSize);
	pXP3Index = (SMyXP3IndexM2 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	if (!lpBuffer || !lpCompressBuffer || !pXP3Index)
	{
		MessageBoxW(Handle->MainWindow, L"Insufficient memory to make package!!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		FileXP3.Close();
		
		if (lpBuffer)         HeapFree(GetProcessHeap(), 0, lpBuffer);
		if (lpCompressBuffer) HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		if (pXP3Index)        HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	Offset.QuadPart = BytesTransfered.QuadPart;

	if (FileList.size() == 0)
	{
		MessageBoxW(Handle->MainWindow, L"No File to be packed", L"KrkrExtract", MB_OK);
		FileXP3.Close();
		HeapFree(GetProcessHeap(), 0, lpBuffer);
		HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_UNSUCCESSFUL;
	}

	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		if (Handle->MainWindow)
		{
			WCHAR OutInfo[MAX_PATH];
			wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
			SetWindowTextW(Handle->MainWindow, OutInfo);
		}

		ZeroMemory(pIndex, sizeof(*pIndex));
		*(PDWORD)(pIndex->yuzu.Magic) = Handle->M2ChunkMagic;
		*(PDWORD)(pIndex->file.Magic) = CHUNK_MAGIC_FILE;
		*(PDWORD)(pIndex->info.Magic) = CHUNK_MAGIC_INFO;
		*(PDWORD)(pIndex->time.Magic) = CHUNK_MAGIC_TIME;
		*(PDWORD)(pIndex->segm.Magic) = CHUNK_MAGIC_SEGM;
		*(PDWORD)(pIndex->adlr.Magic) = CHUNK_MAGIC_ADLR;
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		//pIndex->adlr.ChunkSize.QuadPart = sizeof(pIndex->adlr) - sizeof(pIndex->adlr.Magic) - sizeof(pIndex->adlr.ChunkSize);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		if (M2NameZeroEnd)
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		else
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0 - 2;

		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring FullName = lpBasePack;
		FullName += L"\\";
		FullName += FileList[i];

		Status = File.Open(FullName.c_str());

		if (NT_FAILED(Status))
		{
			wstring InfoW(L"Dummy write :\n[Failed to open]\n");
			InfoW += FullName;
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return Status;
		}

		File.GetSize(&Size);
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
		}

		File.Read(lpBuffer, Size.LowPart, &BytesTransfered);

		if (BytesTransfered.LowPart != Size.LowPart)
		{
			wstring InfoW(L"Dummy write :\n[Failed to read]\n");
			InfoW += FullName;
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			File.Close();
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_IO_DEVICE_ERROR;
		}

		pIndex->segm.segm->Offset = Offset;

		wstring DummyName, HashName;
		
		DummyName = FileList[i] + L".dummy";
		auto DummyLowerName = ToLowerString(DummyName.c_str());

		GenMD5Code(DummyLowerName.c_str(), HashName);

		pIndex->info.FileName = HashName;
		pIndex->info.FileNameLength = (USHORT)HashName.length();

		pIndex->yuzu.Len = (USHORT)DummyName.length();
		pIndex->yuzu.Name = DummyName;

		if (M2NameZeroEnd)
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD)+sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2;
		else
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD)+sizeof(USHORT)+(pIndex->yuzu.Name.length()) * 2;

		pIndex->adlr.Hash = M2Hash;
		pIndex->yuzu.Hash = pIndex->adlr.Hash;

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.OriginalSize.LowPart = BytesTransfered.LowPart;

		FILETIME Time1, Time2;
		GetFileTime(File.GetHandle(), &(pIndex->time.FileTime), &Time1, &Time2);
		File.Close();
		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;

		if (XP3EncryptionFlag)
			pIndex->info.EncryptedFlag = 0x80000000;
		else
			pIndex->info.EncryptedFlag = 0;

		PackChunkList.push_back(*pIndex);

		pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
			pIndex->file.ChunkSize.QuadPart -= 2;
		else if (!M2NameZeroEnd && !InfoNameZeroEnd)
			pIndex->file.ChunkSize.QuadPart -= 4;
		
		pIndex->segm.segm->bZlib = 0;


		pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.ArchiveSize.LowPart = BytesTransfered.LowPart;
		Offset.QuadPart += BytesTransfered.QuadPart;

		FileXP3.Write(lpBuffer, BytesTransfered.LowPart, &BytesTransfered);
	}

	XP3Header.IndexOffset = Offset;

	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
			sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->file.ChunkSize) + MagicLength +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
			Size.LowPart -= 2;
		else if (!M2NameZeroEnd && !InfoNameZeroEnd)
			Size.LowPart -= 4;
	}

	if (Size.LowPart > CompressedSize)
	{
		CompressedSize = Size.LowPart;
		lpCompressBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpCompressBuffer, CompressedSize);
	}
	if (Size.LowPart * 2 > BufferSize)
	{
		BufferSize = Size.LowPart * 2;
		lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
	}

	// generate index to lpCompressBuffer
	pIndex = pXP3Index;
	pbIndex = (PBYTE)lpCompressBuffer;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		ULONG n = sizeof(DWORD);
		CopyMemory(pbIndex, &(pIndex->yuzu.Magic), n);
		pbIndex += n;
		n = 8;
		CopyMemory(pbIndex, &(pIndex->yuzu.ChunkSize), n);
		pbIndex += n;
		n = 4;
		CopyMemory(pbIndex, &(pIndex->yuzu.Hash), n);
		pbIndex += n;
		n = sizeof(USHORT);
		CopyMemory(pbIndex, &(pIndex->yuzu.Len), n);
		pbIndex += n;

		if (M2NameZeroEnd)
		{
			n = (pIndex->yuzu.Name.length() + 1) * 2;
			CopyMemory(pbIndex, (pIndex->yuzu.Name.c_str()), n);
		}
		else
		{
			n = (pIndex->yuzu.Name.length()) * 2;
			CopyMemory(pbIndex, (pIndex->yuzu.Name.c_str()), n);
		}
		pbIndex += n;

		n = sizeof(DWORD);
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

		if (InfoNameZeroEnd)
		{
			n = (pIndex->info.FileName.length() + 1) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		else
		{
			n = (pIndex->info.FileName.length()) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		pbIndex += n;
	}

	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = Size.LowPart;
	IndexHeader.ArchiveSize.LowPart = BufferSize;
	BufferSize = Size.LowPart;
	compress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
	IndexHeader.ArchiveSize.HighPart = 0;

	FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
	FileXP3.Write(lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	Offset.QuadPart = 0;
	FileXP3.Seek(Offset, FILE_BEGIN);
	FileXP3.Write( &XP3Header, sizeof(XP3Header), &BytesTransfered);

	FileXP3.Close();

	HeapFree(GetProcessHeap(), 0, lpBuffer);
	HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
	HeapFree(GetProcessHeap(), 0, pXP3Index);
	PackChunkList.clear();
	return STATUS_SUCCESS;
}



NTSTATUS WINAPI KrkrPacker::DoM2DummyPackFirst_Version2(LPCWSTR lpBasePack)
{
	NTSTATUS                Status;
	GlobalData*             Handle;
	NtFileDisk              File, FileXP3;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset, BytesTransfered;
	SMyXP3IndexM2            *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, (ULONG64)0);
	WCHAR                   CurTempFileName[MAX_PATH];
	ULONG64                 RandNum;


	Handle = GlobalData::GetGlobalData();

	RandNum = genrand64_int64();
	RtlZeroMemory(CurTempFileName, countof(CurTempFileName) * 2);
	wsprintfW(CurTempFileName, L"KrkrzTempWorker_%08x%08x.xp3", HiDword(RandNum), LoDword(RandNum));
	Handle->CurrentTempFileName = CurTempFileName;

	Status = FileXP3.Create(Handle->CurrentTempFileName.c_str());

	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Couldn't create a handle for temporary output file.", L"KrkrExtract", MB_OK);
		return Status;
	}

	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer         = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
	lpCompressBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedSize);
	pXP3Index = (SMyXP3IndexM2 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	if (!lpBuffer || !lpCompressBuffer || !pXP3Index)
	{
		MessageBoxW(Handle->MainWindow, L"Insufficient memory to make package!!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		FileXP3.Close();
		
		if (lpBuffer)         HeapFree(GetProcessHeap(), 0, lpBuffer);
		if (lpCompressBuffer) HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		if (pXP3Index)        HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	Offset.QuadPart = BytesTransfered.QuadPart;

	if (FileList.size() == 0)
	{
		MessageBoxW(Handle->MainWindow, L"No File to be packed", L"KrkrExtract", MB_OK);
		FileXP3.Close();
		HeapFree(GetProcessHeap(), 0, lpBuffer);
		HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_UNSUCCESSFUL;
	}

	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		if (Handle->MainWindow)
		{
			WCHAR OutInfo[MAX_PATH];
			wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
			SetWindowTextW(Handle->MainWindow, OutInfo);
		}

		ZeroMemory(pIndex, sizeof(*pIndex));

		*(PDWORD)(pIndex->file.Magic) = CHUNK_MAGIC_FILE;
		*(PDWORD)(pIndex->info.Magic) = CHUNK_MAGIC_INFO;
		*(PDWORD)(pIndex->time.Magic) = CHUNK_MAGIC_TIME;
		*(PDWORD)(pIndex->segm.Magic) = CHUNK_MAGIC_SEGM;
		*(PDWORD)(pIndex->adlr.Magic) = CHUNK_MAGIC_ADLR;
		*(PDWORD)(pIndex->yuzu.Magic) = Handle->M2ChunkMagic;

		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		//pIndex->adlr.ChunkSize.QuadPart = sizeof(pIndex->adlr) - sizeof(pIndex->adlr.Magic) - sizeof(pIndex->adlr.ChunkSize);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		if (M2NameZeroEnd)
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		else
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0 - 2;
		
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring FullName = lpBasePack;
		FullName += L"\\";
		FullName += FileList[i];
		Status = File.Open(FullName.c_str());

		if (NT_FAILED(Status))
		{
			wstring InfoW(L"Dummy write :\n[Failed to open]\n");
			InfoW += FullName;
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return Status;
		}

		File.GetSize(&Size);
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
		}

		File.Read(lpBuffer, Size.LowPart, &BytesTransfered);
		
		if (BytesTransfered.LowPart != Size.LowPart)
		{
			wstring InfoW(L"Dummy write :\n[Failed to read]\n");
			InfoW += FullName;
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			File.Close();
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_IO_DEVICE_ERROR;
		}

		pIndex->segm.segm->Offset = Offset;

		wstring DummyName, HashName;

		DummyName = FileList[i] + L".dummy";
		auto&& DummyLowerName = ToLowerString(DummyName.c_str());

		GenMD5Code(DummyLowerName.c_str(), HashName);
		pIndex->info.FileName = HashName;
		pIndex->info.FileNameLength = (USHORT)HashName.length();

		pIndex->yuzu.Len = (USHORT)DummyName.length();
		pIndex->yuzu.Name = DummyName;

		if (M2NameZeroEnd)
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2;
		else
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (pIndex->yuzu.Name.length()) * 2;

		//adler32(1/*adler32(0, 0, 0)*/, (Bytef *)lpBuffer, BytesTransfered);
		pIndex->adlr.Hash = M2Hash;
		pIndex->yuzu.Hash = pIndex->adlr.Hash;

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.OriginalSize.LowPart = BytesTransfered.LowPart;

		FILETIME Time1, Time2;
		GetFileTime(File.GetHandle(), &(pIndex->time.FileTime), &Time1, &Time2);
		File.Close();
		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;

		if (XP3EncryptionFlag)
			pIndex->info.EncryptedFlag = 0x80000000;
		else
			pIndex->info.EncryptedFlag = 0;

		PackChunkList.push_back(*pIndex);

		if (M2NameZeroEnd && InfoNameZeroEnd)
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		}
		else if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 2;
		}
		else
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 4;
		}

		pIndex->segm.segm->bZlib = 0;

		pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.ArchiveSize.LowPart = BytesTransfered.LowPart;
		Offset.QuadPart += BytesTransfered.QuadPart;

		FileXP3.Write(lpBuffer, BytesTransfered.LowPart, &BytesTransfered);
	}

	XP3Header.IndexOffset = Offset;

	// generate index, calculate index size first
	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
			sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->file.ChunkSize) + MagicLength +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
			Size.LowPart -= 2;
		else
			Size.LowPart -= 4;
	}

	if (Size.LowPart > CompressedSize)
	{
		CompressedSize = Size.LowPart;
		lpCompressBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpCompressBuffer, CompressedSize);
	}
	if (Size.LowPart * 2 > BufferSize)
	{
		BufferSize = Size.LowPart * 2;
		lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
	}

	// generate index to lpCompressBuffer
	pIndex = pXP3Index;
	pbIndex = (PBYTE)lpCompressBuffer;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		ULONG n = sizeof(DWORD);
		CopyMemory(pbIndex, &(pIndex->yuzu.Magic), n);
		pbIndex += n;
		n = 8;
		CopyMemory(pbIndex, &(pIndex->yuzu.ChunkSize), n);
		pbIndex += n;
		n = 4;
		CopyMemory(pbIndex, &(pIndex->yuzu.Hash), n);
		pbIndex += n;
		n = sizeof(USHORT);
		CopyMemory(pbIndex, &(pIndex->yuzu.Len), n);
		pbIndex += n;

		if (M2NameZeroEnd)
		{
			n = (pIndex->yuzu.Name.length() + 1) * 2;
			CopyMemory(pbIndex, (pIndex->yuzu.Name.c_str()), n);
		}
		else
		{
			n = (pIndex->yuzu.Name.length()) * 2;
			CopyMemory(pbIndex, (pIndex->yuzu.Name.c_str()), n);
		}
		pbIndex += n;
	}

	pIndex = pXP3Index;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
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

		if (InfoNameZeroEnd)
		{
			n = (pIndex->info.FileName.length() + 1) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		else
		{
			n = (pIndex->info.FileName.length()) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		pbIndex += n;
	}

	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = Size.LowPart;
	IndexHeader.ArchiveSize.LowPart = BufferSize;
	BufferSize = Size.LowPart;
	compress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
	IndexHeader.ArchiveSize.HighPart = 0;

	FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
	FileXP3.Write(lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	Offset.QuadPart = 0;
	FileXP3.Seek(Offset, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	FileXP3.Close();

	HeapFree(GetProcessHeap(), 0, lpBuffer);
	HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
	HeapFree(GetProcessHeap(), 0, pXP3Index);
	return STATUS_SUCCESS;
}


/*
[+] M2 modified some code from krkrz
_DWORD *__thiscall sub_427500(void *this)
{
	int v1; // eax
	int v2; // esi
	_DWORD *result; // eax
	int v4; // [esp+0h] [ebp-28h]
	void *Memory; // [esp+10h] [ebp-18h]
	int v6; // [esp+14h] [ebp-14h]
	int *v7; // [esp+18h] [ebp-10h]
	int v8; // [esp+24h] [ebp-4h]

	v7 = &v4;
	v6 = 0;
	v8 = 0;
	v1 = sub_40EF60(this);  //[-] You should check here....wtf
	v2 = v1;
	v6 = v1;
	v8 = -1;
	result = operator new(0xCu);
	Memory = result;
	v8 = 2;
	if (result)
	{
		*result = &tTVPIStreamAdapter::`vftable';
		result[1] = v2;
		result[2] = 1;
	}
	else
	{
		result = 0;
	}
	v8 = -1;
	return result;
}
*/


#define ENABLE_SCRIPT 1

NTSTATUS NTAPI KrkrPacker::DoM2Pack(LPCWSTR lpBasePack, LPCWSTR GuessPackage, LPCWSTR OutName)
{
	NTSTATUS                Status;
	GlobalData*             Handle;
	NtFileDisk              FileXP3;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset, BytesTransfered;
	SMyXP3IndexM2            *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, (ULONG64)0);
	tTJSVariant             ExecResult;
	wstring                 DebugInfo;
	WCHAR                   CurrentDir[MAX_PATH];
	ttstr                   VirtualArchive;

	Handle = GlobalData::GetGlobalData();

	FileList.clear();
	IterFiles(lpBasePack);

	Status = DoM2DummyPackFirst(lpBasePack);
	if (NT_FAILED(Status))
		return Status;

	RtlZeroMemory(CurrentDir, sizeof(CurrentDir));
	GetCurrentDirectoryW(countof(CurrentDir) - 1, CurrentDir);

	VirtualArchive = CurrentDir;
	VirtualArchive += L"\\";
	VirtualArchive += Handle->CurrentTempFileName.c_str();
	VirtualArchive += L">";


	//[+] Bug check here
	//1.Hash generation error?
	//2.chunk size error

#if !defined(ENABLE_SCRIPT)
	//how to ensure this archive is mounted?
	//by GetHandle?
	TVPAddAutoPath(VirtualArchive);
	//check the status
	//1.If engine fail to mount the fake archive?
	//2.logical bug
	//3.DNOT USE TVPExecuteScript

	
#else

	TVPExecuteScript(ttstr(L"Storages.addAutoPath(System.exePath + \"" + ttstr(Handle->CurrentTempFileName.c_str()) + L"\" + \">\");"), &ExecResult);
	
#if 0   //[+] Return value is not reliable...
	if (ExecResult.AsInteger() == FALSE)
	{
		Status = DisPlayProcessFileHandle(DebugInfo);
		if (NT_FAILED(Status))
		{
			PrintConsoleW(L"DisPlayProcessFileHandle failed with status = %08x\n", Status);
		}
		else
		{
			PrintConsoleW(L"Handle status :\n");
			PrintConsoleW(DebugInfo.c_str());
		}
		
		MessageBoxW(Handle->MainWindow, L"Script exec error.(add)", L"KrkrExtract", MB_OK);
		//return STATUS_UNSUCCESSFUL;
	}
#endif

#endif

	Status = FileXP3.Create(OutName);
	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Couldn't create a handle for output xp3 file.", L"KrkrExtract", MB_OK);
		return Status;
	}

	BufferSize       = 0x10000;
	CompressedSize   = BufferSize;
	lpBuffer         = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
	lpCompressBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedSize);
	pXP3Index        = (SMyXP3IndexM2 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pXP3Index) * FileList.size());
	pIndex           = pXP3Index;

	if (!lpBuffer || !lpCompressBuffer || !pXP3Index)
	{
		MessageBoxW(Handle->MainWindow, L"Insufficient memory to make package!!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		FileXP3.Close();
		
		if (lpBuffer)         HeapFree(GetProcessHeap(), 0, lpBuffer);
		if (lpCompressBuffer) HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		if (pXP3Index)        HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	Offset.QuadPart = BytesTransfered.QuadPart;
	for (ULONG i = 0; i <FileList.size(); ++pIndex, i++)
	{
		if (Handle->MainWindow)
		{
			WCHAR OutInfo[MAX_PATH];
			wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
			SetWindowTextW(Handle->MainWindow, OutInfo);
			Handle->SetProcess(Handle->MainWindow, (ULONG)(((float)(i + 1) / (float)FileList.size()) * 100.0), i, FileList.size());
		}

		ZeroMemory(pIndex, sizeof(*pIndex));
		*(PDWORD)(pIndex->file.Magic) = CHUNK_MAGIC_FILE;
		*(PDWORD)(pIndex->info.Magic) = CHUNK_MAGIC_INFO;
		*(PDWORD)(pIndex->time.Magic) = CHUNK_MAGIC_TIME;
		*(PDWORD)(pIndex->segm.Magic) = CHUNK_MAGIC_SEGM;
		*(PDWORD)(pIndex->adlr.Magic) = CHUNK_MAGIC_ADLR;
		*(PDWORD)(pIndex->yuzu.Magic) = Handle->M2ChunkMagic;
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		if (M2NameZeroEnd)
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		else
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0 - 2;

		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring DummyName = FileList[i] + L".dummy";

		ttstr FullName(L"archive://./" + ttstr(Handle->CurrentTempFileName.c_str()) + L"/");
		
		FullName += DummyName.c_str();

		IStream* st = TVPCreateIStream(FullName, TJS_BS_READ);
		if (st == NULL)
		{
			wstring InfoW(L"(virtual)Couldn't open :\n");
			InfoW += FileList[i];
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			FileXP3.Close();
			DeleteFileW(OutName);
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_UNSUCCESSFUL;
		}

		STATSTG t;
		st->Stat(&t, STATFLAG_DEFAULT);
		Size.QuadPart = t.cbSize.QuadPart;
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
		}
		st->Read(lpBuffer, Size.LowPart, &BytesTransfered.LowPart);
		
		if (BytesTransfered.LowPart != Size.LowPart)
		{
			wstring InfoW(L"Couldn't read :\n");
			InfoW += FileList[i];
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_UNSUCCESSFUL;
		}

		pIndex->segm.segm->Offset = Offset;

		auto LowerName = ToLowerString(FileList[i].c_str());
		wstring HashName;

		GenMD5Code(LowerName.c_str(), HashName);
		pIndex->info.FileName = HashName;
		pIndex->info.FileNameLength = (USHORT)HashName.length();

		pIndex->yuzu.Len = (USHORT)FileList[i].length();
		pIndex->yuzu.Name = FileList[i];

		if (M2NameZeroEnd)
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD)+sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2;
		else
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD)+sizeof(USHORT)+(pIndex->yuzu.Name.length()) * 2;

		pIndex->adlr.Hash = M2Hash;
		pIndex->yuzu.Hash = pIndex->adlr.Hash;

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.OriginalSize.LowPart = BytesTransfered.LowPart;

		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;

		if (XP3EncryptionFlag)
			pIndex->info.EncryptedFlag = 0x80000000;
		else
			pIndex->info.EncryptedFlag = 0x0;
		

		pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
			pIndex->file.ChunkSize.QuadPart -= 2;
		else if (!M2NameZeroEnd && !InfoNameZeroEnd)
			pIndex->file.ChunkSize.QuadPart -= 4;
		
		pIndex->segm.segm->bZlib = 0;

		pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.ArchiveSize.LowPart = BytesTransfered.LowPart;
		Offset.QuadPart += BytesTransfered.QuadPart;

		FileXP3.Write(lpBuffer, BytesTransfered.LowPart, &BytesTransfered);
	}

	XP3Header.IndexOffset = Offset;

	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
			sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->file.ChunkSize) + MagicLength +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
			Size.LowPart -= 2;
		else if (!M2NameZeroEnd && !InfoNameZeroEnd)
			Size.LowPart -= 4;
	}

	if (Size.LowPart > CompressedSize)
	{
		CompressedSize = Size.LowPart;
		lpCompressBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpCompressBuffer, CompressedSize);
	}
	if (Size.LowPart * 2 > BufferSize)
	{
		BufferSize = Size.LowPart * 2;
		lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
	}

	// generate index to lpCompressBuffer
	pIndex = pXP3Index;
	pbIndex = (PBYTE)lpCompressBuffer;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		ULONG n = sizeof(DWORD);
		CopyMemory(pbIndex, &(pIndex->yuzu.Magic), n);
		pbIndex += n;
		n = 8;
		CopyMemory(pbIndex, &(pIndex->yuzu.ChunkSize), n);
		pbIndex += n;
		n = 4;
		CopyMemory(pbIndex, &(pIndex->yuzu.Hash), n);
		pbIndex += n;
		n = sizeof(USHORT);
		CopyMemory(pbIndex, &(pIndex->yuzu.Len), n);
		pbIndex += n;

		if (M2NameZeroEnd)
		{
			n = (pIndex->yuzu.Name.length() + 1) * 2;
			CopyMemory(pbIndex, (pIndex->yuzu.Name.c_str()), n);
		}
		else
		{
			n = (pIndex->yuzu.Name.length()) * 2;
			CopyMemory(pbIndex, (pIndex->yuzu.Name.c_str()), n);
		}
		pbIndex += n;

		n = sizeof(DWORD);
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

		if (InfoNameZeroEnd)
		{
			n = (pIndex->info.FileName.length() + 1) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		else
		{
			n = (pIndex->info.FileName.length()) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		pbIndex += n;
	}

	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = Size.LowPart;
	IndexHeader.ArchiveSize.LowPart = BufferSize;
	BufferSize = Size.LowPart;
	compress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
	IndexHeader.ArchiveSize.HighPart = 0;

	FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
	FileXP3.Write(lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	Offset.QuadPart = 0;
	FileXP3.Seek(Offset, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	FileXP3.Close();

	HeapFree(GetProcessHeap(), 0, lpBuffer);
	HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
	HeapFree(GetProcessHeap(), 0, pXP3Index);

	//TVPRemoveAutoPath(VirtualArchive);
	TVPExecuteScript(ttstr(L"Storages.removeAutoPath(System.exePath + \"" + ttstr(Handle->CurrentTempFileName.c_str()) + L"\" + \">\");"), &ExecResult);

	if (Handle->CurrentTempHandle != 0 && Handle->CurrentTempHandle != INVALID_HANDLE_VALUE)
		NtClose(Handle->CurrentTempHandle);

	InterlockedExchangePointer(&(Handle->CurrentTempHandle), INVALID_HANDLE_VALUE);

	Status = DeleteFileW(Handle->CurrentTempFileName.c_str());
	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Making Package : Successful!\nBut you must relaunch this game\nand delete \"KrkrzTempWorker.xp3\" to make the next package!!!", 
			L"KrkrExtract (Important Information!!)", MB_OK);
	}
	else
	{
		MessageBoxW(Handle->MainWindow, L"Making Package : Successful", L"KrkrExtract", MB_OK);
	}

	Handle->CurrentTempFileName = L"KrkrzTempWorker.xp3";

	return STATUS_SUCCESS;
}

//Since nekopara vol2
HRESULT WINAPI KrkrPacker::DoM2Pack_Version2(LPCWSTR lpBasePack, LPCWSTR GuessPackage, LPCWSTR OutName)
{
	NTSTATUS                Status;
	GlobalData*             Handle;
	NtFileDisk              FileXP3;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset, BytesTransfered;
	SMyXP3IndexM2            *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, (ULONG64)0);
	tTJSVariant             ExecResult;

	Handle = GlobalData::GetGlobalData();

	FileList.clear();
	IterFiles(lpBasePack);

	Status = DoM2DummyPackFirst_Version2(lpBasePack);
	if (NT_FAILED(Status))
		return Status;

	TVPExecuteScript(ttstr(L"Storages.addAutoPath(System.exePath + \"" + ttstr(Handle->CurrentTempFileName.c_str()) + L"\" + \">\");"), &ExecResult);
	
#if 0
	if (ExecResult.AsInteger() == FALSE)
	{
		MessageBoxW(Handle->MainWindow, L"Script exec error. (add)", L"KrkrExtract", MB_OK);
		return STATUS_UNSUCCESSFUL;
	}
#endif

	Status = FileXP3.Create(OutName);
	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Couldn't create a handle for output xp3 file.", L"KrkrExtract", MB_OK);
		return Status;
	}

	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
	lpCompressBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedSize);
	pXP3Index = (SMyXP3IndexM2 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	if (!lpBuffer || !lpCompressBuffer || !pXP3Index)
	{
		MessageBoxW(Handle->MainWindow, L"Insufficient memory to make package!!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		FileXP3.Close();
		
		if (lpBuffer)         HeapFree(GetProcessHeap(), 0, lpBuffer);
		if (lpCompressBuffer) HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		if (pXP3Index)        HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	Offset.QuadPart = BytesTransfered.QuadPart;
	for (ULONG i = 0; i <FileList.size(); ++pIndex, i++)
	{
		if (Handle->MainWindow)
		{
			WCHAR OutInfo[MAX_PATH];
			wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
			SetWindowTextW(Handle->MainWindow, OutInfo);
			Handle->SetProcess(Handle->MainWindow, (ULONG)(((float)(i + 1) / (float)FileList.size()) * 100.0), i, FileList.size());
		}

		ZeroMemory(pIndex, sizeof(*pIndex));
		*(PDWORD)(pIndex->file.Magic) = CHUNK_MAGIC_FILE;
		*(PDWORD)(pIndex->info.Magic) = CHUNK_MAGIC_INFO;
		*(PDWORD)(pIndex->time.Magic) = CHUNK_MAGIC_TIME;
		*(PDWORD)(pIndex->segm.Magic) = CHUNK_MAGIC_SEGM;
		*(PDWORD)(pIndex->adlr.Magic) = CHUNK_MAGIC_ADLR;
		*(PDWORD)(pIndex->yuzu.Magic) = Handle->M2ChunkMagic;
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		if (M2NameZeroEnd)
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		else
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0 - 2;
		
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring DummyName = FileList[i] + L".dummy";

		ttstr FullName(L"archive://./" + ttstr(Handle->CurrentTempFileName.c_str()) + L"/");

		FullName += DummyName.c_str();

		IStream* st = TVPCreateIStream(FullName, TJS_BS_READ);
		if (st == NULL)
		{
			wstring InfoW(L"Couldn't open :\n");
			InfoW += FileList[i];
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_UNSUCCESSFUL;
		}

		STATSTG t;
		st->Stat(&t, STATFLAG_DEFAULT);
		Size.QuadPart = t.cbSize.QuadPart;
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
		}
		st->Read(lpBuffer, Size.LowPart, &BytesTransfered.LowPart);

		if (BytesTransfered.LowPart != Size.LowPart)
		{
			wstring InfoW(L"Couldn't read :\n");
			InfoW += FileList[i];
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_UNSUCCESSFUL;
		}

		pIndex->segm.segm->Offset = Offset;

		auto LowerName = ToLowerString(FileList[i].c_str());
		wstring HashName;

		GenMD5Code(LowerName.c_str(), HashName);
		pIndex->info.FileName = HashName;
		pIndex->info.FileNameLength = (USHORT)HashName.length();

		pIndex->yuzu.Len = (USHORT)FileList[i].length();
		pIndex->yuzu.Name = FileList[i];

		if (M2NameZeroEnd)
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2;
		else
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (pIndex->yuzu.Name.length()) * 2;

		pIndex->adlr.Hash = M2Hash;
		pIndex->yuzu.Hash = pIndex->adlr.Hash;

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.OriginalSize.LowPart = BytesTransfered.LowPart;

		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;

		if (XP3EncryptionFlag)
			pIndex->info.EncryptedFlag = 0x80000000;
		else
			pIndex->info.EncryptedFlag = 0;

		pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
			pIndex->file.ChunkSize.QuadPart -= 2;
		else
			pIndex->file.ChunkSize.QuadPart -= 4;
		
		pIndex->segm.segm->bZlib = 0;

		pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.ArchiveSize.LowPart = BytesTransfered.LowPart;
		Offset.QuadPart += BytesTransfered.QuadPart;

		FileXP3.Write(lpBuffer, BytesTransfered.LowPart, &BytesTransfered);
	}

	XP3Header.IndexOffset = Offset;

	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
			sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->file.ChunkSize) + MagicLength +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
			Size.LowPart -= 2;
		else
			Size.LowPart -= 4;
	}

	if (Size.LowPart > CompressedSize)
	{
		CompressedSize = Size.LowPart;
		lpCompressBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpCompressBuffer, CompressedSize);
	}
	if (Size.LowPart * 2 > BufferSize)
	{
		BufferSize = Size.LowPart * 2;
		lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
	}

	// generate index to lpCompressBuffer
	pIndex = pXP3Index;
	pbIndex = (PBYTE)lpCompressBuffer;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		ULONG n = sizeof(DWORD);
		CopyMemory(pbIndex, &(pIndex->yuzu.Magic), n);
		pbIndex += n;
		n = 8;
		CopyMemory(pbIndex, &(pIndex->yuzu.ChunkSize), n);
		pbIndex += n;
		n = 4;
		CopyMemory(pbIndex, &(pIndex->yuzu.Hash), n);
		pbIndex += n;
		n = sizeof(USHORT);
		CopyMemory(pbIndex, &(pIndex->yuzu.Len), n);
		pbIndex += n;

		if (M2NameZeroEnd)
		{
			n = (pIndex->yuzu.Name.length() + 1) * 2;
			CopyMemory(pbIndex, (pIndex->yuzu.Name.c_str()), n);
		}
		else
		{
			n = (pIndex->yuzu.Name.length()) * 2;
			CopyMemory(pbIndex, (pIndex->yuzu.Name.c_str()), n);
		}
		pbIndex += n;
	}

	pIndex = pXP3Index;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
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

		if (InfoNameZeroEnd)
		{
			n = (pIndex->info.FileName.length() + 1) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		else
		{
			n = (pIndex->info.FileName.length()) * 2;
			CopyMemory(pbIndex, pIndex->info.FileName.c_str(), n);
		}
		pbIndex += n;
	}

	IndexHeader.bZlib = 1;
	IndexHeader.OriginalSize.QuadPart = Size.LowPart;
	IndexHeader.ArchiveSize.LowPart = BufferSize;
	BufferSize = Size.LowPart;
	compress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
	IndexHeader.ArchiveSize.HighPart = 0;

	FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
	FileXP3.Write(lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	Offset.QuadPart = 0;
	FileXP3.Seek(Offset, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	FileXP3.Close();

	HeapFree(GetProcessHeap(), 0, lpBuffer);
	HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
	HeapFree(GetProcessHeap(), 0, pXP3Index);

	TVPExecuteScript(ttstr(L"Storages.removeAutoPath(System.exePath + \"" + ttstr(Handle->CurrentTempFileName.c_str()) + L"\" + \">\");"), &ExecResult);

	if (Handle->CurrentTempHandle != 0 && Handle->CurrentTempHandle != INVALID_HANDLE_VALUE)
		NtClose(Handle->CurrentTempHandle);

	InterlockedExchangePointer(&(Handle->CurrentTempHandle), INVALID_HANDLE_VALUE);

	Status = DeleteFileW(Handle->CurrentTempFileName.c_str());
	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Making Package : Successful!\nBut you must relaunch this game\nand delete \"KrkrzTempWorker.xp3\" to make the next package!!!",
			L"KrkrExtract (Important Information!!)", MB_OK);
	}
	else
	{
		MessageBoxW(Handle->MainWindow, L"Making Package : Successful", L"KrkrExtract", MB_OK);
	}

	Handle->CurrentTempFileName = L"KrkrzTempWorker.xp3";

	return STATUS_SUCCESS;
}


NTSTATUS NTAPI KrkrPacker::DoM2Pack_SenrenBanka(LPCWSTR lpBasePack, LPCWSTR GuessPackage, LPCWSTR OutName)
{
	NTSTATUS                     Status;
	NtFileDisk                   FileXP3;
	GlobalData*                  Handle;
	PBYTE                        pbIndex;
	ULONG                        BufferSize, CompressedSize, BlockSize;
	PVOID                        lpBuffer, lpCompressBuffer;
	PBYTE                        lpBlock, lpBlockCompressed;
	LARGE_INTEGER                Size, Offset, BytesTransfered, ChunkSize;
	SMyXP3IndexM2                *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER        IndexHeader;
	BYTE                         FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER             XP3Header(FirstMagic, (ULONG64)0);
	KRKRZ_M2_Senrenbanka_HEADER  SenrenBankaHeader;
	tTJSVariant                  ExecResult;

	FileList.clear();
	IterFiles(lpBasePack);

	Handle = GlobalData::GetGlobalData();
	Status = DoM2DummyPackFirst_SenrenBanka(lpBasePack);
	if (NT_FAILED(Status))
		return Status;

	if (Handle->DebugOn)
		PrintConsoleW(L"Packing files...\n");

	TVPExecuteScript(ttstr(L"Storages.addAutoPath(System.exePath + \"" + ttstr(Handle->CurrentTempFileName.c_str()) + L"\" + \">\");"), &ExecResult);
	
#if 0
	if (ExecResult.AsInteger() == FALSE)
	{
		MessageBoxW(Handle->MainWindow, L"Script exec error. (add)", L"KrkrExtract", MB_OK);
		return STATUS_UNSUCCESSFUL;
	}
#endif
	
	Status = FileXP3.Create(OutName);
	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Couldn't create a handle for output xp3 file.", L"KrkrExtract", MB_OK);
		return Status;
	}

	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
	lpCompressBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedSize);
	pXP3Index = (SMyXP3IndexM2 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	if (!lpBuffer || !lpCompressBuffer || !pXP3Index)
	{
		MessageBoxW(Handle->MainWindow, L"Insufficient memory to make package!!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		FileXP3.Close();

		if (lpBuffer)         HeapFree(GetProcessHeap(), 0, lpBuffer);
		if (lpCompressBuffer) HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		if (pXP3Index)        HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	Offset.QuadPart = BytesTransfered.QuadPart;
	for (ULONG i = 0; i <FileList.size(); ++pIndex, i++)
	{
		WCHAR OutInfo[MAX_PATH];
		RtlZeroMemory(OutInfo, countof(OutInfo) * sizeof(WCHAR));
		wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
		SetWindowTextW(Handle->MainWindow, OutInfo);
		Handle->SetProcess(Handle->MainWindow, (ULONG)(((float)(i + 1) / (float)FileList.size()) * 100.0), i, FileList.size());

		ZeroMemory(pIndex, sizeof(*pIndex));
		*(PDWORD)(pIndex->file.Magic) = CHUNK_MAGIC_FILE;
		*(PDWORD)(pIndex->info.Magic) = CHUNK_MAGIC_INFO;
		*(PDWORD)(pIndex->time.Magic) = CHUNK_MAGIC_TIME;
		*(PDWORD)(pIndex->segm.Magic) = CHUNK_MAGIC_SEGM;
		*(PDWORD)(pIndex->adlr.Magic) = CHUNK_MAGIC_ADLR;
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C;
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;
		pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring DummyName = FileList[i] + L".dummy";

		ttstr FullName(L"archive://./" + ttstr(Handle->CurrentTempFileName.c_str()) + "/");

		FullName += DummyName.c_str();

		IStream* Stream = TVPCreateIStream(FullName, TJS_BS_READ);
		if (Stream == NULL)
		{
			if (Handle->DebugOn)
				PrintConsoleW(L"Couldn't open %s\n", DummyName.c_str());

			wstring InfoW(L"Couldn't open :\n");
			InfoW += FileList[i];
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_UNSUCCESSFUL;
		}

		STATSTG t;
		Stream->Stat(&t, STATFLAG_DEFAULT);
		
		Size.QuadPart = t.cbSize.QuadPart;
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
		}
		
		Stream->Read(lpBuffer, Size.LowPart, &BytesTransfered.LowPart);
		
		if (BytesTransfered.LowPart != Size.LowPart)
		{
			wstring InfoW(L"Couldn't read :\n");
			InfoW += FileList[i];
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_UNSUCCESSFUL;
		}
		
		pIndex->segm.segm->Offset = Offset;

		auto LowerName = ToLowerString(FileList[i].c_str());
		wstring HashName;

		GenMD5Code(LowerName.c_str(), SenrenBankaInfo.ProductName, HashName);
		pIndex->info.FileName = HashName;
		pIndex->info.FileNameLength = (USHORT)HashName.length();

		pIndex->yuzu.Len = (USHORT)FileList[i].length();
		pIndex->yuzu.Name = FileList[i];

		if (M2NameZeroEnd)
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2;
		else
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (pIndex->yuzu.Name.length()) * 2;

		pIndex->adlr.Hash = M2Hash;
		pIndex->yuzu.Hash = pIndex->adlr.Hash;

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.OriginalSize.LowPart = BytesTransfered.LowPart;

		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;

		if (XP3EncryptionFlag)
			pIndex->info.EncryptedFlag = 0x80000000;
		else
			pIndex->info.EncryptedFlag = 0;

		PackChunkList.push_back(*pIndex);

		pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		pIndex->segm.segm->bZlib = 0;

		pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.ArchiveSize.LowPart = BytesTransfered.LowPart;
		Offset.QuadPart += BytesTransfered.QuadPart;

		FileXP3.Write(lpBuffer, BytesTransfered.LowPart, &BytesTransfered);
	}

	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		Size.LowPart += 
			sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->file.ChunkSize) + MagicLength +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
	}

	Size.LowPart += sizeof(KRKRZ_M2_Senrenbanka_HEADER);

	if (Size.LowPart > CompressedSize)
	{
		CompressedSize = Size.LowPart;
		lpCompressBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpCompressBuffer, CompressedSize);
	}
	if (Size.LowPart * 2 > BufferSize)
	{
		BufferSize = Size.LowPart * 2;
		lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
	}

	// generate index to lpCompressBuffer
	
	lpBlock = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FileList.size() * sizeof(KRKRZ_XP3_INDEX_CHUNK_Yuzu2));
	lpBlockCompressed = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FileList.size() * sizeof(KRKRZ_XP3_INDEX_CHUNK_Yuzu2) * 2);

	BlockSize = 0;
	for (ULONG i = 0; i < FileList.size(); i++)
	{
		RtlCopyMemory((lpBlock + BlockSize), &M2SubChunkMagic, 4);
		BlockSize += 4;
		ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (lstrlenW(FileList[i].c_str()) + 1) * 2;
		RtlCopyMemory((lpBlock + BlockSize), &(ChunkSize.QuadPart), 8);
		BlockSize += 8;
		RtlCopyMemory((lpBlock + BlockSize), &M2Hash, 4);
		BlockSize += 4;

		USHORT NameLength = (USHORT)FileList[i].length();
		RtlCopyMemory((lpBlock + BlockSize), &NameLength, 2);
		BlockSize += 2;
		RtlCopyMemory((lpBlock + BlockSize), FileList[i].c_str(), (FileList[i].length() + 1) * 2);
		BlockSize += (FileList[i].length() + 1) * 2;
	}


	SenrenBankaHeader.OriginalSize = BlockSize;
	SenrenBankaHeader.Magic = Handle->M2ChunkMagic;
	SenrenBankaHeader.LengthOfProduct = SenrenBankaInfo.LengthOfProduct;
	lstrcpyW(SenrenBankaHeader.ProductName, SenrenBankaInfo.ProductName);
	SenrenBankaHeader.ArchiveSize = FileList.size() * sizeof(KRKRZ_XP3_INDEX_CHUNK_Yuzu2) * 2;
	compress2(lpBlockCompressed, &SenrenBankaHeader.ArchiveSize, lpBlock, BlockSize, Z_BEST_COMPRESSION);
	HeapFree(GetProcessHeap(), 0, lpBlock);
	SenrenBankaHeader.ChunkSize = SenrenBankaInfo.ChunkSize;
	SenrenBankaHeader.Offset = Offset;


	if (Handle->IsSpcialChunkEncrypted && Handle->SpecialChunkDecoder)
		Handle->SpecialChunkDecoder(lpBlockCompressed, lpBlockCompressed, 
			SenrenBankaHeader.ArchiveSize <= 0x100 ? SenrenBankaHeader.ArchiveSize : 0x100);

	FileXP3.Write(lpBlockCompressed, SenrenBankaHeader.ArchiveSize);
	HeapFree(GetProcessHeap(), 0, lpBlockCompressed);

	Offset.QuadPart += SenrenBankaHeader.ArchiveSize;
	XP3Header.IndexOffset = Offset;

	BlockSize = 0;

	*(PDWORD)(PBYTE)lpCompressBuffer = Handle->M2ChunkMagic;
	BlockSize += 4;
	*(PULONG64)((PBYTE)lpCompressBuffer + BlockSize) = SenrenBankaHeader.ChunkSize.QuadPart;
	BlockSize += 8;
	*(PULONG64)((PBYTE)lpCompressBuffer + BlockSize) = SenrenBankaHeader.Offset.QuadPart;
	BlockSize += 8;
	*(PDWORD)((PBYTE)lpCompressBuffer + BlockSize) = SenrenBankaHeader.OriginalSize;
	BlockSize += 4;
	*(PDWORD)((PBYTE)lpCompressBuffer + BlockSize) = SenrenBankaHeader.ArchiveSize;
	BlockSize += 4;
	*(PUSHORT)((PBYTE)lpCompressBuffer + BlockSize) = SenrenBankaHeader.LengthOfProduct;
	BlockSize += 2;
	RtlCopyMemory(((PBYTE)lpCompressBuffer + BlockSize), SenrenBankaHeader.ProductName, (lstrlenW(SenrenBankaHeader.ProductName) + 1) * 2);
	BlockSize += (lstrlenW(SenrenBankaHeader.ProductName) + 1) * 2;

	pIndex = pXP3Index;
	pbIndex = (PBYTE)lpCompressBuffer + BlockSize;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
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
	IndexHeader.OriginalSize.QuadPart = Size.LowPart + BlockSize - sizeof(KRKRZ_M2_Senrenbanka_HEADER);
	BufferSize = Size.LowPart + BlockSize - sizeof(KRKRZ_M2_Senrenbanka_HEADER);
	IndexHeader.ArchiveSize.LowPart = BufferSize;
	compress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
	IndexHeader.ArchiveSize.HighPart = 0;

	FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
	FileXP3.Write(lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	Offset.QuadPart = 0;
	FileXP3.Seek(Offset, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	FileXP3.Close();

	HeapFree(GetProcessHeap(), 0, lpBuffer);
	HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
	HeapFree(GetProcessHeap(), 0, pXP3Index);

	TVPExecuteScript(ttstr(L"Storages.removeAutoPath(System.exePath + \"" + ttstr(Handle->CurrentTempFileName.c_str()) + L"\" + \">\");"));

	if (Handle->CurrentTempHandle != 0 && Handle->CurrentTempHandle != INVALID_HANDLE_VALUE)
		NtClose(Handle->CurrentTempHandle);

	InterlockedExchangePointer(&(Handle->CurrentTempHandle), INVALID_HANDLE_VALUE);
	
	Status = DeleteFileW(Handle->CurrentTempFileName.c_str());
	if (NT_FAILED(Status))
	{
		wstring OutputInfo = L"Making Package : Successful!\nBut you must relaunch this game\nand delete \"";
		OutputInfo += Handle->CurrentTempFileName;
		OutputInfo += L"\" to make the next package!!!";
		MessageBoxW(Handle->MainWindow,
			OutputInfo.c_str(),
			L"KrkrExtract (Important Information!!)", MB_OK);
	}
	else
	{
		MessageBoxW(Handle->MainWindow, L"Making Package : Successful", L"KrkrExtract", MB_OK);
	}

	Handle->CurrentTempFileName = L"KrkrzTempWorker.xp3";
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI KrkrPacker::DoM2DummyPackFirst_SenrenBanka(LPCWSTR lpBasePack)
{
	NTSTATUS                     Status;
	GlobalData*                  Handle;
	NtFileDisk                   File, FileXP3;
	PBYTE                        pbIndex;
	ULONG                        BufferSize, CompressedSize, BlockSize;
	PVOID                        lpBuffer, lpCompressBuffer;
	PBYTE                        lpBlock, lpBlockCompressed;
	LARGE_INTEGER                Size, Offset, BytesTransfered, ChunkSize;
	SMyXP3IndexM2                *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER        IndexHeader;
	BYTE                         FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER             XP3Header(FirstMagic, (ULONG64)0);
	KRKRZ_M2_Senrenbanka_HEADER  SenrenBankaHeader;
	WCHAR                        CurTempFileName[MAX_PATH];
	ULONG64                      RandNum;

	Handle = GlobalData::GetGlobalData();

	RandNum = genrand64_int64();
	RtlZeroMemory(CurTempFileName, countof(CurTempFileName) * 2);
	wsprintfW(CurTempFileName, L"KrkrzTempWorker_%08x%08x.xp3", HiDword(RandNum), LoDword(RandNum));
	Handle->CurrentTempFileName = CurTempFileName;

	RtlZeroMemory(&SenrenBankaHeader, sizeof(SenrenBankaHeader));
	Status = FileXP3.Create(Handle->CurrentTempFileName.c_str());


	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Couldn't create a handle for temporary output file.", L"KrkrExtract", MB_OK);
		return Status;
	}
	
	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
	lpCompressBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedSize);
	pXP3Index = (SMyXP3IndexM2 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	if (!lpBuffer || !lpCompressBuffer || !pXP3Index)
	{
		MessageBoxW(Handle->MainWindow, L"Insufficient memory to make package!!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		FileXP3.Close();

		if(lpBuffer)         HeapFree(GetProcessHeap(), 0, lpBuffer);
		if(lpCompressBuffer) HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		if(pXP3Index)        HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);

	Offset.QuadPart = BytesTransfered.QuadPart;

	if (FileList.size() == 0)
	{
		MessageBoxW(Handle->MainWindow, L"No File to be packed", L"KrkrExtract", MB_OK);
		FileXP3.Close();
		HeapFree(GetProcessHeap(), 0, lpBuffer);
		HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
		HeapFree(GetProcessHeap(), 0, pXP3Index);
		return STATUS_UNSUCCESSFUL;
	}
	
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{

		ZeroMemory(pIndex, sizeof(*pIndex));
		*(PDWORD)(pIndex->file.Magic) = CHUNK_MAGIC_FILE;
		*(PDWORD)(pIndex->info.Magic) = CHUNK_MAGIC_INFO;
		*(PDWORD)(pIndex->time.Magic) = CHUNK_MAGIC_TIME;
		*(PDWORD)(pIndex->segm.Magic) = CHUNK_MAGIC_SEGM;
		*(PDWORD)(pIndex->adlr.Magic) = CHUNK_MAGIC_ADLR;
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C;
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;
		pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring FullName = lpBasePack;
		FullName += L"\\";
		FullName += FileList[i];
		Status = File.Open(FullName.c_str());

		if (NT_FAILED(Status))
		{
			wstring InfoW(L"Dummy write :\n[Failed to open]\n");
			InfoW += FullName;
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return Status;
		}

		File.GetSize(&Size);
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
		}

		File.Read(lpBuffer, Size.LowPart, &BytesTransfered);

		if (BytesTransfered.LowPart != Size.LowPart)
		{
			wstring InfoW(L"Dummy write :\n[Failed to read]\n");
			InfoW += FullName;
			MessageBoxW(Handle->MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			File.Close();
			FileXP3.Close();
			HeapFree(GetProcessHeap(), 0, lpBuffer);
			HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
			HeapFree(GetProcessHeap(), 0, pXP3Index);
			return STATUS_IO_DEVICE_ERROR;
		}

		pIndex->segm.segm->Offset = Offset;

		wstring DummyName, HashName;

		DummyName = FileList[i] + L".dummy";
		auto DummyLowerName = ToLowerString(DummyName.c_str());

		GenMD5Code(DummyLowerName.c_str(), SenrenBankaInfo.ProductName, HashName);
		pIndex->info.FileName = HashName;
		pIndex->info.FileNameLength = (USHORT)HashName.length();

		pIndex->adlr.Hash = M2Hash;

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.OriginalSize.LowPart = BytesTransfered.LowPart;

		FILETIME Time1, Time2;
		GetFileTime(File.GetHandle(), &(pIndex->time.FileTime), &Time1, &Time2);
		File.Close();
		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;

		if (XP3EncryptionFlag)
			pIndex->info.EncryptedFlag = 0x80000000;
		else
			pIndex->info.EncryptedFlag = 0;

		PackChunkList.push_back(*pIndex);

		pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);

		pIndex->segm.segm->bZlib = 0;

		pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered.LowPart;
		pIndex->info.ArchiveSize.LowPart = BytesTransfered.LowPart;
		Offset.QuadPart += BytesTransfered.QuadPart;

		FileXP3.Write(lpBuffer, BytesTransfered.LowPart, &BytesTransfered);
	}
	
	// generate index, calculate index size first
	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		Size.LowPart += 
			sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
			sizeof(pIndex->file.ChunkSize) + MagicLength +
			sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
			sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
			sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
	}
	
	Size.LowPart += sizeof(KRKRZ_M2_Senrenbanka_HEADER);

	if (Size.LowPart > CompressedSize)
	{
		CompressedSize = Size.LowPart;
		lpCompressBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpCompressBuffer, CompressedSize);
	}
	if (Size.LowPart * 2 > BufferSize)
	{
		BufferSize = Size.LowPart * 2;
		lpBuffer = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpBuffer, BufferSize);
	}

	lpBlock = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FileList.size() * sizeof(KRKRZ_XP3_INDEX_CHUNK_Yuzu2));
	lpBlockCompressed = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FileList.size() * sizeof(KRKRZ_XP3_INDEX_CHUNK_Yuzu2) * 2);

	BlockSize = 0;
	for (ULONG i = 0; i < FileList.size(); i++)
	{
		RtlCopyMemory((lpBlock + BlockSize), &M2SubChunkMagic, 4);
		BlockSize += 4;
		ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (FileList[i].length() + lstrlenW(L".dummy") + 1) * 2;
		RtlCopyMemory((lpBlock + BlockSize), &(ChunkSize.QuadPart), 8);
		BlockSize += 8;
		RtlCopyMemory((lpBlock + BlockSize), &M2Hash, 4);
		BlockSize += 4;

		USHORT NameLength = (USHORT)(lstrlenW(FileList[i].c_str()) + lstrlenW(L".dummy"));
		RtlCopyMemory((lpBlock + BlockSize), &NameLength, 2);
		BlockSize += 2;
		RtlCopyMemory((lpBlock + BlockSize), FileList[i].c_str(), FileList[i].size() * 2);
		BlockSize += FileList[i].size() * 2;
		RtlCopyMemory((lpBlock + BlockSize), L".dummy", (lstrlenW(L".dummy") + 1) * 2);
		BlockSize += (lstrlenW(L".dummy") + 1) * 2;
	}
	
	SenrenBankaHeader.OriginalSize = BlockSize;
	SenrenBankaHeader.Magic = Handle->M2ChunkMagic;
	SenrenBankaHeader.LengthOfProduct = SenrenBankaInfo.LengthOfProduct;
	lstrcpyW(SenrenBankaHeader.ProductName, SenrenBankaInfo.ProductName);
	SenrenBankaHeader.ArchiveSize = FileList.size() * sizeof(KRKRZ_XP3_INDEX_CHUNK_Yuzu2) * 2;
	compress2(lpBlockCompressed, &SenrenBankaHeader.ArchiveSize, lpBlock, BlockSize, Z_BEST_COMPRESSION);
	HeapFree(GetProcessHeap(), 0, lpBlock);
	SenrenBankaHeader.ChunkSize = SenrenBankaInfo.ChunkSize;
	SenrenBankaHeader.Offset   = Offset;


	if (Handle->IsSpcialChunkEncrypted && Handle->SpecialChunkDecoder)
		Handle->SpecialChunkDecoder(lpBlockCompressed, lpBlockCompressed,
		SenrenBankaHeader.ArchiveSize <= 0x100 ? SenrenBankaHeader.ArchiveSize : 0x100);

	FileXP3.Write(lpBlockCompressed, SenrenBankaHeader.ArchiveSize);
	HeapFree(GetProcessHeap(), 0, lpBlockCompressed);

	Offset.QuadPart += SenrenBankaHeader.ArchiveSize;
	XP3Header.IndexOffset = Offset;

	BlockSize = 0;
	
	*(PDWORD)(PBYTE)lpCompressBuffer = Handle->M2ChunkMagic;
	BlockSize += 4;
	*(PULONG64)((PBYTE)lpCompressBuffer + BlockSize) = SenrenBankaHeader.ChunkSize.QuadPart;
	BlockSize += 8;
	*(PULONG64)((PBYTE)lpCompressBuffer + BlockSize) = SenrenBankaHeader.Offset.QuadPart;
	BlockSize += 8;
	*(PDWORD)((PBYTE)lpCompressBuffer + BlockSize) = SenrenBankaHeader.OriginalSize;
	BlockSize += 4;
	*(PDWORD)((PBYTE)lpCompressBuffer + BlockSize) = SenrenBankaHeader.ArchiveSize;
	BlockSize += 4;
	*(PUSHORT)((PBYTE)lpCompressBuffer + BlockSize) = SenrenBankaHeader.LengthOfProduct;
	BlockSize += 2;
	RtlCopyMemory(((PBYTE)lpCompressBuffer + BlockSize), SenrenBankaHeader.ProductName, (lstrlenW(SenrenBankaHeader.ProductName) + 1) * 2);
	BlockSize += (lstrlenW(SenrenBankaHeader.ProductName) + 1) * 2;
	
	
	pIndex = pXP3Index;
	pbIndex = (PBYTE)lpCompressBuffer + BlockSize;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
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
	IndexHeader.OriginalSize.QuadPart = Size.LowPart + BlockSize - sizeof(KRKRZ_M2_Senrenbanka_HEADER);
	BufferSize = Size.LowPart + BlockSize - sizeof(KRKRZ_M2_Senrenbanka_HEADER);
	IndexHeader.ArchiveSize.LowPart = BufferSize;
	compress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
	IndexHeader.ArchiveSize.HighPart = 0;

	FileXP3.Write(&IndexHeader, sizeof(IndexHeader), &BytesTransfered);
	FileXP3.Write(lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered);
	Offset.QuadPart = 0;
	FileXP3.Seek(Offset, FILE_BEGIN);
	FileXP3.Write(&XP3Header, sizeof(XP3Header), &BytesTransfered);
	FileXP3.Close();

	HeapFree(GetProcessHeap(), 0, lpBuffer);
	HeapFree(GetProcessHeap(), 0, lpCompressBuffer);
	HeapFree(GetProcessHeap(), 0, pXP3Index);

	
	return STATUS_SUCCESS;
}

/*********************************************************************/


DWORD WINAPI PackerThread(PVOID lpParam)
{
	NTSTATUS      Status;
	GlobalData*   Handle;
	WCHAR         BasePackName[MAX_PATH];
	WCHAR         GuessPackName[MAX_PATH];
	WCHAR         OutPackName[MAX_PATH];

	Handle = GlobalData::GetGlobalData();

	RtlZeroMemory(GuessPackName, countof(GuessPackName) * sizeof(WCHAR));
	RtlZeroMemory(BasePackName,  countof(BasePackName)  * sizeof(WCHAR));
	RtlZeroMemory(OutPackName,   countof(OutPackName)   * sizeof(WCHAR));

	Handle->GetGuessPack(GuessPackName, MAX_PATH);
	Handle->GetOutputPack(OutPackName,  MAX_PATH);
	Handle->GetFolder(BasePackName,     MAX_PATH);

	Status = LocalKrkrPacker->DetactPackFormat(GuessPackName);
	if (NT_FAILED(Status))
	{
		LocalKrkrPacker->InternalReset();
		return Status;
	}

	switch (LocalKrkrPacker->KrkrPackType)
	{
	case PackInfo::NormalPack:
		Status = LocalKrkrPacker->DoNormalPack(BasePackName, GuessPackName, OutPackName);
		break;

	case PackInfo::NormalPack_NoExporter:
		Status = LocalKrkrPacker->DoNormalPackEx(BasePackName, GuessPackName, OutPackName);
		break;

	case PackInfo::KrkrZ:
		Status = LocalKrkrPacker->DoM2Pack(BasePackName, GuessPackName, OutPackName);
		break;

	case PackInfo::KrkrZ_V2:
		Status = LocalKrkrPacker->DoM2Pack_Version2(BasePackName, GuessPackName, OutPackName);
		break;

	case PackInfo::KrkrZ_SenrenBanka:
		Status = LocalKrkrPacker->DoM2Pack_SenrenBanka(BasePackName, GuessPackName, OutPackName);
		break;
	}

	LocalKrkrPacker->InternalReset();
	return Status;
}

HANDLE NTAPI StartPacker()
{
	NTSTATUS     Status;
	GlobalData*  Handle;
	WCHAR        BasePack[MAX_PATH];

	Handle = GlobalData::GetGlobalData();
	
	RtlZeroMemory(BasePack, countof(BasePack) * sizeof(WCHAR));
	Handle->DisableAll(Handle->MainWindow);

	if (LocalKrkrPacker == NULL)
		LocalKrkrPacker = new KrkrPacker;

	LocalKrkrPacker->InternalReset();
	if (Handle->isRunning || Handle->WorkerThread != INVALID_HANDLE_VALUE)
	{
		MessageBoxW(Handle->MainWindow, L"Another task is under processing!", L"KrkrExtract", MB_OK);
		LocalKrkrPacker->InternalReset();
		return INVALID_HANDLE_VALUE;
	}

	GlobalData::GetGlobalData()->isRunning = TRUE;

	DeleteFileW(L"KrkrzTempWorker.xp3");

	Handle->GetFolder(BasePack, countof(BasePack));

	LocalKrkrPacker->Init();
	

	ULONG Attr = GetFileAttributesW(BasePack);
	if (!(Attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		MessageBoxW(Handle->MainWindow, L"Couldn't regard the target path as a directory", L"KrkrExtract", MB_OK);
		GlobalData::GetGlobalData()->isRunning = FALSE;
		LocalKrkrPacker->InternalReset();
		return INVALID_HANDLE_VALUE;
	}

	Status = Nt_CreateThread(PackerThread, NULL, FALSE, NtCurrentProcess(), &LocalKrkrPacker->hThread);
	if (NT_FAILED(Status))
	{
		MessageBoxW(Handle->MainWindow, L"Cannot launch packer thread", L"KrkrExtract", MB_OK);
		GlobalData::GetGlobalData()->isRunning = FALSE;
		LocalKrkrPacker->InternalReset();
		return INVALID_HANDLE_VALUE;
	}
	return LocalKrkrPacker->hThread;
}

