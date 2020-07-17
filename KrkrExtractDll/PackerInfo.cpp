#include "PackerInfo.h"

static WCHAR* PackingFormatString = L"Packing[%d/%d]";

PackInfo::PackInfo(HWND _WinMain, const WCHAR* WinText) :
	PackInformation(PackInfo::UnknownPack),
	M2Hash(0),
	DecryptionKey(0),
	pfProc(nullptr),
	XP3EncryptionFlag(TRUE),
	pIndexBuffer(nullptr),
	MainWindow(nullptr),
	hThread((HANDLE)-1),
	ThreadId(0),
	InfoNameZeroEnd(TRUE),
	M2NameZeroEnd(TRUE)
{
	RtlZeroMemory(M2ChunkMagic, sizeof(M2ChunkMagic));
	this->MainWindow = _WinMain;

	mWinText = WinText;
	//pfProc = GetExtractionFilter();
}

#define GetItemX(x)    GetDlgItem(hWnd, x)
#define EnableX(x)     EnableWindow(x, TRUE)
#define DisableX(x)    EnableWindow(x, FALSE)
#define CheckItem(x)   SendMessageW(x, BM_SETCHECK, BST_UNCHECKED, 0)
#define UnCheckItem(x) SendMessageW(x, BM_SETCHECK, BST_CHECKED, 0)

HRESULT WINAPI PackInfo::EnableAll(HWND hWnd)
{
	HWND hPNGRaw = GetItemX(IDC_PNG_RAW);
	HWND hPNGSys = GetItemX(IDC_PNG_SYSTEM);

	HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
	HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
	HWND hPSBText = GetItemX(IDC_PSB_TEXT);
	HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
	HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
	HWND hPSBAll = GetItemX(IDC_PSB_ALL);

	HWND hTXTRaw = GetItemX(IDC_TEXT_RAW);
	HWND hTXTText = GetItemX(IDC_TEXT_TEXT);

	HWND hTLGRaw = GetItemX(IDC_TLG_RAW);
	HWND hTLGBuildin = GetItemX(IDC_TLG_BUILDIN);
	HWND hTLGSys = GetItemX(IDC_TLG_SYSTEM);
	HWND hTLGPng = GetItemX(IDC_TLG_PNG);

	HWND hFolderEdit = GetItemX(IDC_PACK_EDIT_FOLDER);
	HWND hOriPackEdit = GetItemX(IDC_PACK_EDIT_ORIPACK);
	HWND hOutPackEdit = GetItemX(IDC_PACK_EDIT_OUTPACK);

	HWND hFolderSel = GetItemX(IDC_PACK_BUTTON_FOLDER);
	HWND hOriPackSel = GetItemX(IDC_PACK_BUTTON_ORIPACK);
	HWND hOutPackSel = GetItemX(IDC_PACK_BUTTON_OUTPACK);

	HWND hDoPack = GetItemX(IDC_PACK_BUTTON_MAKE);
	HWND hProcess = GetItemX(IDC_PROGRESS1);
	HWND hDbg = GetItemX(IDC_BUTTON_DEBUGGER);

	EnableX(hPNGRaw);
	EnableX(hPNGSys);

	EnableX(hPSBRaw);
	EnableX(hPSBDecom);
	EnableX(hPSBText);
	EnableX(hPSBImage);
	EnableX(hPSBAnm);
	EnableX(hPSBAll);

	EnableX(hTXTRaw);
	EnableX(hTXTText);

	EnableX(hTLGRaw);
	EnableX(hTLGBuildin);
	EnableX(hTLGSys);
	EnableX(hTLGPng);

	EnableX(hFolderEdit);
	EnableX(hOriPackEdit);
	EnableX(hOutPackEdit);

	EnableX(hFolderSel);
	EnableX(hOriPackSel);
	EnableX(hOutPackSel);

	EnableX(hDoPack);
	return S_OK;
}

HRESULT WINAPI PackInfo::DisableAll(HWND hWnd)
{
	HWND hPNGRaw = GetItemX(IDC_PNG_RAW);
	HWND hPNGSys = GetItemX(IDC_PNG_SYSTEM);

	HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
	HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
	HWND hPSBText = GetItemX(IDC_PSB_TEXT);
	HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
	HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
	HWND hPSBAll = GetItemX(IDC_PSB_ALL);

	HWND hTXTRaw = GetItemX(IDC_TEXT_RAW);
	HWND hTXTText = GetItemX(IDC_TEXT_TEXT);

	HWND hTLGRaw = GetItemX(IDC_TLG_RAW);
	HWND hTLGBuildin = GetItemX(IDC_TLG_BUILDIN);
	HWND hTLGSys = GetItemX(IDC_TLG_SYSTEM);
	HWND hTLGPng = GetItemX(IDC_TLG_PNG);

	HWND hFolderEdit = GetItemX(IDC_PACK_EDIT_FOLDER);
	HWND hOriPackEdit = GetItemX(IDC_PACK_EDIT_ORIPACK);
	HWND hOutPackEdit = GetItemX(IDC_PACK_EDIT_OUTPACK);

	HWND hFolderSel = GetItemX(IDC_PACK_BUTTON_FOLDER);
	HWND hOriPackSel = GetItemX(IDC_PACK_BUTTON_ORIPACK);
	HWND hOutPackSel = GetItemX(IDC_PACK_BUTTON_OUTPACK);

	HWND hDoPack = GetItemX(IDC_PACK_BUTTON_MAKE);
	HWND hProcess = GetItemX(IDC_PROGRESS1);
	HWND hDbg = GetItemX(IDC_BUTTON_DEBUGGER);

	DisableX(hPNGRaw);
	DisableX(hPNGSys);

	DisableX(hPSBRaw);
	DisableX(hPSBDecom);
	DisableX(hPSBText);
	DisableX(hPSBImage);
	DisableX(hPSBAnm);
	DisableX(hPSBAll);

	DisableX(hTXTRaw);
	DisableX(hTXTText);

	DisableX(hTLGRaw);
	DisableX(hTLGBuildin);
	DisableX(hTLGSys);
	DisableX(hTLGPng);

	DisableX(hFolderEdit);
	DisableX(hOriPackEdit);
	DisableX(hOutPackEdit);

	DisableX(hFolderSel);
	DisableX(hOriPackSel);
	DisableX(hOutPackSel);

	DisableX(hDoPack);
	return S_OK;
}

//RtlHeapAlloc
HRESULT WINAPI PackInfo::ForceInit(HWND _WinMain, const WCHAR* WinText)
{
	PackInformation = PackInfo::UnknownPack;
	M2Hash = 0;
	DecryptionKey = 0;
	pfProc = nullptr;
	XP3EncryptionFlag = TRUE;
	pIndexBuffer = nullptr;
	MainWindow = nullptr;
	RtlZeroMemory(M2ChunkMagic, sizeof(M2ChunkMagic));
	this->MainWindow = _WinMain;
	hThread = INVALID_HANDLE_VALUE;
	ThreadId = 0;
	InfoNameZeroEnd = TRUE;
	M2NameZeroEnd = TRUE;

	pfProc = GlobalData::GetGlobalData()->pfGlobalXP3Filter;

	//ToDo: 妈蛋 我还是别用英语装逼了
	/*
	krkrz中的加密className都是cxdec
	*/

	WCHAR Info2[260] = { 0 };
	wsprintfW(Info2, L"Filter Addr : 0x%08x\n", pfProc);
	OutputString(Info2);

	mWinText = WinText;
	return S_OK;
}

PackInfo::~PackInfo()
{
	if (pIndexBuffer)
	{
		CMem::Free(pIndexBuffer);
		pIndexBuffer = nullptr;
	}
}

//outdated
VOID WINAPI PackInfo::DecryptWorkerM2(ULONG EncryptOffset, PBYTE pBuffer, ULONG BufferSize, ULONG Hash, BYTE Key)
{
	DWORD iPos = 0;
	for (iPos = 0; iPos < BufferSize; iPos++)
	{
		pBuffer[iPos] ^= Key;
	}
}

VOID WINAPI PackInfo::DecryptWorker(ULONG EncryptOffset, PBYTE pBuffer, ULONG BufferSize, ULONG Hash)
{
	tTVPXP3ExtractionFilterInfo Info(0, pBuffer, BufferSize, Hash);
	if (pfProc != nullptr)
	{
		pfProc(&Info);
	}
}

HRESULT WINAPI PackInfo::DetactPackFormat(const WCHAR* lpFileName)
{
	WinFile File;
	if (File.Open(lpFileName, WinFile::FileRead) != S_OK)
	{
		wstring Info(L"Couldn't open : \n");
		Info += lpFileName;
		Info += L"\nFor Guessing XP3 Package Type!";
		MessageBoxW(NULL, Info.c_str(), L"KrkrExtract", MB_OK);
		return S_OK;
	}
	
	ULONG                   Count = 0;
	KRKR2_XP3_HEADER        XP3Header;
	KRKR2_XP3_DATA_HEADER   DataHeader;
	tjs_uint8               *indexdata = nullptr;
	DWORD                   nRet = 0;
	ULONG                   IndexSectionSize = 0;
	HRESULT                 Status;
	LARGE_INTEGER           BeginOffset;
	WCHAR                   OutputInfo[MAX_PATH] = { 0 };

	BeginOffset.QuadPart = (ULONG64)0;

	if(File.Read((PBYTE)(&XP3Header), sizeof(XP3Header)) != S_OK)
	{
		MessageBoxW(NULL, L"Invalid XP3 Packer\n", L"KrkrExtract", MB_OK);
		File.Release();
		return S_FALSE;
	}

	//Exe Built-in Package Support
	if ((*(PUSHORT)XP3Header.Magic) == IMAGE_DOS_SIGNATURE)
	{
		Status = FindEmbededXp3OffsetSlow(lpFileName, &BeginOffset);

		if (!KERNEL32_SUCCESS(Status))
		{
			MessageBoxW(NULL, L"No a Built-in Package\n", L"KrkrExtract", MB_OK);
			File.Release();
			return S_FALSE;
		}

		File.Seek(BeginOffset.LowPart, FILE_BEGIN);
		File.Read((PBYTE)(&XP3Header), sizeof(XP3Header));
	}
	else
	{
		BeginOffset.QuadPart = 0;
	}

	if (RtlCompareMemory(StaticXP3V2Magic, XP3Header.Magic, sizeof(StaticXP3V2Magic)) != sizeof(StaticXP3V2Magic))
	{
		MessageBoxW(NULL, L"No a XP3 Package!\n", L"KrkrExtract", MB_OK);
		File.Release();
		return S_FALSE;
	}

	ULONG64 CompresseBufferSize = 0x1000;
	ULONG64 DecompressBufferSize = 0x1000;
	PBYTE pCompress = (PBYTE)HeapAlloc(GetProcessHeap(), 0, (ULONG)CompresseBufferSize);
	PBYTE pDecompress = (PBYTE)HeapAlloc(GetProcessHeap(), 0, (ULONG)DecompressBufferSize);
	DataHeader.OriginalSize = XP3Header.IndexOffset;


	ULONG Result = PackInfo::UnknownPack;
	do
	{
		LARGE_INTEGER Offset;

		Offset.QuadPart = DataHeader.OriginalSize.QuadPart + BeginOffset.QuadPart;
		File.Seek(Offset.LowPart, FILE_BEGIN);
		if (File.Read((PBYTE)(&DataHeader), sizeof(DataHeader)) != S_OK)
		{
			MessageBoxW(NULL, L"Couldn't Read Index Header\n", L"KrkrExtract", MB_OK);
			File.Release();
			return S_FALSE;
		}

		if (DataHeader.ArchiveSize.HighPart != 0 || DataHeader.ArchiveSize.LowPart == 0)
		{
			continue;
		}

		if (DataHeader.ArchiveSize.LowPart > CompresseBufferSize)
		{
			CompresseBufferSize = DataHeader.ArchiveSize.LowPart;
			pCompress = (PBYTE)HeapReAlloc(GetProcessHeap(), 0, pCompress, (ULONG)CompresseBufferSize);
		}

		if ((DataHeader.bZlib & 7) == 0)
		{
			Offset.QuadPart = -8;
			File.Seek(Offset.LowPart, FILE_CURRENT);
		}

		File.Read(pCompress, DataHeader.ArchiveSize.LowPart);
		BOOL EncodeMark = DataHeader.bZlib & 7;

		if (EncodeMark == FALSE)
		{
			if (DataHeader.ArchiveSize.LowPart > DecompressBufferSize)
			{
				DecompressBufferSize = DataHeader.ArchiveSize.LowPart;
				pDecompress = (PBYTE)HeapReAlloc(GetProcessHeap(), 0, pDecompress, (ULONG)DecompressBufferSize);
			}
			CopyMemory(pDecompress, pCompress, DataHeader.ArchiveSize.LowPart);
			DataHeader.OriginalSize.LowPart = DataHeader.ArchiveSize.LowPart;
		}
		else if (EncodeMark == TRUE)
		{
			if (DataHeader.OriginalSize.LowPart > DecompressBufferSize)
			{
				DecompressBufferSize = DataHeader.OriginalSize.LowPart;
				pDecompress = (PBYTE)HeapReAlloc(GetProcessHeap(), 0, pDecompress, (ULONG)DecompressBufferSize);
			}

			DataHeader.OriginalSize.HighPart = DataHeader.OriginalSize.LowPart;
			if (uncompress((PBYTE)pDecompress, (PULONG)&DataHeader.OriginalSize.HighPart,
				(PBYTE)pCompress, DataHeader.ArchiveSize.LowPart) == Z_OK)
			{
				DataHeader.OriginalSize.LowPart = DataHeader.OriginalSize.HighPart;
			}
		}
		else
		{
			DataHeader.bZlib = 0;
			File.Release();
			return S_FALSE;
		}

		Result = InitIndex(pDecompress, DataHeader.OriginalSize.LowPart);
		if (Result == PackInfo::UnknownPack)
		{
			HeapFree(GetProcessHeap(), 0, pCompress);
			HeapFree(GetProcessHeap(), 0, pDecompress);
			File.Release();

			MessageBoxW(NULL, L"Unknown Type Pack -- Inner Error", L"KrkrExtract", MB_OK);
			this->PackInformation = PackInfo::UnknownPack;
			return S_FALSE;
		}
		else
		{
			if (Result == NormalPack)
			{
				this->PackInformation = NormalPack;
				goto DetactEnd;
			}
			else if (Result == KrkrZ)
			{
				this->PackInformation = KrkrZ;

				WCHAR Info[260] = { 0 };
				wsprintfW(Info, L"SunChunk : %c%c%c%c", M2ChunkMagic[0], M2ChunkMagic[1], M2ChunkMagic[2], M2ChunkMagic[3]);
				OutputString(Info);

				InitIndexForM2(pDecompress, DataHeader.OriginalSize.LowPart);
				
				goto DetactEnd;
			}
			else if (Result == KrkrZ_V2)
			{
				this->PackInformation = KrkrZ_V2;
				//Chunk ok
				goto DetactEnd;
			}
		}

	} while (DataHeader.bZlib & 0x80);

	//

DetactEnd:
	HeapFree(GetProcessHeap(), 0, pCompress);
	HeapFree(GetProcessHeap(), 0, pDecompress);
	File.Release();

#if 0
	if (this->PackInformation == NormalPack)
	{
		MessageBoxW(NULL, L"Krkr2", L"info", MB_OK);
	}
	else
	{
		MessageBoxW(NULL, L"Krkrz", L"info", MB_OK);
	}
#endif

	return S_OK;
}


ULONG WINAPI PackInfo::InitIndexForM2(PBYTE pDecompress, ULONG Size)
{
	BOOL First = TRUE;
	OutputString(L"Rebuild M2 File Info\n");
	ULONG PtrOffset = 0;
	ULONG PackType = PackInfo::UnknownPack;
#define PtrInc(x) PtrOffset += (ULONG)x

	while (PtrOffset < Size)
	{
		BOOL          NotAdd = FALSE;
		XP3Index      item;

		if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_YUZU, 4) == 4)
		{
			PtrOffset += 4;
			PtrOffset += 8;
			item.isM2Format = TRUE;
			ULONG HashInfo = 0;
			CopyMemory(&HashInfo, (pDecompress + PtrOffset), 4);
			item.yuzu.Hash = HashInfo;
			PtrOffset += 4;
			USHORT FileNameLen = 0;
			CopyMemory(&FileNameLen, (pDecompress + PtrOffset), 2);
			item.yuzu.Len = FileNameLen;
			PtrOffset += 2;
			wstring M2FileName((WCHAR*)(pDecompress + PtrOffset), FileNameLen);
			item.yuzu.Name = M2FileName;

			WCHAR ZeroEnd = *(WCHAR*)(pDecompress + PtrOffset + FileNameLen * 2);
			if (!ZeroEnd)
			{
				PtrOffset += (FileNameLen + 1) * 2;
			}
			else
			{
				M2NameZeroEnd = FALSE;
				PtrOffset += (FileNameLen)* 2;
			}

			OutputString(L"[M2 file]");
			OutputString(item.yuzu.Name.c_str());
			OutputString(L"\n");

			//RtlCopyMemory(M2ChunkMagic, CHUNK_MAGIC_YUZU, 4);

		}
		else if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_M2, 4) == 4 ||
			RtlCompareMemory(pDecompress + PtrOffset, M2ChunkMagic, 4) == 4)
		{
			PtrOffset += 4;
			PtrOffset += 8;
			item.isM2Format = TRUE;
			ULONG HashInfo = 0;
			CopyMemory(&HashInfo, (pDecompress + PtrOffset), 4);
			item.yuzu.Hash = HashInfo;
			PtrOffset += 4;
			USHORT FileNameLen = 0;
			CopyMemory(&FileNameLen, (pDecompress + PtrOffset), 2);
			item.yuzu.Len = FileNameLen;
			PtrOffset += 2;
			wstring M2FileName((WCHAR*)(pDecompress + PtrOffset), FileNameLen);
			item.yuzu.Name = M2FileName;

			WCHAR ZeroEnd = *(WCHAR*)(pDecompress + PtrOffset + FileNameLen * 2);
			if (!ZeroEnd)
			{
				PtrOffset += (FileNameLen + 1) * 2;
			}
			else
			{
				M2NameZeroEnd = FALSE;
				PtrOffset += (FileNameLen) * 2;
			}

			OutputString(L"[M2 file]");
			OutputString(item.yuzu.Name.c_str());
			OutputString(L"\n");

			//RtlCopyMemory(M2ChunkMagic, CHUNK_MAGIC_KRKRZ_M2, 4);

			pIndexBuffer = (PBYTE)CMem::Alloc(Size);
			RtlCopyMemory(pIndexBuffer, pDecompress, Size);
		}

		if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_FILE, 4) != 4)
		{
			return PackInfo::UnknownPack;
		}
		//File Maigc
		PtrInc(4);
		ULONG64 FileChunkSize64 = 0;
		CopyMemory(&FileChunkSize64, (PtrOffset + pDecompress), 8);
		PtrInc(8);

		ULONG SavePtrOffset = PtrOffset;
		ULONG32 FileChunkSize32 = (ULONG32)FileChunkSize64;
		ULONG iPos = 0;

#define iPosAdd(x) iPos+=(ULONG)x

		while (iPos < FileChunkSize32)
		{
			if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_INFO, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.info.EncryptedFlag), (PtrOffset + pDecompress), 4);
				PtrInc(4);
				iPosAdd(4);
				CopyMemory(&(item.info.OriginalSize), (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.info.ArchiveSize), (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.info.FileNameLength), (PtrOffset + pDecompress), 2);
				PtrInc(2);
				iPosAdd(2);
				wstring WideFileName((WCHAR*)(PtrOffset + pDecompress), item.info.FileNameLength);

				if (!wcsncmp(item.info.FileName.c_str(), L"$$$", 3))
				{
					PtrOffset = SavePtrOffset;
					PtrOffset += FileChunkSize32;
					iPos = FileChunkSize32;
					NotAdd = TRUE;
				}

				if (item.info.OriginalSize.LowPart == 0xFFFFFFFF)
				{
					PtrOffset = SavePtrOffset;
					PtrOffset += FileChunkSize32;
					iPos = FileChunkSize32;
					NotAdd = TRUE;
				}

				WCHAR ZeroEnd = *(WCHAR*)(PtrOffset + pDecompress + item.info.FileNameLength * 2);
				if (!ZeroEnd)
				{
					PtrInc((item.info.FileNameLength + 1) * 2);
					iPosAdd((item.info.FileNameLength + 1) * 2);
				}
				else
				{
					InfoNameZeroEnd = FALSE;
					PtrInc((item.info.FileNameLength) * 2);
					iPosAdd((item.info.FileNameLength) * 2);
				}

				item.info.FileName = WideFileName;

				if (item.info.EncryptedFlag)
				{
					XP3EncryptionFlag = TRUE;
				}
				else
				{
					XP3EncryptionFlag = FALSE;
				}
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_SEGM, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_ADLR, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);
				CopyMemory(&FileChunkSize64, (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.adlr.Hash), (PtrOffset + pDecompress), 4);
				PtrInc(4);
				iPosAdd(4);
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_TIME, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
			else
			{
				PtrInc(4);
				iPosAdd(4);

				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
		}
		//Scan Normal Chunk Block

		if (!NotAdd)
		{
			M2ChunkList.push_back(item);
			if (First)
			{
				M2Hash = item.adlr.Hash;
				First = FALSE;
			}
		}
	}
	return PackInfo::NormalPack;
}

ULONG WINAPI PackInfo::InitIndex(PBYTE pDecompress, ULONG Size)
{
	ULONG PtrOffset = 0;
	ULONG PackType = PackInfo::UnknownPack;

	if (!IsCompatXP3(pDecompress, Size))
	{
		return KrkrZ_V2;
	}

	OutputString(L"Scanf Index\n");
#define PtrInc(x) PtrOffset += (ULONG)x

	while (PtrOffset < Size)
	{
		BOOL     NotAdd = FALSE;
		XP3Index item;

		WCHAR InfoW[500] = { 0 };
		wsprintfW(InfoW, L"Start Ptr : %08x\n", PtrOffset);
		OutputString(InfoW);

		if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_YUZU, 4) == 4)
		{

			wsprintfW(InfoW, L"M2Chunk Ptr : %08x\n", PtrOffset);
			OutputString(InfoW);

			PtrOffset += 4;
			PtrOffset += 8;
			item.isM2Format = TRUE;
			ULONG HashInfo = 0;
			CopyMemory(&HashInfo, (pDecompress + PtrOffset), 4);
			item.yuzu.Hash = HashInfo;
			PtrOffset += 4;
			USHORT FileNameLen = 0;
			CopyMemory(&FileNameLen, (pDecompress + PtrOffset), 2);
			item.yuzu.Len = FileNameLen;
			PtrOffset += 2;
			wstring M2FileName((WCHAR*)(pDecompress + PtrOffset), FileNameLen);
			item.yuzu.Name = M2FileName;
			
			WCHAR ZeroEnd = *(WCHAR*)(pDecompress + PtrOffset + FileNameLen * 2);
			if (!ZeroEnd)
			{
				PtrOffset += (FileNameLen + 1) * 2;
			}
			else
			{
				PtrOffset += (FileNameLen)* 2;
			}

			RtlCopyMemory(M2ChunkMagic, CHUNK_MAGIC_YUZU, 4);

			pIndexBuffer = (PBYTE)CMem::Alloc(Size);
			RtlCopyMemory(pIndexBuffer, pDecompress, Size);
			

			//MessageBoxW(NULL, L"Return as Krkrz", NULL, MB_OK);
			return PackInfo::KrkrZ;
		}
		else
		{
			if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_M2, 4) == 4)
			{

				wsprintfW(InfoW, L"M2Chunk Ptr : %08x\n", PtrOffset);
				OutputString(InfoW);
				
				PtrOffset += 4;
				PtrOffset += 8;
				item.isM2Format = TRUE;
				ULONG HashInfo = 0;
				CopyMemory(&HashInfo, (pDecompress + PtrOffset), 4);
				item.yuzu.Hash = HashInfo;
				PtrOffset += 4;
				USHORT FileNameLen = 0;
				CopyMemory(&FileNameLen, (pDecompress + PtrOffset), 2);
				item.yuzu.Len = FileNameLen;
				PtrOffset += 2;
				wstring M2FileName((WCHAR*)(pDecompress + PtrOffset), FileNameLen);
				item.yuzu.Name = M2FileName;
				
				WCHAR ZeroEnd = *(WCHAR*)(pDecompress + PtrOffset + FileNameLen * 2);
				if (!ZeroEnd)
				{
					PtrOffset += (FileNameLen + 1) * 2;
				}
				else
				{
					PtrOffset += (FileNameLen)* 2;
				}

				RtlCopyMemory(M2ChunkMagic, CHUNK_MAGIC_KRKRZ_M2, 4);

				pIndexBuffer = (PBYTE)CMem::Alloc(Size);
				RtlCopyMemory(pIndexBuffer, pDecompress, Size);

				//MessageBoxW(NULL, L"Return as Krkrz", NULL, MB_OK);
				return KrkrZ;
			}
		}

		//做M2 Pre-Analysis的时候，如果这里找不到File Magic
		//可能是M2的Special Sub-Chunk Magic 先分析是否符合特征

		if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_FILE, 4) != 4)
		{

			BOOL isSubChunk = FALSE;
			//Pre
			ULONG SavedOffset = PtrOffset;
			ULONG iOffset = 0;
			UCHAR TempMaigcInfo[4];
			RtlCopyMemory(TempMaigcInfo, (pDecompress + SavedOffset), 4);
			SavedOffset += 4;
			ULONG64 TempChunkSize = 0;
			RtlCopyMemory(&TempChunkSize, (pDecompress + SavedOffset), 8);
			SavedOffset += 8;
			if (TempChunkSize >= 4)
			{
				SavedOffset += 4;
				iOffset += 4;
				if (TempChunkSize > iOffset + 2)
				{
					USHORT DummyLength = 0;
					RtlCopyMemory(&DummyLength, (pDecompress + SavedOffset), 2);
					iOffset += 2;
					SavedOffset += 2;

					if (*(WCHAR*)(pDecompress + SavedOffset + DummyLength * 2) == 0)
					{
						isSubChunk = TRUE;
						RtlCopyMemory(M2ChunkMagic, TempMaigcInfo, 4);
						return PackInfo::KrkrZ;
					}
				}
			}

			if (!isSubChunk)
			{
				wsprintfW(InfoW, L"File Error Ptr : %08x\n", PtrOffset);
				OutputString(InfoW);

				MessageBoxW(MainWindow, L"No File Magic", L"KrkrExtract", MB_OK);

				return PackInfo::UnknownPack;
			}
		}
		//File Maigc
		PtrInc(4);
		ULONG64 FileChunkSize64 = 0;
		CopyMemory(&FileChunkSize64, (PtrOffset + pDecompress), 8);
		PtrInc(8);

		ULONG SavePtrOffset = PtrOffset;
		ULONG32 FileChunkSize32 = (ULONG32)FileChunkSize64;
		ULONG iPos = 0;

#define iPosAdd(x) iPos+=(ULONG)x

		while (iPos < FileChunkSize32)
		{
			if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_INFO, 4) == 4)
			{
				wsprintfW(InfoW, L"Info Ptr : %08x\n", PtrOffset);
				OutputString(InfoW);

				PtrInc(4);
				iPosAdd(4);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.info.EncryptedFlag), (PtrOffset + pDecompress), 4);
				PtrInc(4);
				iPosAdd(4);
				CopyMemory(&(item.info.OriginalSize), (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.info.ArchiveSize), (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.info.FileNameLength), (PtrOffset + pDecompress), 2);
				PtrInc(2);
				iPosAdd(2);
				wstring WideFileName((WCHAR*)(PtrOffset + pDecompress), item.info.FileNameLength);

				WCHAR ZeroEnd = *(WCHAR*)(PtrOffset + pDecompress + item.info.FileNameLength * 2);
				if (!ZeroEnd)
				{
					PtrInc((item.info.FileNameLength + 1) * 2);
					iPosAdd((item.info.FileNameLength + 1) * 2);
				}
				else
				{
					PtrInc((item.info.FileNameLength) * 2);
					iPosAdd((item.info.FileNameLength) * 2);
				}

				item.info.FileName = WideFileName;

				if (!wcsncmp(item.info.FileName.c_str(), L"$$$", 3))
				{
					PtrOffset = SavePtrOffset;
					PtrOffset += FileChunkSize32;
					iPos = FileChunkSize32;
					NotAdd = TRUE;
				}

				if (item.info.OriginalSize.LowPart == 0xFFFFFFFF)
				{
					PtrOffset = SavePtrOffset;
					PtrOffset += FileChunkSize32;
					iPos = FileChunkSize32;
					NotAdd = TRUE;
				}

				if (item.info.EncryptedFlag)
				{
					XP3EncryptionFlag = TRUE;
				}
				else
				{
					XP3EncryptionFlag = FALSE;
				}
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_SEGM, 4) == 4)
			{
				wsprintfW(InfoW, L"Segm Ptr : %08x\n", PtrOffset);
				OutputString(InfoW);

				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_ADLR, 4) == 4)
			{
				wsprintfW(InfoW, L"Adlr Ptr : %08x\n", PtrOffset);
				OutputString(InfoW);

				PtrInc(4);
				iPosAdd(4);
				CopyMemory(&FileChunkSize64, (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.adlr.Hash), (PtrOffset + pDecompress), 4);
				PtrInc(4);
				iPosAdd(4);
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_TIME, 4) == 4)
			{
				wsprintfW(InfoW, L"Time Ptr : %08x\n", PtrOffset);
				OutputString(InfoW);

				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
			else
			{

				wsprintfW(InfoW, L"Unknown Ptr : %08x\n", PtrOffset);
				OutputString(InfoW);

				//Try to find ExtraChunk Automatically
				//Magic
				CHAR TempMagic[4];
				RtlCopyMemory(TempMagic, PtrOffset + pDecompress, 4);
				PtrInc(4);
				iPosAdd(4);

				ULONG SaveOffset = PtrOffset;
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				//ChunkSize
				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);

				/*****************/
				SaveOffset += 8;
				if (SaveOffset + 4 <= PtrOffset)
				{
					SaveOffset += 4;
					if (SaveOffset + 2 <= PtrOffset)
					{
						USHORT StringLength = 0;
						RtlCopyMemory(&StringLength, (SaveOffset + pDecompress), 2);
						SaveOffset += 2;
						WCHAR* Start = (WCHAR*)(SaveOffset + pDecompress);
						if ((TempChunkSize == sizeof(USHORT)+sizeof(ULONG)+(StringLength + 1) * 2) &&
							*(WCHAR*)(SaveOffset + pDecompress + StringLength*2) == 0)
						{
							RtlCopyMemory(M2ChunkMagic, TempMagic, 4);

							pIndexBuffer = (PBYTE)CMem::Alloc(Size);
							RtlCopyMemory(pIndexBuffer, pDecompress, Size);
							return PackInfo::KrkrZ;
						}
					}
				}
			}
			//Enum Normal Chunk
		}
		//Scan Normal Chunk Block

		wsprintfW(InfoW, L"End Ptr : %08x\n", PtrOffset);
		OutputString(InfoW);
	}
	return PackInfo::NormalPack;
}


HRESULT WINAPI PackInfo::Reset()
{
	mBasePack.clear();
	mGuessPackage.clear();
	mOutName.clear();

	EnableAll(MainWindow);

	M2ChunkList.clear();
	DeleteFileW(L"KrkrzTempWorker.xp3");
	SetWindowTextW(MainWindow, mWinText.c_str());
	HWND hProcess = GetDlgItem(MainWindow, IDC_PROGRESS1);
	SendMessageW(hProcess, PBM_SETPOS, 0, 0);
	GlobalData::GetGlobalData()->isRunning = FALSE;
	return S_OK;
}

DWORD WINAPI PackerThread(PVOID lpParam)
{
	PackInfo* o = static_cast<PackInfo*>(lpParam);

	wstring Info;
	Info.clear();
	Info = L"Packer Debug2 :\n";
	Info += L"BasePack : ";
	Info += o->mBasePack;
	Info += L"\nGuessPackage : ";
	Info += o->mGuessPackage;
	Info += L"\nOutName : ";
	Info += o->mOutName;
	Info += L"\n";
	OutputString(Info.c_str());

	o->DisableAll(o->MainWindow);
	if (o->DetactPackFormat(o->mGuessPackage.c_str()) != S_OK)
	{
		OutputString(L"Unknown Package Type\n");
		return S_FALSE;
	}

	HRESULT result; 

	if (o->PackInformation == PackInfo::KrkrZ)
	{
		OutputString(L"Packer Mode : M2\n");
		//MessageBoxW(NULL, L"M2", L"", MB_OK);
		result = o->DoM2PackV2(o->mBasePack.c_str(), o->mGuessPackage.c_str(), 
			o->mOutName.c_str(), o->PackInformation);
	}
	else if (o->PackInformation == PackInfo::NormalPack)
	{
		OutputString(L"Packer Mode : Normal\n");
		//MessageBoxW(NULL, L"Normal", L"", MB_OK);
		result = o->DoNormalPack(o->mBasePack.c_str(), o->mGuessPackage.c_str(),
			o->mOutName.c_str());
	}
	else if (o->PackInformation == PackInfo::KrkrZ_V2)
	{
		OutputString(L"Packer Mode : M2 Ver2\n");
		//MessageBoxW(NULL, L"M2 V2", L"", MB_OK);
		result = o->DoM2PackV2_Version2(o->mBasePack.c_str(), o->mGuessPackage.c_str(),
			o->mOutName.c_str(), o->PackInformation);
	}

	if (o->MainWindow)
	{
		SetWindowTextW(o->MainWindow, o->mWinText.c_str());
	}

	o->Reset();
	return result;
}

HRESULT WINAPI PackInfo::DoPack(
	const WCHAR* lpBasePack,
	const WCHAR* GuessPackage,
	const WCHAR* OutName
)
{
	mBasePack = lpBasePack;
	mGuessPackage = GuessPackage;
	mOutName = OutName;

	ULONG Attr = GetFileAttributesW(lpBasePack);
	if ((Attr & FILE_ATTRIBUTE_DIRECTORY) && (Attr != INVALID_FILE_ATTRIBUTES))
	{
	}
	else
	{
		MessageBoxW(MainWindow, L"Cannot open target path", L"KrkrExtract", MB_OK);
		GlobalData::GetGlobalData()->isRunning = FALSE;
		return S_FALSE;
	}

	DeleteFileW(L"KrkrzTempWorker.xp3");

	if (this->MainWindow)
	{
		SetWindowTextW(this->MainWindow, L"Start Packing...");
	}
	wstring Info;
	Info = L"Packer Debug :\n";
	Info += L"BasePack : ";
	Info += lpBasePack;
	Info += L"\nGuessPackage : ";
	Info += GuessPackage;
	Info += L"\nOutName : ";
	Info += OutName;
	Info += L"\n";
	OutputString(Info.c_str());

	HANDLE hThread = CreateThread(NULL, NULL, PackerThread, this, NULL, &ThreadId);
	//PackerThread(this);
	return S_OK;
}

VOID WINAPI PackInfo::SetProcess(ULONG iPos)
{
	HWND hProcess = GetDlgItem(MainWindow, IDC_PROGRESS1);
	SendMessageW(hProcess, PBM_SETPOS, iPos, 0);
}

HRESULT WINAPI PackInfo::DoNormalPack(
	const WCHAR* lpBasePack,
	const WCHAR* lpGuessPack,
	const WCHAR* OutName)
{

	FileList.clear();
	this->IterFiles(wstring(lpBasePack));

	BOOL                    Result;
	HANDLE                  hHeap, hFile, hFileXP3;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize, BytesTransfered;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset;
	SMyXP3IndexNormal       *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, (ULONG64)0);
	wstring                 WStrBasePath(lpBasePack);
	//wstring                 TempPath(L"TempWorker\\");


	OutputString(L"Starting Packing...\n");

	const WCHAR* pszOutput = OutName;
	hFileXP3 = CreateFileW(pszOutput,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFileXP3 == INVALID_HANDLE_VALUE)
	{
		MessageBoxW(NULL, L"Couldn't open a handle for temporary output file.", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}

	hHeap = GetProcessHeap();
	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(hHeap, 0, BufferSize);
	lpCompressBuffer = HeapAlloc(hHeap, 0, CompressedSize);
	pXP3Index = (SMyXP3IndexNormal *)HeapAlloc(hHeap, 0, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	Offset.QuadPart = BytesTransfered;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		if (this->MainWindow)
		{
			WCHAR OutInfo[MAX_PATH] = { 0 };
			wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
			SetWindowTextW(MainWindow, OutInfo);
			SetProcess(((float)(i + 1) / (float)FileList.size()) * 100);
		}

		ZeroMemory(pIndex, sizeof(*pIndex));
		CopyMemory(pIndex->file.Magic, CHUNK_MAGIC_FILE, 4);
		CopyMemory(pIndex->info.Magic, CHUNK_MAGIC_INFO, 4);
		CopyMemory(pIndex->time.Magic, CHUNK_MAGIC_TIME, 4);
		CopyMemory(pIndex->segm.Magic, CHUNK_MAGIC_SEGM, 4);
		CopyMemory(pIndex->adlr.Magic, CHUNK_MAGIC_ADLR, 4);
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		//
		pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring FullName = wstring(lpBasePack) + L"\\";
		FullName += FileList[i].c_str();
		hFile = CreateFileW(FullName.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			wprintf(L"Couldn't open %s\n", FullName.c_str());
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
			wprintf(L"Couldn't open %s\n", FullName.c_str());
			getchar();
			continue;
		}

		pIndex->segm.segm->Offset = Offset;

		pIndex->info.FileName = FileList[i].c_str();
		pIndex->info.FileNameLength = FileList[i].length();

		pIndex->adlr.Hash = adler32(1, (Bytef *)lpBuffer, BytesTransfered);

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered;
		pIndex->info.OriginalSize.LowPart = BytesTransfered;

		FILETIME Time1, Time2;
		GetFileTime(hFile, &(pIndex->time.FileTime), &Time1, &Time2);

		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;
		DecryptWorker(EncryptOffset.LowPart, (PBYTE)lpBuffer, BytesTransfered, pIndex->adlr.Hash);
		if (XP3EncryptionFlag && pfProc)
		{
			pIndex->info.EncryptedFlag = 0x80000000;
		}
		else
		{
			pIndex->info.EncryptedFlag = 0x0;
		}

		if (InfoNameZeroEnd)
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		}
		else
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 2;
		}

		pIndex->segm.segm->bZlib = 0;
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

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		if (InfoNameZeroEnd)
		{
			Size.LowPart +=
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		}
		else
		{
			Size.LowPart +=
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 2;
		}
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

	WriteFile(hFileXP3, &IndexHeader, sizeof(IndexHeader), &BytesTransfered, NULL);
	WriteFile(hFileXP3, lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered, NULL);
	Offset.QuadPart = 0;
	SetFilePointerEx(hFileXP3, Offset, NULL, FILE_BEGIN);
	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	CloseHandle(hFileXP3);

	HeapFree(hHeap, 0, lpBuffer);
	HeapFree(hHeap, 0, lpCompressBuffer);
	HeapFree(hHeap, 0, pXP3Index);

	MessageBoxW(NULL, L"Making Package : Successful", L"KrkrExtract", MB_OK);
	return S_OK;
}


HRESULT WINAPI PackInfo::IterFiles(wstring srcPath)
{
	HRESULT dwStatus = S_OK;

	WIN32_FIND_DATAW findFileData = { 0 };
	wstring filePath(srcPath);
	filePath += L"\\*.*";

	HANDLE hFind = FindFirstFileW(filePath.c_str(), &findFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
			{
				continue;
			}
			else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				continue;
			}
			else
			{
				wstring WStrInfo(L"Found file : ");
				WStrInfo += findFileData.cFileName;
				WStrInfo += L"\n";
				OutputString(WStrInfo.c_str());
				FileList.push_back(findFileData.cFileName);
			}
		} while (FindNextFileW(hFind, &findFileData));
	}

	return dwStatus;
}

wstring GetPackageNamePacker(wstring& fileName)
{
	wstring temp(fileName);
	wstring::size_type pos = temp.find_last_of(L"\\");

	if (pos != wstring::npos)
	{
		temp = temp.substr(pos + 1, temp.length());
	}

	wstring temp2(temp);
	wstring::size_type pos2 = temp2.find_last_of(L"\\");
	if (pos2 != wstring::npos)
	{
		temp2 = temp2.substr(pos + 1, temp2.length());
	}
	return temp2;
}

void FormatPathNormal(wstring& package, ttstr& out)
{
	out.Clear();
	out = L"file://./";
	for (unsigned int iPos = 0; iPos < package.length(); iPos++)
	{
		if (package[iPos] == L':')
		{
			continue;
		}
		else if (package[iPos] == L'\\')
		{
			out += L'/';
		}
		else
		{
			out += package[iPos];
		}
	}
	out += L'>';
}


void FormatPathM2(wstring& package, ttstr& out)
{
	out.Clear();
	out = L"archive://./";
	out += GetPackageNamePacker(package).c_str();
	out += L"/";
}

//outdated
HRESULT WINAPI PackInfo::GetM2SingleEncryptionCode(const WCHAR* lpFileName)
{
	ttstr out;
	ttstr ArcOut;
	FormatPathNormal(wstring(lpFileName), out);

	FormatPathM2(wstring(lpFileName), ArcOut);

	WCHAR InfoW[260] = { 0 };
	wsprintfW(InfoW, L"M2ChunkList Size : %d\n", M2ChunkList.size());
	OutputString(InfoW);

	BOOL Found = FALSE;
	for (auto it : M2ChunkList)
	{
		if (it.isM2Format)
		{
			ttstr outFile = ArcOut + it.yuzu.Name.c_str();
			OutputString(L"[M2]");
			OutputString(outFile.c_str());
			OutputString(L"\n");
			IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);
			if (st == NULL)
			{
				OutputString(L"Failed\n");
				continue;
			}

			STATSTG t;
			st->Stat(&t, STATFLAG_DEFAULT);
			ULONG size = 0;
			UCHAR RtcData = 0;
			UCHAR EbbData = 0;

			st->Read(&RtcData, 1, &size);

			ttstr outFile2 = out + it.info.FileName.c_str();
			IStream * st2 = TVPCreateIStream(outFile2, TJS_BS_READ);
			OutputString(L"[Normal]");
			OutputString(outFile2.c_str());
			OutputString(L"\n");

			if (st2 == NULL)
			{
				OutputString(L"Failed\n");
				continue;
			}

			STATSTG t2;
			st2->Stat(&t2, STATFLAG_DEFAULT);
			st2->Read(&EbbData, 1, &size);

			this->DecryptionKey = (EbbData ^ RtcData);
			this->M2Hash = it.adlr.Hash;

			Found = TRUE;
			break;
		}
	}
	if (!Found)
	{
		MessageBoxW(NULL, L"Couldn't Get Enough Infomation.\nPlease try another package.", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}
	return S_OK;
}


HRESULT WINAPI PackInfo::DoM2DummyPackFirst(const WCHAR* lpBasePack)
{

	BOOL                    Result;
	HANDLE                  hHeap, hFile, hFileXP3;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize, BytesTransfered;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset;
	SMyXP3IndexM2            *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, (ULONG64)0);
	wstring                 WStrBasePath(lpBasePack);

	hFileXP3 = CreateFileW(L"KrkrzTempWorker.xp3",
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFileXP3 == INVALID_HANDLE_VALUE)
	{
		MessageBoxW(NULL, L"Couldn't create a handle for temporary output file.", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}

	hHeap = GetProcessHeap();
	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(hHeap, 0, BufferSize);
	lpCompressBuffer = HeapAlloc(hHeap, 0, CompressedSize);
	pXP3Index = (SMyXP3IndexM2 *)HeapAlloc(hHeap, 0, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	Offset.QuadPart = BytesTransfered;

	if (FileList.size() == 0)
	{
		MessageBoxW(NULL, L"No File to be packed", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}

	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		if (this->MainWindow)
		{
			WCHAR OutInfo[MAX_PATH] = { 0 };
			wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
			SetWindowTextW(this->MainWindow, OutInfo);
		}

		ZeroMemory(pIndex, sizeof(*pIndex));
		CopyMemory(pIndex->file.Magic, CHUNK_MAGIC_FILE, 4);
		CopyMemory(pIndex->info.Magic, CHUNK_MAGIC_INFO, 4);
		CopyMemory(pIndex->time.Magic, CHUNK_MAGIC_TIME, 4);
		CopyMemory(pIndex->yuzu.Magic, M2ChunkMagic, 4);
		CopyMemory(pIndex->segm.Magic, CHUNK_MAGIC_SEGM, 4);
		CopyMemory(pIndex->adlr.Magic, CHUNK_MAGIC_ADLR, 4);
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		//pIndex->adlr.ChunkSize.QuadPart = sizeof(pIndex->adlr) - sizeof(pIndex->adlr.Magic) - sizeof(pIndex->adlr.ChunkSize);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		if (M2NameZeroEnd)
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		}
		else
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0 - 2;
		}
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring FullName = wstring(lpBasePack) + L"\\";
		FullName += FileList[i].c_str();
		hFile = CreateFileW(FullName.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			wstring InfoW(L"Dummy write :\n[Failed to open]\n");
			InfoW += FullName;
			MessageBoxW(MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			return S_FALSE;
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
			wstring InfoW(L"Dummy write :\n[Failed to read]\n");
			InfoW += FullName;
			MessageBoxW(MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			return S_FALSE;
		}

		pIndex->segm.segm->Offset = Offset;

		wstring DummyName, DummyLowerName, HashName;
		
		DummyName = FileList[i] + L".dummy";
		DummyLowerName = ToLowerString(DummyName.c_str());

		GenMD5Code(DummyLowerName.c_str(), HashName);
		pIndex->info.FileName = HashName;
		pIndex->info.FileNameLength = HashName.length();

		pIndex->yuzu.Len = DummyName.length();
		pIndex->yuzu.Name = DummyName;

		if (M2NameZeroEnd)
		{
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD)+sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2;
		}
		else
		{
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD)+sizeof(USHORT)+(pIndex->yuzu.Name.length()) * 2;
		}

		//adler32(1/*adler32(0, 0, 0)*/, (Bytef *)lpBuffer, BytesTransfered);
		//pIndex->adlr.Hash = adler32(1/*adler32(0, 0, 0)*/, (Bytef *)lpBuffer, BytesTransfered);;
		pIndex->adlr.Hash = M2Hash;
		pIndex->yuzu.Hash = pIndex->adlr.Hash;

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered;
		pIndex->info.OriginalSize.LowPart = BytesTransfered;

		FILETIME Time1, Time2;
		GetFileTime(hFile, &(pIndex->time.FileTime), &Time1, &Time2);

		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;

		//Do nothing at present
		//DecryptWorkerM2(EncryptOffset.LowPart, (PBYTE)lpBuffer, BytesTransfered, pIndex->adlr.Hash, DecryptionKey);

		if (XP3EncryptionFlag)
		{
			pIndex->info.EncryptedFlag = 0x80000000;
		}
		else
		{
			pIndex->info.EncryptedFlag = 0x0;
		}

		PackChunkList.push_back(*pIndex);

		if (M2NameZeroEnd && InfoNameZeroEnd)
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		}
		else if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 2;
		}
		else
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 4;
		}
		pIndex->segm.segm->bZlib = 0;
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

	//EntryCount = pIndex - pXP3Index;
	XP3Header.IndexOffset = Offset;

	// generate index, calculate index size first
	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		if (M2NameZeroEnd && InfoNameZeroEnd)
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		}
		else if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 2;
		}
		else
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 4;
		}
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

	WriteFile(hFileXP3, &IndexHeader, sizeof(IndexHeader), &BytesTransfered, NULL);
	WriteFile(hFileXP3, lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered, NULL);
	Offset.QuadPart = 0;
	SetFilePointerEx(hFileXP3, Offset, NULL, FILE_BEGIN);
	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	CloseHandle(hFileXP3);

	HeapFree(hHeap, 0, lpBuffer);
	HeapFree(hHeap, 0, lpCompressBuffer);
	HeapFree(hHeap, 0, pXP3Index);
	return S_OK;
}



HRESULT WINAPI PackInfo::DoM2DummyPackFirst_Version2(const WCHAR* lpBasePack)
{

	BOOL                    Result;
	HANDLE                  hHeap, hFile, hFileXP3;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize, BytesTransfered;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset;
	SMyXP3IndexM2            *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, (ULONG64)0);
	wstring                 WStrBasePath(lpBasePack);

	hFileXP3 = CreateFileW(L"KrkrzTempWorker.xp3",
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFileXP3 == INVALID_HANDLE_VALUE)
	{
		MessageBoxW(NULL, L"Couldn't create a handle for temporary output file.", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}

	hHeap = GetProcessHeap();
	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(hHeap, 0, BufferSize);
	lpCompressBuffer = HeapAlloc(hHeap, 0, CompressedSize);
	pXP3Index = (SMyXP3IndexM2 *)HeapAlloc(hHeap, 0, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	Offset.QuadPart = BytesTransfered;

	if (FileList.size() == 0)
	{
		MessageBoxW(NULL, L"No File to be packed", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}

	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		if (this->MainWindow)
		{
			WCHAR OutInfo[MAX_PATH] = { 0 };
			wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
			SetWindowTextW(this->MainWindow, OutInfo);
		}

		ZeroMemory(pIndex, sizeof(*pIndex));
		CopyMemory(pIndex->file.Magic, CHUNK_MAGIC_FILE, 4);
		CopyMemory(pIndex->info.Magic, CHUNK_MAGIC_INFO, 4);
		CopyMemory(pIndex->time.Magic, CHUNK_MAGIC_TIME, 4);
		CopyMemory(pIndex->yuzu.Magic, M2ChunkMagic, 4);
		CopyMemory(pIndex->segm.Magic, CHUNK_MAGIC_SEGM, 4);
		CopyMemory(pIndex->adlr.Magic, CHUNK_MAGIC_ADLR, 4);
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		//pIndex->adlr.ChunkSize.QuadPart = sizeof(pIndex->adlr) - sizeof(pIndex->adlr.Magic) - sizeof(pIndex->adlr.ChunkSize);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		if (M2NameZeroEnd)
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		}
		else
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0 - 2;
		}
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring FullName = wstring(lpBasePack) + L"\\";
		FullName += FileList[i].c_str();
		hFile = CreateFileW(FullName.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			wstring InfoW(L"Dummy write :\n[Failed to open]\n");
			InfoW += FullName;
			MessageBoxW(MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			return S_FALSE;
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
			wstring InfoW(L"Dummy write :\n[Failed to read]\n");
			InfoW += FullName;
			MessageBoxW(MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			return S_FALSE;
		}

		pIndex->segm.segm->Offset = Offset;

		wstring DummyName, DummyLowerName, HashName;

		DummyName = FileList[i] + L".dummy";
		DummyLowerName = ToLowerString(DummyName.c_str());

		GenMD5Code(DummyLowerName.c_str(), HashName);
		pIndex->info.FileName = HashName;
		pIndex->info.FileNameLength = HashName.length();

		pIndex->yuzu.Len = DummyName.length();
		pIndex->yuzu.Name = DummyName;

		if (M2NameZeroEnd)
		{
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2;
		}
		else
		{
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (pIndex->yuzu.Name.length()) * 2;
		}

		//adler32(1/*adler32(0, 0, 0)*/, (Bytef *)lpBuffer, BytesTransfered);
		//pIndex->adlr.Hash = adler32(1/*adler32(0, 0, 0)*/, (Bytef *)lpBuffer, BytesTransfered);;
		pIndex->adlr.Hash = M2Hash;
		pIndex->yuzu.Hash = pIndex->adlr.Hash;

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered;
		pIndex->info.OriginalSize.LowPart = BytesTransfered;

		FILETIME Time1, Time2;
		GetFileTime(hFile, &(pIndex->time.FileTime), &Time1, &Time2);

		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;

		//Do nothing at present
		//DecryptWorkerM2(EncryptOffset.LowPart, (PBYTE)lpBuffer, BytesTransfered, pIndex->adlr.Hash, DecryptionKey);

		if (XP3EncryptionFlag)
		{
			pIndex->info.EncryptedFlag = 0x80000000;
		}
		else
		{
			pIndex->info.EncryptedFlag = 0x0;
		}

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

	//EntryCount = pIndex - pXP3Index;
	XP3Header.IndexOffset = Offset;

	// generate index, calculate index size first
	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		if (M2NameZeroEnd && InfoNameZeroEnd)
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		}
		else if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 2;
		}
		else
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 4;
		}
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

	WriteFile(hFileXP3, &IndexHeader, sizeof(IndexHeader), &BytesTransfered, NULL);
	WriteFile(hFileXP3, lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered, NULL);
	Offset.QuadPart = 0;
	SetFilePointerEx(hFileXP3, Offset, NULL, FILE_BEGIN);
	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	CloseHandle(hFileXP3);

	HeapFree(hHeap, 0, lpBuffer);
	HeapFree(hHeap, 0, lpCompressBuffer);
	HeapFree(hHeap, 0, pXP3Index);
	return S_OK;
}

/************************************************/

BOOL WINAPI PackInfo::IsCompatXP3(PBYTE Data, ULONG Size, PBYTE pMagic)
{
	if (pMagic)
		RtlZeroMemory(pMagic, 4);

	if (Size < 4)
		return TRUE;

	BYTE ChunkMagic[4], NextChunkMagic[4];
	if (RtlCompareMemory(Data, "File", 4) != 4)
	{
		//Search sp chunk to detect version
		ULONG iPos = 4;
		RtlCopyMemory(ChunkMagic, Data, 4);
		ULARGE_INTEGER ChunkSize;
		RtlCopyMemory(&ChunkSize, Data + iPos, 8);
		iPos += 8;
		RtlCopyMemory(NextChunkMagic, Data, 4);

		if (RtlCompareMemory(NextChunkMagic, ChunkMagic, 4) != 4)
		{
			return TRUE;
		}
		else
		{
			RtlCopyMemory(M2ChunkMagic, ChunkMagic, 4);
			if (pMagic)
				RtlCopyMemory(ChunkMagic, pMagic, 4);
			return FALSE;
		}
	}
	else
	{
		return TRUE;
	}
}


HRESULT WINAPI PackInfo::DoM2PackV2(
	const WCHAR* lpBasePack,
	const WCHAR* GuessPackage,
	const WCHAR* OutName,
	ULONG PackType)
{
	BOOL                    Result;
	HANDLE                  hHeap, hFileXP3;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize, BytesTransfered;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset;
	SMyXP3IndexM2            *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, (ULONG64)0);
	wstring                 WStrBasePath(lpBasePack);
	//wstring                 TempPath(L"TempWorker\\");

	OutputString(L"Starting Packing...\n");

	FileList.clear();
	this->IterFiles(wstring(lpBasePack));

	if (DoM2DummyPackFirst(lpBasePack) != S_OK)
	{
		return S_FALSE;
	}

	TVPExecuteScript(ttstr(L"Storages.addAutoPath(System.exePath + \"KrkrzTempWorker.xp3\" + \">\");"));

	const WCHAR* pszOutput = OutName;
	hFileXP3 = CreateFileW(pszOutput,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFileXP3 == INVALID_HANDLE_VALUE)
	{
		MessageBoxW(NULL, L"Couldn't create a handle for output xp3 file.", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}

	hHeap = GetProcessHeap();
	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(hHeap, 0, BufferSize);
	lpCompressBuffer = HeapAlloc(hHeap, 0, CompressedSize);
	pXP3Index = (SMyXP3IndexM2 *)HeapAlloc(hHeap, 0, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	Offset.QuadPart = BytesTransfered;
	for (ULONG i = 0; i <FileList.size(); ++pIndex, i++)
	{
		OutputString(L"At ");
		OutputString(PackChunkList[i].yuzu.Name.c_str());
		OutputString(L"\n");

		if (this->MainWindow)
		{
			WCHAR OutInfo[MAX_PATH] = { 0 };
			wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
			SetWindowTextW(this->MainWindow, OutInfo);
			SetProcess(((float)(i + 1) / (float)FileList.size()) * 100);
		}

		ZeroMemory(pIndex, sizeof(*pIndex));
		CopyMemory(pIndex->file.Magic, CHUNK_MAGIC_FILE, 4);
		CopyMemory(pIndex->info.Magic, CHUNK_MAGIC_INFO, 4);
		CopyMemory(pIndex->time.Magic, CHUNK_MAGIC_TIME, 4);
		CopyMemory(pIndex->yuzu.Magic, M2ChunkMagic, 4);
		CopyMemory(pIndex->segm.Magic, CHUNK_MAGIC_SEGM, 4);
		CopyMemory(pIndex->adlr.Magic, CHUNK_MAGIC_ADLR, 4);
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		//pIndex->adlr.ChunkSize.QuadPart = sizeof(pIndex->adlr) - sizeof(pIndex->adlr.Magic) - sizeof(pIndex->adlr.ChunkSize);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		if (M2NameZeroEnd)
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		}
		else
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0 - 2;
		}
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring DummyName = FileList[i] + L".dummy";
		//GenSha1CodeConst(FileList[i].c_str(), Sha1Name);

		ttstr FullName(L"archive://./KrkrzTempWorker.xp3/");
		//FullName += M2ChunkList[i].yuzu.Name.c_str();
		FullName += DummyName.c_str();
		

		OutputString(L"Dummy open : [");
		OutputString(FullName.c_str());
		OutputString(L"]\n");
		OutputString(FileList[i].c_str());
		OutputString(L"\n");

		IStream* st = TVPCreateIStream(FullName, TJS_BS_READ);
		if (st == NULL)
		{
			wstring InfoW(L"Couldn't open :\n");
			InfoW += FileList[i];
			MessageBoxW(MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			return S_FALSE;
		}

		STATSTG t;
		st->Stat(&t, STATFLAG_DEFAULT);
		Size.QuadPart = t.cbSize.QuadPart;
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(hHeap, 0, lpBuffer, BufferSize);
		}
		Result = st->Read(lpBuffer, Size.LowPart, &BytesTransfered);
		
		if (Result != S_OK || BytesTransfered != Size.LowPart)
		{
			//wprintf(L"Couldn't open %s\n", FullName.c_str());
			wstring InfoW(L"Couldn't read :\n");
			InfoW += FileList[i];
			MessageBoxW(MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			return S_FALSE;
			//continue;
		}

		pIndex->segm.segm->Offset = Offset;

		wstring LowerName = ToLowerString(FileList[i].c_str());
		wstring HashName;

		GenMD5Code(LowerName.c_str(), HashName);
		pIndex->info.FileName = HashName;
		pIndex->info.FileNameLength = HashName.length();

		pIndex->yuzu.Len = FileList[i].length();
		pIndex->yuzu.Name = FileList[i];

		if (M2NameZeroEnd)
		{
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD)+sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2;
		}
		else
		{
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD)+sizeof(USHORT)+(pIndex->yuzu.Name.length()) * 2;
		}

		pIndex->adlr.Hash = M2Hash;
		pIndex->yuzu.Hash = pIndex->adlr.Hash;

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered;
		pIndex->info.OriginalSize.LowPart = BytesTransfered;

		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;

		if (XP3EncryptionFlag)
		{
			pIndex->info.EncryptedFlag = 0x80000000;
		}
		else
		{
			pIndex->info.EncryptedFlag = 0x0;
		}

		if (M2NameZeroEnd && InfoNameZeroEnd)
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		}
		else if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 2;
		}
		else
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 4;
		}
		pIndex->segm.segm->bZlib = 0;
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

	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		if (M2NameZeroEnd && InfoNameZeroEnd)
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		}
		else if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 2;
		}
		else
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 4;
		}
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

	WriteFile(hFileXP3, &IndexHeader, sizeof(IndexHeader), &BytesTransfered, NULL);
	WriteFile(hFileXP3, lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered, NULL);
	Offset.QuadPart = 0;
	SetFilePointerEx(hFileXP3, Offset, NULL, FILE_BEGIN);
	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	CloseHandle(hFileXP3);

	HeapFree(hHeap, 0, lpBuffer);
	HeapFree(hHeap, 0, lpCompressBuffer);
	HeapFree(hHeap, 0, pXP3Index);

	MessageBoxW(NULL, L"Making Package : Successful!\nBut you must relaunch this game\nand delete \"KrkrzTempWorker.xp3\" to make the next package!!!", L"KrkrExtract (Important Infomation!!)", MB_OK);
	return S_OK;

}

//Since nekopara vol2
HRESULT WINAPI PackInfo::DoM2PackV2_Version2(
	const WCHAR* lpBasePack,
	const WCHAR* GuessPackage,
	const WCHAR* OutName,
	ULONG PackType)
{
	BOOL                    Result;
	HANDLE                  hHeap, hFileXP3;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize, BytesTransfered;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset;
	SMyXP3IndexM2            *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, (ULONG64)0);
	wstring                 WStrBasePath(lpBasePack);

	OutputString(L"Starting Packing...\n");

	FileList.clear();
	this->IterFiles(wstring(lpBasePack));

	if (DoM2DummyPackFirst_Version2(lpBasePack) != S_OK)
	{
		return S_FALSE;
	}

	TVPExecuteScript(ttstr(L"Storages.addAutoPath(System.exePath + \"KrkrzTempWorker.xp3\" + \">\");"));

	const WCHAR* pszOutput = OutName;
	hFileXP3 = CreateFileW(pszOutput,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFileXP3 == INVALID_HANDLE_VALUE)
	{
		MessageBoxW(NULL, L"Couldn't create a handle for output xp3 file.", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}

	hHeap = GetProcessHeap();
	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(hHeap, 0, BufferSize);
	lpCompressBuffer = HeapAlloc(hHeap, 0, CompressedSize);
	pXP3Index = (SMyXP3IndexM2 *)HeapAlloc(hHeap, 0, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	Offset.QuadPart = BytesTransfered;
	for (ULONG i = 0; i <FileList.size(); ++pIndex, i++)
	{
		OutputString(L"At ");
		OutputString(PackChunkList[i].yuzu.Name.c_str());
		OutputString(L"\n");

		if (this->MainWindow)
		{
			WCHAR OutInfo[MAX_PATH] = { 0 };
			wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
			SetWindowTextW(this->MainWindow, OutInfo);
			SetProcess(((float)(i + 1) / (float)FileList.size()) * 100);
		}

		ZeroMemory(pIndex, sizeof(*pIndex));
		CopyMemory(pIndex->file.Magic, CHUNK_MAGIC_FILE, 4);
		CopyMemory(pIndex->info.Magic, CHUNK_MAGIC_INFO, 4);
		CopyMemory(pIndex->time.Magic, CHUNK_MAGIC_TIME, 4);
		CopyMemory(pIndex->yuzu.Magic, M2ChunkMagic, 4);
		CopyMemory(pIndex->segm.Magic, CHUNK_MAGIC_SEGM, 4);
		CopyMemory(pIndex->adlr.Magic, CHUNK_MAGIC_ADLR, 4);
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		//pIndex->adlr.ChunkSize.QuadPart = sizeof(pIndex->adlr) - sizeof(pIndex->adlr.Magic) - sizeof(pIndex->adlr.ChunkSize);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		if (M2NameZeroEnd)
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		}
		else
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0 - 2;
		}
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring DummyName = FileList[i] + L".dummy";
		//GenSha1CodeConst(FileList[i].c_str(), Sha1Name);

		ttstr FullName(L"archive://./KrkrzTempWorker.xp3/");
		//FullName += M2ChunkList[i].yuzu.Name.c_str();
		FullName += DummyName.c_str();


		OutputString(L"Dummy open : [");
		OutputString(FullName.c_str());
		OutputString(L"]\n");
		OutputString(FileList[i].c_str());
		OutputString(L"\n");

		IStream* st = TVPCreateIStream(FullName, TJS_BS_READ);
		if (st == NULL)
		{
			wstring InfoW(L"Couldn't open :\n");
			InfoW += FileList[i];
			MessageBoxW(MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			return S_FALSE;
		}

		STATSTG t;
		st->Stat(&t, STATFLAG_DEFAULT);
		Size.QuadPart = t.cbSize.QuadPart;
		if (Size.LowPart > BufferSize)
		{
			BufferSize = Size.LowPart;
			lpBuffer = HeapReAlloc(hHeap, 0, lpBuffer, BufferSize);
		}
		Result = st->Read(lpBuffer, Size.LowPart, &BytesTransfered);

		if (Result != S_OK || BytesTransfered != Size.LowPart)
		{
			//wprintf(L"Couldn't open %s\n", FullName.c_str());
			wstring InfoW(L"Couldn't read :\n");
			InfoW += FileList[i];
			MessageBoxW(MainWindow, InfoW.c_str(), L"KrkrExtract", MB_OK);
			return S_FALSE;
			//continue;
		}

		pIndex->segm.segm->Offset = Offset;

		wstring LowerName = ToLowerString(FileList[i].c_str());
		wstring HashName;

		GenMD5Code(LowerName.c_str(), HashName);
		pIndex->info.FileName = HashName;
		pIndex->info.FileNameLength = HashName.length();

		pIndex->yuzu.Len = FileList[i].length();
		pIndex->yuzu.Name = FileList[i];

		if (M2NameZeroEnd)
		{
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2;
		}
		else
		{
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD) + sizeof(USHORT) + (pIndex->yuzu.Name.length()) * 2;
		}

		pIndex->adlr.Hash = M2Hash;
		pIndex->yuzu.Hash = pIndex->adlr.Hash;

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered;
		pIndex->info.OriginalSize.LowPart = BytesTransfered;

		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;

		if (XP3EncryptionFlag)
		{
			pIndex->info.EncryptedFlag = 0x80000000;
		}
		else
		{
			pIndex->info.EncryptedFlag = 0x0;
		}

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

	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		if (M2NameZeroEnd && InfoNameZeroEnd)
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		}
		else if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 2;
		}
		else
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT) + (pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT) + sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL) +
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 4;
		}
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

	WriteFile(hFileXP3, &IndexHeader, sizeof(IndexHeader), &BytesTransfered, NULL);
	WriteFile(hFileXP3, lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered, NULL);
	Offset.QuadPart = 0;
	SetFilePointerEx(hFileXP3, Offset, NULL, FILE_BEGIN);
	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	CloseHandle(hFileXP3);

	HeapFree(hHeap, 0, lpBuffer);
	HeapFree(hHeap, 0, lpCompressBuffer);
	HeapFree(hHeap, 0, pXP3Index);

	if (!DeleteFileW(L"KrkrzTempWorker.xp3"))
	{
		MessageBoxW(NULL, L"Making Package : Successful!\nBut you must relaunch this game\nand delete \"KrkrzTempWorker.xp3\" to make the next package!!!", L"KrkrExtract (Important Infomation!!)", MB_OK);
	}
	return S_OK;
}


HRESULT WINAPI PackInfo::DoM2Pack(
	const WCHAR* lpBasePack,
	const WCHAR* GuessPackage,
	const WCHAR* OutName,
	ULONG PackType)
{
	FileList.clear();
	this->IterFiles(wstring(lpBasePack));

	BOOL                    Result;
	HANDLE                  hHeap, hFile, hFileXP3;
	PBYTE                   pbIndex;
	ULONG                   BufferSize, CompressedSize, BytesTransfered;
	PVOID                   lpBuffer, lpCompressBuffer;
	LARGE_INTEGER           Size, Offset;
	SMyXP3IndexM2            *pXP3Index, *pIndex;
	KRKR2_XP3_DATA_HEADER   IndexHeader;
	BYTE                    FirstMagic[11] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER        XP3Header(FirstMagic, (ULONG64)0);
	wstring                 WStrBasePath(lpBasePack);
	//wstring                 TempPath(L"TempWorker\\");

	OutputString(L"Starting Packing...\n");

	if (this->GetM2SingleEncryptionCode(GuessPackage) != S_OK)
	{
		return S_FALSE;
	}

	const WCHAR* pszOutput = OutName;
	hFileXP3 = CreateFileW(pszOutput,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFileXP3 == INVALID_HANDLE_VALUE)
	{
		MessageBoxW(NULL, L"Couldn't create a handle for output xp3 file.", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}

	hHeap = GetProcessHeap();
	BufferSize = 0x10000;
	CompressedSize = BufferSize;
	lpBuffer = HeapAlloc(hHeap, 0, BufferSize);
	lpCompressBuffer = HeapAlloc(hHeap, 0, CompressedSize);
	pXP3Index = (SMyXP3IndexM2 *)HeapAlloc(hHeap, 0, sizeof(*pXP3Index) * FileList.size());
	pIndex = pXP3Index;

	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	Offset.QuadPart = BytesTransfered;
	for (ULONG i = 0; i < FileList.size(); ++pIndex, i++)
	{
		if (this->MainWindow)
		{
			WCHAR OutInfo[MAX_PATH] = { 0 };
			wsprintfW(OutInfo, PackingFormatString, i + 1, FileList.size());
			SetWindowTextW(this->MainWindow, OutInfo);
			SetProcess(((float)(i + 1) / (float)FileList.size()) * 100);
		}

		ZeroMemory(pIndex, sizeof(*pIndex));
		CopyMemory(pIndex->file.Magic, CHUNK_MAGIC_FILE, 4);
		CopyMemory(pIndex->info.Magic, CHUNK_MAGIC_INFO, 4);
		CopyMemory(pIndex->time.Magic, CHUNK_MAGIC_TIME, 4);
		CopyMemory(pIndex->yuzu.Magic, M2ChunkMagic, 4);
		CopyMemory(pIndex->segm.Magic, CHUNK_MAGIC_SEGM, 4);
		CopyMemory(pIndex->adlr.Magic, CHUNK_MAGIC_ADLR, 4);
		pIndex->segm.ChunkSize.QuadPart = (ULONG64)0x1C; //sizeof(pIndex->segm.segm);
		//pIndex->adlr.ChunkSize.QuadPart = sizeof(pIndex->adlr) - sizeof(pIndex->adlr.Magic) - sizeof(pIndex->adlr.ChunkSize);
		pIndex->adlr.ChunkSize.QuadPart = (ULONG64)0x04;
		pIndex->info.ChunkSize.QuadPart = (ULONG64)0x58;

		if (M2NameZeroEnd)
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0;
		}
		else
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)0xB0 - 2;
		}
		pIndex->time.ChunkSize.QuadPart = (ULONG64)0x08;

		wstring FullName = wstring(lpBasePack) + L"\\";
		FullName += FileList[i].c_str();
		hFile = CreateFileW(FullName.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			wprintf(L"Couldn't open %s\n", FullName.c_str());
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
			wprintf(L"Couldn't open %s\n", FullName.c_str());
			continue;
		}

		pIndex->segm.segm->Offset = Offset;

		wstring LowerName = ToLowerString(FileList[i].c_str());
		wstring HashName;

		GenMD5Code(LowerName.c_str(), HashName);
		pIndex->info.FileName = HashName;
		pIndex->info.FileNameLength = HashName.length();

		pIndex->yuzu.Len = FileList[i].length();
		pIndex->yuzu.Name = FileList[i];

		if (M2NameZeroEnd)
		{
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD)+sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2;
		}
		else
		{
			pIndex->yuzu.ChunkSize.QuadPart = sizeof(DWORD)+sizeof(USHORT)+(pIndex->yuzu.Name.length()) * 2;
		}

		//adler32(1/*adler32(0, 0, 0)*/, (Bytef *)lpBuffer, BytesTransfered);
		pIndex->adlr.Hash = M2Hash;

		pIndex->yuzu.Hash = pIndex->adlr.Hash;

		pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered;
		pIndex->info.OriginalSize.LowPart = BytesTransfered;

		FILETIME Time1, Time2;
		GetFileTime(hFile, &(pIndex->time.FileTime), &Time1, &Time2);

		LARGE_INTEGER EncryptOffset;

		EncryptOffset.QuadPart = 0;
		DecryptWorkerM2(EncryptOffset.LowPart, (PBYTE)lpBuffer, BytesTransfered, pIndex->adlr.Hash, DecryptionKey);
		
		if (XP3EncryptionFlag)
		{
			pIndex->info.EncryptedFlag = 0x80000000;
		}
		else
		{
			pIndex->info.EncryptedFlag = 0x0;
		}

		if (M2NameZeroEnd && InfoNameZeroEnd)
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		}
		else if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 2;
		}
		else
		{
			pIndex->file.ChunkSize.QuadPart = (ULONG64)sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 4;
		}
		pIndex->segm.segm->bZlib = 0;
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

	//EntryCount = pIndex - pXP3Index;
	XP3Header.IndexOffset = Offset;

	// generate index, calculate index size first
	Size.LowPart = 0;
	pIndex = pXP3Index;

	for (ULONG i = 0; i < FileList.size(); ++i, ++pIndex)
	{
		if (M2NameZeroEnd && InfoNameZeroEnd)
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash);
		}
		else if ((M2NameZeroEnd && (!InfoNameZeroEnd)) || ((!M2NameZeroEnd) && InfoNameZeroEnd))
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 2;
		}
		else
		{
			Size.LowPart += sizeof(pIndex->yuzu.Hash) * 3 + sizeof(USHORT)+(pIndex->yuzu.Name.length() + 1) * 2 + MagicLength +
				sizeof(pIndex->time.ChunkSize) + MagicLength + sizeof(pIndex->time.FileTime) +
				sizeof(pIndex->file.ChunkSize) + MagicLength +
				sizeof(pIndex->info.ArchiveSize) * 2 + sizeof(pIndex->info.EncryptedFlag) + sizeof(USHORT)+sizeof(pIndex->info.ChunkSize) + MagicLength + (pIndex->info.FileName.length() + 1) * 2 +
				sizeof(pIndex->segm.ChunkSize) + MagicLength + sizeof(pIndex->segm.segm[0].ArchiveSize) * 3 + sizeof(BOOL)+
				sizeof(pIndex->adlr.ChunkSize) + MagicLength + sizeof(pIndex->adlr.Hash) - 4;
		}
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

	WriteFile(hFileXP3, &IndexHeader, sizeof(IndexHeader), &BytesTransfered, NULL);
	WriteFile(hFileXP3, lpBuffer, IndexHeader.ArchiveSize.LowPart, &BytesTransfered, NULL);
	Offset.QuadPart = 0;
	SetFilePointerEx(hFileXP3, Offset, NULL, FILE_BEGIN);
	WriteFile(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered, NULL);

	CloseHandle(hFileXP3);

	HeapFree(hHeap, 0, lpBuffer);
	HeapFree(hHeap, 0, lpCompressBuffer);
	HeapFree(hHeap, 0, pXP3Index);

	MessageBoxW(NULL, L"Making Package : Successful", L"KrkrExtract", MB_OK);
	return S_OK;
}

HRESULT WINAPI PackInfo::IsM2SingleDecryption(
	BYTE* pOriginalBuffer,
	ULONG OriginalSize,
	BYTE* pDecodeBuffer,
	ULONG DecodeSize)
{
	if (DecodeSize != OriginalSize)
	{
		return S_FALSE;
	}
	if (RtlCompareMemory(pOriginalBuffer, pDecodeBuffer, DecodeSize) == OriginalSize)
	{
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}

