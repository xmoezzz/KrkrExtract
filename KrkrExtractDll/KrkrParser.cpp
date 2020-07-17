#include "PluginHook.h"

static const WCHAR* ProtectionInfo = L"$$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ Warning! Extracting this archive may infringe on author's rights. 警告 このアーカイブを展開することにより、あなたは著作者の権利を侵害するおそれがあります。.txt";

HRESULT WINAPI PluginHook::ProcessXP3Archive(std::wstring xp3, HANDLE st)
{
	GlobalData::GetGlobalData()->isRunning = TRUE;
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

	if (ReadFile(st, &XP3Header, sizeof(XP3Header), &nRet, NULL) != TRUE)
	{
		OutputString(L"Invaild Package\n");
		return S_FALSE;
	}

	//Exe Built-in Package Support
	if ((*(PUSHORT)XP3Header.Magic) == IMAGE_DOS_SIGNATURE)
	{
		Status = FindEmbededXp3OffsetSlow(this->wFileName, &BeginOffset);

		if (!KERNEL32_SUCCESS(Status))
		{
			OutputString(L"No a Built-in Package\n");
			return S_FALSE;
		}

		SetFilePointerEx(st, BeginOffset, NULL, FILE_BEGIN);
		ReadFile(st, &XP3Header, sizeof(XP3Header), &nRet, NULL);
	}
	else
	{
		BeginOffset.QuadPart = 0;
	}

	if (RtlCompareMemory(StaticXP3V2Magic, XP3Header.Magic, sizeof(StaticXP3V2Magic)) != sizeof(StaticXP3V2Magic))
	{
		OutputString(L"No a XP3 Package!\n");
		return S_FALSE;
	}

	ULONG64 CompresseBufferSize = 0x1000;
	ULONG64 DecompressBufferSize = 0x1000;
	PBYTE pCompress = (PBYTE)HeapAlloc(GetProcessHeap(), 0, (ULONG)CompresseBufferSize);
	PBYTE pDecompress = (PBYTE)HeapAlloc(GetProcessHeap(), 0, (ULONG)DecompressBufferSize);
	DataHeader.OriginalSize = XP3Header.IndexOffset;

	wsprintfW(OutputInfo, L"Index Offset %08x\n", (ULONG32)XP3Header.IndexOffset.QuadPart);
	OutputString(OutputInfo);
	BOOL Result = FALSE;
	do
	{
		LARGE_INTEGER Offset;

		Offset.QuadPart = DataHeader.OriginalSize.QuadPart + BeginOffset.QuadPart;
		SetFilePointerEx(st, Offset, NULL, FILE_BEGIN);
		if (!USER32_SUCCESS(ReadFile(st, &DataHeader, sizeof(DataHeader), &nRet, NULL)))
		{
			OutputString(L"Couldn't Read Index Header\n");
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
			SetFilePointerEx(st, Offset, NULL, FILE_CURRENT);
		}

		ReadFile(st, pCompress, DataHeader.ArchiveSize.LowPart, &nRet, NULL);

		BOOL EncodeMark = DataHeader.bZlib & 7;

		wsprintfW(OutputInfo, L"Index Encode %x\n", DataHeader.bZlib);
		OutputString(OutputInfo);

		if (EncodeMark == FALSE)
		{
			OutputString(L"Index : Raw Data\n");
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
			OutputString(L"Index : Zlib Data\n");
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
			OutputString(L"Index : Unknown Encode Method!\n");
			return S_FALSE;
		}

		wsprintfW(OutputInfo, L"Index Size %08x\n", (ULONG32)DataHeader.OriginalSize.LowPart);
		OutputString(OutputInfo);

		if (IsCompatXP3(pDecompress, DataHeader.OriginalSize.LowPart))
		{
			FindChunkMagicFirst(pDecompress, DataHeader.OriginalSize.LowPart);

			CHAR M2ChunkInfo[10] = { 0 };
			CopyMemory(M2ChunkInfo, M2ChunkMagic, 4);
			lstrcatA(M2ChunkInfo, "\n");
			OutputString(M2ChunkInfo);

			//再次检查Chunk，如果SP Chunk只是出现在文件头，并且只是出现一次，那么就是千恋万花的新格式。
			
			switch (DetectCompressedChunk(pDecompress, DataHeader.OriginalSize.LowPart))
			{
			case TRUE:
				Result = InitIndexFileFirst(pDecompress, DataHeader.OriginalSize.LowPart);
				break;

			case FALSE:
				Result = InitIndexFile_SenrenBanka(pDecompress, DataHeader.OriginalSize.LowPart, st);
				break;
			}
			
			if (Result)
			{
				OutputString(L"Normal XP3 or M2 XP3\n");
				break;
			}
		}
		else
		{
			Result = InitIndexNew1(pDecompress, DataHeader.OriginalSize.LowPart);
			if (Result)
			{
				OutputString(L"M2 XP3(since nekopara vol2)\n");
				break;
			}
		}
		
	} while (DataHeader.bZlib & 0x80);

	
	HeapFree(GetProcessHeap(), 0, pCompress);
	HeapFree(GetProcessHeap(), 0, pDecompress);

	return S_OK;
}


#define iPosAdd(x) iPos+= (ULONG)x
#define PtrInc(x) PtrOffset += (ULONG)x

ULONG WINAPI PluginHook::DetectCompressedChunk(PBYTE pDecompress, ULONG Size)
{
	ULONG PtrOffset = 0;
	BOOL  FirstTraveled, NextTraveled, IsFirstChunk;

	OutputString(L"Detacting Index Info...\n");

	FirstTraveled = FALSE;
	NextTraveled  = FALSE;
	IsFirstChunk  = FALSE;

	while (PtrOffset < Size)
	{
		BOOL     NotAdd = FALSE;
		XP3Index item;

		WCHAR PosInfo[260] = { 0 };
		wsprintfW(PosInfo, L"[Start]Pos : %08x\n", PtrOffset);
		OutputString(PosInfo);

		if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_YUZU, 4) == 4 ||
			RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_M2, 4) == 4 ||
			RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_NEKO, 4) == 4 ||
			RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_FENG, 4) == 4 ||
			RtlCompareMemory(pDecompress + PtrOffset, M2ChunkMagic, 4) == 4)
		{
			if (FirstTraveled)
				NextTraveled = TRUE;

			if (!FirstTraveled)
				IsFirstChunk = PtrOffset == 0;

			FirstTraveled = TRUE;
			PtrOffset += 4;
			ULARGE_INTEGER IndexLength;
			RtlCopyMemory(&IndexLength, pDecompress + PtrOffset, 8);

			//look forward the related file chunk
			ULONG CurOffset = PtrOffset + IndexLength.LowPart + 8;
			if (RtlCompareMemory(pDecompress + CurOffset, "File", 4) != 4)
			{
				OutputString(L"[info]No File Chunk Magic(at bad chunk detaction)\n");
				MessageBoxW(NULL, L"Internal Error", L"KrkrExtract", MB_OK);
				GlobalData::GetGlobalData()->isRunning = FALSE;
				return FALSE;
			}

			CurOffset += 4;
			RtlCopyMemory(&IndexLength, pDecompress + CurOffset, 8);
			CurOffset += 8;
			ULONG EndOffset = CurOffset + IndexLength.LowPart;

			while (CurOffset < EndOffset)
			{
				if (RtlCompareMemory(pDecompress + CurOffset, "info", 4) == 4)
				{
					CurOffset += 4;
					RtlCopyMemory(&IndexLength, pDecompress + CurOffset, 8);
					CurOffset += 8;
					CurOffset += 4;
					CurOffset += 8 * 2;
					CurOffset += 2;

					if (!wcsncmp((LPCWSTR)(pDecompress + CurOffset), ProtectionInfo, lstrlenW(ProtectionInfo)))
					{
						NotAdd = TRUE;
						PtrOffset = EndOffset;
						goto NextFileChunk;
					}
				}
				else
				{
					CurOffset += 4;
					RtlCopyMemory(&IndexLength, pDecompress + CurOffset, 8);
					CurOffset += 8;
					CurOffset += IndexLength.LowPart;
				}
			}

			PtrOffset += 8;
			item.isM2Format = TRUE;
			ULONG HashInfo = 0;
			RtlCopyMemory(&HashInfo, (pDecompress + PtrOffset), 4);
			item.yuzu.Hash = HashInfo;
			PtrOffset += 4;
			USHORT FileNameLen = 0;
			RtlCopyMemory(&FileNameLen, (pDecompress + PtrOffset), 2);
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

			this->isM2Format = TRUE;

			if (!wcscmp(item.yuzu.Name.c_str(), ProtectionInfo))
			{
				NotAdd = TRUE;
			}
		}

		if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_FILE, 4) != 4)
		{
			OutputString(L"[info]No File Chunk Magic\n");
			MessageBoxW(NULL, L"Internal Error", L"KrkrExtract", MB_OK);
			GlobalData::GetGlobalData()->isRunning = FALSE;
			return FALSE;
		}

		//File Maigc
		PtrInc(4);
		ULONG64 FileChunkSize64 = 0;
		CopyMemory(&FileChunkSize64, (PtrOffset + pDecompress), 8);
		PtrInc(8);

		wsprintfW(PosInfo, L"FileChunkSize : %08x\n", (ULONG32)FileChunkSize64);
		OutputString(PosInfo);

		ULONG SavePtrOffset = PtrOffset;
		ULONG32 FileChunkSize32 = (ULONG32)FileChunkSize64;
		ULONG iPos = 0;

		while (iPos < FileChunkSize32)
		{
			if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_INFO, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 InfoChunkSize = 0;
				RtlCopyMemory(&InfoChunkSize, (PtrOffset + pDecompress), sizeof(ULONG64));
				wsprintfW(PosInfo, L"Info Chunk : %08x\n", InfoChunkSize);
				OutputString(PosInfo);

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


				//某高考 的 支持
				WCHAR* FakeName = (WCHAR*)(PtrOffset + pDecompress);
				if ((item.info.FileNameLength > MAX_PATH && (ULONG)InfoChunkSize == 0x20 &&
					(FakeName[0] != L'$' && FakeName[1] != L'$' && FakeName[2] != L'$'))
					|| this->RawExtract)
					//并不稳定，最好是对齐比较，自动获取到下一个Sign的长度
				{
					this->RawExtract = TRUE;
					PtrOffset -= 2;
					iPos -= 2;
					item.info.FileNameLength = 0xFFFF;
					ULONG FakeNameSegm1, FakeNameSegm2, FakeNameSegm3;

					FakeNameSegm1 = *(PULONG)(PtrOffset + pDecompress);
					PtrInc(4);
					iPosAdd(4);
					FakeNameSegm2 = *(PULONG)(PtrOffset + pDecompress);
					PtrInc(4);
					iPosAdd(4);
					FakeNameSegm3 = *(PULONG)(PtrOffset + pDecompress);
					PtrInc(4);
					iPosAdd(4);

					WCHAR FakeNameStr[60] = { 0 };
					wsprintfW(FakeNameStr, L"%08X-%08X-%08X", FakeNameSegm1, FakeNameSegm2, FakeNameSegm3);
					item.info.FileName = FakeNameStr;
				}
				else
				{
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
					if (WideFileName[0] == L'$' && WideFileName[1] == L'$' && WideFileName[2] == L'$')
					{
						//FileChunkSize32
						PtrOffset = SavePtrOffset;
						PtrOffset += FileChunkSize32;
						iPos = FileChunkSize32;
						NotAdd = TRUE;
					}
					else if (item.info.FileNameLength > MAX_PATH)
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

					if (!wcscmp(ProtectionInfo, WideFileName.c_str()))
					{
						NotAdd = TRUE;
					}
				}
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_SEGM, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				item.segm.ChunkSize.QuadPart = TempChunkSize;
				wsprintfW(PosInfo, L"Segm Chunk : %08x\n", TempChunkSize);
				OutputString(PosInfo);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.segm.segm[0].bZlib), (PtrOffset + pDecompress), sizeof(BOOL));
				CopyMemory(&(item.segm.segm[0].Offset), (PtrOffset + pDecompress + sizeof(BOOL)), sizeof(ULONG64));
				CopyMemory(&(item.segm.segm[0].OriginalSize), (PtrOffset + pDecompress + sizeof(BOOL) + sizeof(ULONG64)), sizeof(ULONG64));
				CopyMemory(&(item.segm.segm[0].ArchiveSize), (PtrOffset + pDecompress + sizeof(BOOL) + sizeof(ULONG64) * 2), sizeof(ULONG64));
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_ADLR, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);

				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				wsprintfW(PosInfo, L"Adlr Chunk : %08x\n", TempChunkSize);
				OutputString(PosInfo);
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

				wsprintfW(PosInfo, L"Time Chunk : %08x\n", TempChunkSize);
				OutputString(PosInfo);
				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
			else
			{
				UCHAR UnknownMagic[4];
				RtlCopyMemory(UnknownMagic, (PtrOffset + pDecompress), 4);
				//Magic
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);

				RtlZeroMemory(PosInfo, sizeof(PosInfo));
				wsprintfW(PosInfo, L"Found Unknown Chunk at %08x\n", PtrOffset);
				OutputString(PosInfo);
				RtlZeroMemory(PosInfo, sizeof(PosInfo));
				wsprintfW(PosInfo, L"Unknown Chunk [%c%c%c%c] : %08x\n", UnknownMagic[0], UnknownMagic[1],
					UnknownMagic[2], UnknownMagic[3], TempChunkSize);
				OutputString(PosInfo);
				//ChunkSize
				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
		}

	NextFileChunk:
		__asm nop;
	}

	switch (M2ChunkMagic[0])
	{
	case 0:
		return TRUE;

	default:
		//return NextTraveled && IsFirstChunk;
		return NextTraveled;
	}

	// NextTraveled && M2ChunkMagic[0] && !IsFirstChunk;
}


BOOL WINAPI PluginHook::InitIndexFile_SenrenBanka(PBYTE pDecompress, ULONG Size, HANDLE hFile)
{
	KRKRZ_COMPRESSED_INDEX  CompressedIndex;
	PBYTE                   CompressedBuffer;
	PBYTE                   IndexBuffer;
	ULONG                   DecompSize, iPos;
	DWORD                   BytesTranferred, Hash;
	XP3Index                Item;
	ULARGE_INTEGER          ChunkSize;
	USHORT                  NameLength;

	RtlCopyMemory(&CompressedIndex, pDecompress, sizeof(KRKRZ_COMPRESSED_INDEX));
	SetFilePointer(hFile, CompressedIndex.Offset.LowPart, NULL, FILE_BEGIN);

	iPos             = 0;
	DecompSize       = CompressedIndex.DecompressedSize;
	IndexBuffer      = (PBYTE)HeapAlloc(GetProcessHeap(), 0, CompressedIndex.DecompressedSize);
	CompressedBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, CompressedIndex.CompressedSize);

	if (!IndexBuffer || !CompressedBuffer)
	{
		MessageBoxW(NULL, L"Insufficient memory", L"KrkrExtract", MB_OK);
		return FALSE;
	}

	ReadFile(hFile, CompressedBuffer, CompressedIndex.CompressedSize, &BytesTranferred, NULL);


	if ((DecompSize = uncompress((PBYTE)IndexBuffer, (PULONG)&DecompSize,
		(PBYTE)CompressedBuffer, CompressedIndex.CompressedSize)) != Z_OK)
	{
		MessageBoxW(NULL, L"Failed to decompress special chunk", L"KrkrExtract", MB_OK);
		return FALSE;
	}

	isM2Format = TRUE;
	
	while (iPos < CompressedIndex.DecompressedSize)
	{
		iPos += 4;
		RtlCopyMemory(&ChunkSize, IndexBuffer + iPos, 8);
		iPos += 8;
		RtlCopyMemory(&Hash, IndexBuffer + iPos, 4);
		iPos += 4;
		RtlCopyMemory(&NameLength, IndexBuffer + iPos, 2);
		iPos += 2;
		wstring FileName((LPCWSTR)(IndexBuffer + iPos), NameLength);
		iPos += (NameLength + 1) * 2;

		Item.yuzu.ChunkSize.QuadPart = ChunkSize.QuadPart;
		Item.yuzu.Hash = Hash;
		Item.yuzu.Len  = NameLength;
		Item.yuzu.Name = FileName;

		Item.isM2Format = true;
		
		ItemVector.push_back(Item);
	}

	CountFile = ItemVector.size();
	HeapFree(GetProcessHeap(), 0, IndexBuffer);
	HeapFree(GetProcessHeap(), 0, CompressedBuffer);

	return TRUE;
}

//Return TRUE -> 老版本的封包
BOOL WINAPI PluginHook::IsCompatXP3(PBYTE Data, ULONG Size, PBYTE pMagic)
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
		iPos += ChunkSize.LowPart;
		RtlCopyMemory(NextChunkMagic, Data + iPos, 4);

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


BOOL WINAPI PluginHook::InitIndexNew1(PBYTE pDecompress, ULONG Size)
{
	this->isM2Format = TRUE;
	ULONG iPos = 0;
	CountFile = 0;
	ULONG SavePos = 0;
	while (TRUE)
	{
		if (RtlCompareMemory((pDecompress + iPos), M2ChunkMagic, 4) == 4)
		{
			iPos += 4;
			ULONG64 ChunkSize = 0;
			RtlCopyMemory(&ChunkSize, (pDecompress + iPos), 8);
			iPos += 8;
			SavePos = iPos;
			XP3Index item;
			item.yuzu.ChunkSize.QuadPart = ChunkSize;
			ULONG HashValue = 0;
			RtlCopyMemory(&HashValue, (pDecompress + iPos), 4);
			iPos += 4;
			item.yuzu.Hash = HashValue;
			USHORT FileNameLen = 0;
			RtlCopyMemory(item.yuzu.Magic, M2ChunkMagic, 4);
			RtlCopyMemory(&FileNameLen, (pDecompress + iPos), 2);
			iPos += 2;
			wstring FileName((WCHAR*)(pDecompress + iPos), FileNameLen);
			item.yuzu.Name = FileName;
			iPos = SavePos;
			iPos += ChunkSize;
			item.isM2Format = TRUE;
			ItemVector.push_back(item);
			CountFile++;
		}
		else
		{
			break;
		}
	}
	return TRUE;
}


BOOL WINAPI PluginHook::InitIndexFileFirst(PBYTE pDecompress, ULONG Size)
{
	ULONG PtrOffset = 0;

	OutputString(L"Init Index Info...\n");

	while (PtrOffset < Size)
	{
		BOOL     NotAdd = FALSE;
		XP3Index item;

		WCHAR PosInfo[260] = { 0 };
		wsprintfW(PosInfo, L"[Start]Pos : %08x\n", PtrOffset);
		OutputString(PosInfo);

		if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_YUZU, 4) == 4       ||
			RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_M2, 4) == 4   ||
			RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_NEKO, 4) == 4 ||
			RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_FENG, 4) == 4 ||
			RtlCompareMemory(pDecompress + PtrOffset, M2ChunkMagic, 4) == 4)
		{
			PtrOffset += 4;
			ULARGE_INTEGER IndexLength;
			RtlCopyMemory(&IndexLength, pDecompress + PtrOffset, 8);

			//look forward the related file chunk
			ULONG CurOffset = PtrOffset + IndexLength.LowPart + 8;
			if (RtlCompareMemory(pDecompress + CurOffset, "File", 4)!= 4)
			{
				OutputString(L"[info]No File Chunk Magic(at bad chunk detaction)\n");
				MessageBoxW(NULL, L"Internal Error", L"KrkrExtract", MB_OK);
				GlobalData::GetGlobalData()->isRunning = FALSE;
				return FALSE;
			}

			CurOffset += 4;
			RtlCopyMemory(&IndexLength, pDecompress + CurOffset, 8);
			CurOffset += 8;
			ULONG EndOffset = CurOffset + IndexLength.LowPart;

			while (CurOffset < EndOffset)
			{
				if (RtlCompareMemory(pDecompress + CurOffset, "info", 4) == 4)
				{
					CurOffset += 4;
					RtlCopyMemory(&IndexLength, pDecompress + CurOffset, 8);
					CurOffset += 8;
					CurOffset += 4;
					CurOffset += 8 * 2;
					CurOffset += 2;

					if (!wcsncmp((LPCWSTR)(pDecompress + CurOffset), ProtectionInfo, lstrlenW(ProtectionInfo)))
					{
						NotAdd = TRUE;
						PtrOffset = EndOffset;
						goto NextFileChunk;
					}
				}
				else
				{
					CurOffset += 4;
					RtlCopyMemory(&IndexLength, pDecompress + CurOffset, 8);
					CurOffset += 8;
					CurOffset += IndexLength.LowPart;
				}
			}

			PtrOffset += 8;
			item.isM2Format = TRUE;
			ULONG HashInfo = 0;
			RtlCopyMemory(&HashInfo, (pDecompress + PtrOffset), 4);
			item.yuzu.Hash = HashInfo;
			PtrOffset += 4;
			USHORT FileNameLen = 0;
			RtlCopyMemory(&FileNameLen, (pDecompress + PtrOffset), 2);
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
				PtrOffset += (FileNameLen) * 2;
			}
				
			this->isM2Format = TRUE;

			if (!wcscmp(item.yuzu.Name.c_str(), ProtectionInfo))
			{
				NotAdd = TRUE;
			}
		}
		else
		{
			item.isM2Format = FALSE;
			this->isM2Format = FALSE;
		}

		if (item.isM2Format)
		{
			OutputString(L"Found M2-SubChunk\n");
			wsprintfW(PosInfo, L"[Middle]Pos : %08x\n", PtrOffset);
			OutputString(PosInfo);
		}
		else
		{
			OutputString(L"Normal\n");
			wsprintfW(PosInfo, L"[Middle]Pos : %08x\n", PtrOffset);
			OutputString(PosInfo);
		}

		if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_FILE, 4) != 4)
		{
			OutputString(L"[info]No File Chunk Magic\n");
			MessageBoxW(NULL, L"Internal Error", L"KrkrExtract", MB_OK);
			GlobalData::GetGlobalData()->isRunning = FALSE;
			return FALSE;
		}

		//File Maigc
		PtrInc(4);
		ULONG64 FileChunkSize64 = 0;
		CopyMemory(&FileChunkSize64, (PtrOffset + pDecompress), 8);
		PtrInc(8);

		wsprintfW(PosInfo, L"FileChunkSize : %08x\n", (ULONG32)FileChunkSize64);
		OutputString(PosInfo);

		ULONG SavePtrOffset = PtrOffset;
		ULONG32 FileChunkSize32 = (ULONG32)FileChunkSize64;
		ULONG iPos = 0;

		while (iPos < FileChunkSize32)
		{
			if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_INFO, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 InfoChunkSize = 0;
				RtlCopyMemory(&InfoChunkSize, (PtrOffset + pDecompress), sizeof(ULONG64));
				wsprintfW(PosInfo, L"Info Chunk : %08x\n", InfoChunkSize);
				OutputString(PosInfo);

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


				//某高考 的 支持
				WCHAR* FakeName = (WCHAR*)(PtrOffset + pDecompress);
				if ((item.info.FileNameLength > MAX_PATH && (ULONG)InfoChunkSize == 0x20 &&
					(FakeName[0] != L'$' && FakeName[1] != L'$' && FakeName[2] != L'$'))
					|| this->RawExtract)
					//并不稳定，最好是对齐比较，自动获取到下一个Sign的长度
				{
					this->RawExtract = TRUE;
					PtrOffset -= 2;
					iPos -= 2;
					item.info.FileNameLength = 0xFFFF;
					ULONG FakeNameSegm1, FakeNameSegm2, FakeNameSegm3;

					FakeNameSegm1 = *(PULONG)(PtrOffset + pDecompress);
					PtrInc(4);
					iPosAdd(4);
					FakeNameSegm2 = *(PULONG)(PtrOffset + pDecompress);
					PtrInc(4);
					iPosAdd(4);
					FakeNameSegm3 = *(PULONG)(PtrOffset + pDecompress);
					PtrInc(4);
					iPosAdd(4);

					WCHAR FakeNameStr[60] = { 0 };
					wsprintfW(FakeNameStr, L"%08X-%08X-%08X", FakeNameSegm1, FakeNameSegm2, FakeNameSegm3);
					item.info.FileName = FakeNameStr;
				}
				else
				{
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
					if (WideFileName[0] == L'$' && WideFileName[1] == L'$' && WideFileName[2] == L'$')
					{
						//FileChunkSize32
						PtrOffset = SavePtrOffset;
						PtrOffset += FileChunkSize32;
						iPos = FileChunkSize32;
						NotAdd = TRUE;
					}
					else if (item.info.FileNameLength > MAX_PATH)
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

					if (!wcscmp(ProtectionInfo, WideFileName.c_str()))
					{
						NotAdd = TRUE;
					}
				}
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_SEGM, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				item.segm.ChunkSize.QuadPart = TempChunkSize;
				wsprintfW(PosInfo, L"Segm Chunk : %08x\n", TempChunkSize);
				OutputString(PosInfo);
				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.segm.segm[0].bZlib), (PtrOffset + pDecompress), sizeof(BOOL));
				CopyMemory(&(item.segm.segm[0].Offset), (PtrOffset + pDecompress + sizeof(BOOL)), sizeof(ULONG64));
				CopyMemory(&(item.segm.segm[0].OriginalSize), (PtrOffset + pDecompress + sizeof(BOOL) + sizeof(ULONG64)), sizeof(ULONG64));
				CopyMemory(&(item.segm.segm[0].ArchiveSize), (PtrOffset + pDecompress + sizeof(BOOL) + sizeof(ULONG64) * 2), sizeof(ULONG64));
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
			else if (RtlCompareMemory((PtrOffset + pDecompress), CHUNK_MAGIC_ADLR, 4) == 4)
			{
				PtrInc(4);
				iPosAdd(4);

				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				wsprintfW(PosInfo, L"Adlr Chunk : %08x\n", TempChunkSize);
				OutputString(PosInfo);
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

				wsprintfW(PosInfo, L"Time Chunk : %08x\n", TempChunkSize);
				OutputString(PosInfo);
				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
			else
			{
				UCHAR UnknownMagic[4];
				RtlCopyMemory(UnknownMagic, (PtrOffset + pDecompress), 4);
				//Magic
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);

				RtlZeroMemory(PosInfo, sizeof(PosInfo));
				wsprintfW(PosInfo, L"Found Unknown Chunk at %08x\n", PtrOffset);
				OutputString(PosInfo);
				RtlZeroMemory(PosInfo, sizeof(PosInfo));
				wsprintfW(PosInfo, L"Unknown Chunk [%c%c%c%c] : %08x\n", UnknownMagic[0], UnknownMagic[1],
					UnknownMagic[2], UnknownMagic[3], TempChunkSize);
				OutputString(PosInfo);
				//ChunkSize
				PtrInc(8);
				iPosAdd(8);
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
		}

		NextFileChunk:

		if (!NotAdd)
		{
			if (item.isM2Format)
			{
				WCHAR Info2[300] = { 0 };
				wsprintfW(Info2, L"[Found File:]%s\n", item.yuzu.Name.c_str());
				OutputString(Info2);
			}
			else
			{
				WCHAR Info2[300] = { 0 };
				wsprintfW(Info2, L"[Found File:]%s\n", item.info.FileName.c_str());
				OutputString(Info2);
			}
			ItemVector.push_back(item);
			ItemNormalVector.push_back(item);
		}
		OutputString(L"In Index : ");
		OutputString(item.info.FileName.c_str());
		OutputString(L"\n");

		wsprintfW(PosInfo, L"[End]Pos : %08x\n", PtrOffset);
		OutputString(PosInfo);
	}
	//End loop

	if (isM2Format)
	{
		SetCount(ItemVector.size());
		WCHAR InfoW[260] = { 0 };
		wsprintfW(InfoW, L"File Count : [%d]\n", ItemVector.size());
		OutputString(InfoW);
	}
	else
	{
		SetCount(ItemNormalVector.size());
		WCHAR InfoW[260] = { 0 };
		wsprintfW(InfoW, L"File Count : [%d]\n", ItemNormalVector.size());
		OutputString(InfoW);
	}
	return TRUE;
}

//Full Path Name
void FormatPath(wstring& package, ttstr& out)
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

wstring FormatPathFull(const wchar_t * path)
{
	if (RtlCompareMemory(path, L"file:", 10) == 10)
	{
		return path;
	}
	else if (path[1] == L':' && (path[0] <= L'Z' && path[0] >= L'A' || path[0] <= L'z' && path[0] >= L'a')) 
	{
		wstring res(L"file://./");
		res += path[0];
		unsigned int i;
		for (i = 2; path[i] != 0; ++i) 
		{
			if (path[i] == '\\')
			{
				res += '/';
			}
			else
			{
				res += path[i];
			}
		}
		return res;
	}
	else 
	{
		unsigned int i, flag = 0;
		for (i = 0; path[i] != 0; ++i)
		{
			if (path[i] == '/' || path[i] == '\\' || path[i] == '*')
			{
				flag = 1;
				break;
			}
		}
		if (!flag) 
		{
			wchar_t * buffer = new wchar_t[512];
			GetCurrentDirectoryW(512, buffer);
			wstring res = FormatPathFull((wstring(buffer) + L'/' + path).c_str());
			delete[] buffer;
			return res;
		}
	}
	return L"";
}


//查询M2-Krkrz的Sign
ULONG WINAPI PluginHook::FindChunkMagicFirst(PBYTE pDecompress, ULONG Size)
{
	ULONG PtrOffset = 0;
	ULONG PackType = PackInfo::UnknownPack;

	while (PtrOffset < Size)
	{
		BOOL     NotAdd = FALSE;
		XP3Index item;

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
			PtrOffset += (FileNameLen + 1) * 2;

			RtlCopyMemory(M2ChunkMagic, CHUNK_MAGIC_YUZU, 4);
			return S_OK;
		}
		else
		{
			if (RtlCompareMemory(pDecompress + PtrOffset, CHUNK_MAGIC_KRKRZ_M2, 4) == 4)
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
				PtrOffset += (FileNameLen + 1) * 2;

				RtlCopyMemory(M2ChunkMagic, CHUNK_MAGIC_KRKRZ_M2, 4);

				return S_OK;
			}
		}

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
				return PackInfo::UnknownPack;
			}
		}
		//File Maigc
		PtrInc(4);
		ULONG64 FileChunkSize64 = 0;
		CopyMemory(&FileChunkSize64, (PtrOffset + pDecompress), 8);
		PtrInc(8);

		ULONG32 FileChunkSize32 = (ULONG32)FileChunkSize64;
		ULONG iPos = 0;

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
				PtrInc((item.info.FileNameLength + 1) * 2);
				iPosAdd((item.info.FileNameLength + 1) * 2);
				item.info.FileName = WideFileName;
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
							return S_OK;
						}
					}
				}
			}
			//Enum Normal Chunk
		}
		//Scan Normal Chunk Block
	}
	return S_FALSE;
}

wstring GetPackageName(wstring& fileName)
{
	wstring temp(fileName);
	wstring::size_type pos = temp.find_last_of(L"\\");

	if (pos != wstring::npos)
	{
		temp = temp.substr(pos + 1, temp.length());
	}

	wstring temp2(temp);
	wstring::size_type pos2 = temp2.find_last_of(L"/");
	if (pos2 != wstring::npos)
	{
		temp2 = temp2.substr(pos + 1, temp2.length());
	}
	return temp2;
}

wstring GetExtensionUpper(wstring& FileName)
{
	wstring OutName = PathFindExtensionW(FileName.c_str());
	std::transform(OutName.begin(), OutName.end(), OutName.begin(), toupper);
	return OutName;
}

wstring GetExtensionUpperV2(wstring& FileName)
{
	wstring OutName;
	wstring::size_type pos = FileName.find_last_of(L".");
	if (pos != wstring::npos)
	{
		OutName = FileName.substr(pos, FileName.length());
	}
	std::transform(OutName.begin(), OutName.end(), OutName.begin(), toupper);
	return OutName;
}

void FormatArc(wstring& package, ttstr& out)
{
	out.Clear();
	out = L"archive://./";
	out += GetPackageName(package).c_str();
	out += L"/";
}

//#define DebugOutput

VOID DecryptWorker(ULONG EncryptOffset, PBYTE pBuffer, ULONG BufferSize, ULONG Hash)
{
	tTVPXP3ExtractionFilterInfo Info(0, pBuffer, BufferSize, Hash);
	if (GlobalData::GetGlobalData()->pfGlobalXP3Filter != nullptr)
	{
		GlobalData::GetGlobalData()->pfGlobalXP3Filter(&Info);
	}
}

//实际上，目录大概是这样的：
//file://./e/yuzusoft/sa/data.xp3>16d64654fdfd1

//检查png!!
HRESULT WINAPI PluginHook::DumpFile()
{
	vector<wstring> Failed;
	ttstr out;
	ttstr ArcOut;
	FormatPath(wstring(wFileName), out);
	FormatArc(wstring(wFileName), ArcOut);
	if (ItemVector.size() == 0)
	{
		GlobalData::GetGlobalData()->isRunning = FALSE;
		return S_OK;
	}

#ifdef DebugOutput
	WinFile FileList;
	if (FileList.Open(L"FileList.txt", WinFile::FileWrite) == S_OK)
	{
		CHAR* Ending = "\r\n";
		if (isM2Format)
		{
			for (auto it : ItemVector)
			{
				CHAR Info[512] = { 0 };
				WideCharToMultiByte(CP_UTF8, 0, it.info.FileName.c_str(), it.info.FileName.length(), Info, 512, nullptr, nullptr);
				FileList.Write((PBYTE)Info, lstrlenA(Info));
				FileList.Write((PBYTE)Ending, lstrlenA(Ending));
			}
		}
		else
		{
			for (auto it : ItemNormalVector)
			{
				CHAR Info[512] = { 0 };
				WideCharToMultiByte(CP_UTF8, 0, it.c_str(), it.length(), Info, 512, nullptr, nullptr);
				FileList.Write((PBYTE)Info, lstrlenA(Info));
				FileList.Write((PBYTE)Ending, lstrlenA(Ending));
			}
		}
		FileList.Release();
	}
#endif

	DWORD iPos = 1;

	if (this->RawExtract == FALSE)
	{
		if (isM2Format)
		{
			for (auto it : ItemVector)
			{
				if (it.isM2Format)
				{
					SetCurFile(iPos);
					if (it.yuzu.Name.length() == 0)
					{
						it.yuzu.Name = it.info.FileName;
					}
					ttstr outFile = ArcOut + it.yuzu.Name.c_str();
					wstring Info = L"[M2]";
					Info += outFile.c_str();
					Info += L"\n";
					OutputString(Info.c_str());

					wstring ExtName = GetExtensionUpperV2(it.yuzu.Name);

					if (lstrcmpW(ExtName.c_str(), L".PNG") == 0)
					{
						if (GlobalData::GetGlobalData()->GetPngFlag() == PNG_SYS)
						{
							OutputString(L"Using Build-in Decode Mode\n");

							WCHAR CurDir[MAX_PATH] = { 0 };
							GetCurrentDirectoryW(MAX_PATH, CurDir);
							wstring outFilePath = CurDir;
							outFilePath += L"\\outPath\\";
							outFilePath += GetPackageName(wstring(this->wFileName));
							outFilePath += L"\\";
							outFilePath += it.yuzu.Name.c_str();

							wstring OutDebug1(L"Ori Path : ");
							OutDebug1 += outFilePath;
							OutDebug1 += L"\n";
							OutputString(OutDebug1.c_str());
							OutputString(FormatPathFull(outFilePath.c_str()).c_str());
							OutputString(L"\n");

							this->AddPath(outFilePath.c_str());
							SavePng(GetPackageName(it.yuzu.Name).c_str(), FormatPathFull(outFilePath.c_str()).c_str());
						}
						else if (GlobalData::GetGlobalData()->GetPngFlag() == PNG_RAW)
						{
							IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);

							if (st == NULL)
							{
								Info.clear();
								Info = L"Failed to open ";
								Info += outFile.c_str();
								Info += L" \n";
								OutputString(Info.c_str());
								continue;
							}

							STATSTG t;
							st->Stat(&t, STATFLAG_DEFAULT);
							unsigned long long ss = t.cbSize.QuadPart, now = 0;

							wstring outFilePath = L"outPath\\";
							outFilePath += GetPackageName(wstring(this->wFileName));
							outFilePath += L"\\";
							outFilePath += it.yuzu.Name.c_str();
							this->AddPath(outFilePath.c_str());

							HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
							if (hFile == INVALID_HANDLE_VALUE)
							{
								Info.clear();
								Info = L"Failed to write ";
								Info += it.yuzu.Name.c_str();
								Info += L" \n";
								OutputString(Info.c_str());
								Failed.push_back(it.yuzu.Name);
								continue;
							}

							ULONG size, os, tmp;
							unsigned int last_eql_cnt = 0;
							char last_char = 0;
							static char buffer[1024 * 64];

							while (now < ss)
							{
								st->Read(&buffer, sizeof(buffer), &size);
								now += size;
								tmp = 0;
								while (tmp < size)
								{
									WriteFile(hFile, buffer, size, &os, NULL);
									tmp += os;
								}
							}
							CloseHandle(hFile);
						}
					}
					else if (lstrcmpW(ExtName.c_str(), L".TLG") == 0)
					{
						if (GlobalData::GetGlobalData()->GetTlgFlag() == TLG_RAW)
						{
							OutputString(L"Decoding tlg image to raw file...\n");
							IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);

							if (st == NULL)
							{
								Info.clear();
								Info = L"Failed to open ";
								Info += outFile.c_str();
								Info += L" \n";
								OutputString(Info.c_str());
								continue;
							}

							STATSTG t;
							st->Stat(&t, STATFLAG_DEFAULT);
							unsigned long long ss = t.cbSize.QuadPart, now = 0;

							wstring outFilePath = L"outPath\\";
							outFilePath += GetPackageName(wstring(this->wFileName));
							outFilePath += L"\\";
							outFilePath += it.yuzu.Name.c_str();
							this->AddPath(outFilePath.c_str());
							HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
							if (hFile == INVALID_HANDLE_VALUE)
							{
								Info.clear();
								Info = L"Failed to write ";
								Info += it.yuzu.Name.c_str();
								Info += L" \n";
								OutputString(Info.c_str());
								Failed.push_back(it.info.FileName);
								continue;
							}

							ULONG size, os, tmp;
							unsigned int last_eql_cnt = 0;
							char last_char = 0;
							static char buffer[1024 * 64];

							while (now < ss)
							{
								st->Read(&buffer, sizeof(buffer), &size);
								now += size;
								tmp = 0;
								while (tmp < size)
								{
									WriteFile(hFile, buffer, size, &os, NULL);
									tmp += os;
								}
							}
							CloseHandle(hFile);
						}
						else if (GlobalData::GetGlobalData()->GetTlgFlag() == TLG_BUILDIN)
						{
							OutputString(L"Decoding tlg image to bmp...\n");
							IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);

							if (st == NULL)
							{
								Info.clear();
								Info = L"Failed to open ";
								Info += outFile.c_str();
								Info += L" \n";
								OutputString(Info.c_str());
								continue;
							}

							STATSTG t;
							st->Stat(&t, STATFLAG_DEFAULT);
							unsigned long long ss = t.cbSize.QuadPart, now = 0;

							wstring outFilePath = L"outPath\\";
							outFilePath += GetPackageName(wstring(this->wFileName));
							outFilePath += L"\\";
							outFilePath += it.yuzu.Name.c_str();
							this->AddPath(outFilePath.c_str());

							ULONG size, os;
							unsigned int last_eql_cnt = 0;
							char last_char = 0;

							ULONG Size = t.cbSize.LowPart;
							PBYTE Buffer = nullptr;
							Buffer = (PBYTE)CMem::Alloc(Size);
							if (Buffer == nullptr)
							{
								MessageBoxW(Gui, L"Failed to Allocate memory\n"
									L"for tlg Build-in Decoder", L"KrkrExtract", MB_OK);
							}
							PBYTE OutBuffer = nullptr;
							ULONG OutSize = 0;
							BOOL  TempDecode = TRUE;

							st->Read(Buffer, Size, &size);

							PBYTE Magic = Buffer;
							if (RtlCompareMemory(Magic, KRKR2_TLG5_MAGIC, 6) == 6)
							{
								if (!DecodeTLG5(Buffer, Size, (PVOID*)&OutBuffer, &OutSize))
								{
									TempDecode = FALSE;
									OutSize = Size;
									OutBuffer = Buffer;
								}
							}
							else if (RtlCompareMemory(Magic, KRKR2_TLG6_MAGIC, 6) == 6)
							{
								if (!DecodeTLG6(Buffer, Size, (PVOID*)&OutBuffer, &OutSize))
								{
									TempDecode = FALSE;
									OutSize = Size;
									OutBuffer = Buffer;
								}
							}//KRKR2_TLG0_MAGIC
							else if (RtlCompareMemory(Magic, KRKR2_TLG0_MAGIC, 6) == 6)
							{
								//Dummy Header?
								//I hope this header will never used for extraction in the future.
								if (RtlCompareMemory(Buffer + 0xF, KRKR2_TLG5_MAGIC, 6) == 6)
								{
									if (!DecodeTLG5(Buffer + 0xF, Size - 0xF, (PVOID*)&OutBuffer, &OutSize))
									{
										TempDecode = FALSE;
										OutSize = Size;
										OutBuffer = Buffer;
									}
								}
								else
								{
									if (!DecodeTLG6(Buffer + 0xF, Size - 0xF, (PVOID*)&OutBuffer, &OutSize))
									{
										TempDecode = FALSE;
										OutSize = Size;
										OutBuffer = Buffer;
									}
								}
							}
							else
							{
								TempDecode = FALSE;
								OutSize = Size;
								OutBuffer = Buffer;
							}

							HANDLE hFile = INVALID_HANDLE_VALUE;

							if (!TempDecode)
							{
								hFile = CreateFileW(outFilePath.c_str(),
									GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
							}
							else
							{
								hFile = CreateFileW((outFilePath + L".bmp").c_str(),
									GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
							}

							if (hFile == INVALID_HANDLE_VALUE)
							{
								Info.clear();
								Info = L"Failed to write ";
								Info += it.yuzu.Name.c_str();
								if (TempDecode)
								{
									Info += L".bmp";
								}
								Info += L" \n";
								OutputString(Info.c_str());
								Failed.push_back(it.info.FileName);
								continue;
							}

							WriteFile(hFile, OutBuffer, OutSize, &os, NULL);
							CloseHandle(hFile);
							CMem::Free(Buffer);
							if (TempDecode)
							{
								CMem::Free(OutBuffer);
							}
						}
						else if (GlobalData::GetGlobalData()->GetTlgFlag() == TLG_SYS)
						{
							OutputString(L"Using System Decode Mode[TLG]\n");

							WCHAR CurDir[MAX_PATH] = { 0 };
							GetCurrentDirectoryW(MAX_PATH, CurDir);
							wstring outFilePath = CurDir;
							outFilePath += L"\\outPath\\";
							outFilePath += GetPackageName(wstring(this->wFileName));
							outFilePath += L"\\";
							outFilePath += it.yuzu.Name.c_str();

							wstring OutDebug1(L"Ori Path : ");
							OutDebug1 += outFilePath;
							OutDebug1 += L"\n";
							OutputString(OutDebug1.c_str());
							OutputString(FormatPathFull(outFilePath.c_str()).c_str());
							OutputString(L"\n");

							this->AddPath(outFilePath.c_str());
							SavePng(GetPackageName(it.yuzu.Name).c_str(), FormatPathFull((outFilePath + L".png").c_str()).c_str());
						}
						//TLG_PNG
						else
						{
							OutputString(L"Decoding tlg image to png file(Build-in)...\n");
							IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);
							if (st == NULL)
							{
								Info.clear();
								Info = L"Failed to open ";
								Info += outFile.c_str();
								Info += L" \n";
								OutputString(Info.c_str());
								continue;
							}

							STATSTG t;
							st->Stat(&t, STATFLAG_DEFAULT);
							unsigned long long ss = t.cbSize.QuadPart, now = 0;

							wstring outFilePath = L"outPath\\";
							outFilePath += GetPackageName(wstring(this->wFileName));
							outFilePath += L"\\";
							outFilePath += it.yuzu.Name.c_str();
							this->AddPath(outFilePath.c_str());

							ULONG size, os;
							unsigned int last_eql_cnt = 0;
							char last_char = 0;

							ULONG Size = t.cbSize.LowPart;
							PBYTE Buffer = nullptr;
							Buffer = (PBYTE)CMem::Alloc(Size);
							if (Buffer == nullptr)
							{
								MessageBoxW(Gui, L"Failed to Allocate memory\n"
									L"for tlg Build-in Decoder", L"KrkrExtract", MB_OK);
							}
							PBYTE OutBuffer = nullptr;
							ULONG OutSize = 0;
							BOOL  TempDecode = TRUE;

							st->Read(Buffer, Size, &size);

							PBYTE Magic = Buffer;
							if (RtlCompareMemory(Magic, KRKR2_TLG5_MAGIC, 6) == 6)
							{
								if (!DecodeTLG5(Buffer, Size, (PVOID*)&OutBuffer, &OutSize))
								{
									TempDecode = FALSE;
									OutSize = Size;
									OutBuffer = Buffer;
								}
							}
							else if (RtlCompareMemory(Magic, KRKR2_TLG6_MAGIC, 6) == 6)
							{
								if (!DecodeTLG6(Buffer, Size, (PVOID*)&OutBuffer, &OutSize))
								{
									TempDecode = FALSE;
									OutSize = Size;
									OutBuffer = Buffer;
								}
							}//KRKR2_TLG0_MAGIC
							else if (RtlCompareMemory(Magic, KRKR2_TLG0_MAGIC, 6) == 6)
							{
								//Dummy Header?
								//I hope this header will never used for extraction in the future.
								if (RtlCompareMemory(Buffer + 0xF, KRKR2_TLG5_MAGIC, 6) == 6)
								{
									if (!DecodeTLG5(Buffer + 0xF, Size - 0xF, (PVOID*)&OutBuffer, &OutSize))
									{
										TempDecode = FALSE;
										OutSize = Size;
										OutBuffer = Buffer;
									}
								}
								else
								{
									if (!DecodeTLG6(Buffer + 0xF, Size - 0xF, (PVOID*)&OutBuffer, &OutSize))
									{
										TempDecode = FALSE;
										OutSize = Size;
										OutBuffer = Buffer;
									}
								}
							}
							else
							{
								TempDecode = FALSE;
								OutSize = Size;
								OutBuffer = Buffer;
							}

							HANDLE hFile = INVALID_HANDLE_VALUE;

							if (TempDecode)
							{
								if (Bmp2PNG(OutBuffer, OutSize, (outFilePath + L".png").c_str()) != B2P_ERROR)
								{
									//None
								}
								else
								{
									hFile = CreateFileW(outFilePath.c_str(),
										GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

									if (hFile == INVALID_HANDLE_VALUE)
									{
										Info.clear();
										Info = L"Failed to write ";
										Info += it.yuzu.Name.c_str();
										Info += L" \n";
										OutputString(Info.c_str());
										Failed.push_back(it.yuzu.Name);
										continue;
									}

									WriteFile(hFile, OutBuffer, OutSize, &os, NULL);
									CloseHandle(hFile);
								}
							}
							else
							{
								hFile = CreateFileW(outFilePath.c_str(),
									GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

								if (hFile == INVALID_HANDLE_VALUE)
								{
									Info.clear();
									Info = L"Failed to write ";
									Info += it.yuzu.Name.c_str();
									if (TempDecode)
									{
										Info += L".png";
									}
									Info += L" \n";
									OutputString(Info.c_str());
									Failed.push_back(it.yuzu.Name);
									continue;
								}

								WriteFile(hFile, OutBuffer, OutSize, &os, NULL);
								CloseHandle(hFile);
							}

							CMem::Free(Buffer);
							if (TempDecode)
							{
								CMem::Free(OutBuffer);
							}
						}
					}
					else if (ExtName == L".PSB" ||
							 ExtName == L".SCN" ||
							 ExtName == L".MTN" ||
							 ExtName == L".PIMG")
					{

						IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);
						if (st == NULL)
						{
							Info.clear();
							Info = L"Failed to open ";
							Info += outFile.c_str();
							Info += L" \n";
							OutputString(Info.c_str());
							continue;
						}

						STATSTG t;
						st->Stat(&t, STATFLAG_DEFAULT);
						unsigned long long ss = t.cbSize.QuadPart, now = 0;

						wstring outFilePath = L"outPath\\";
						outFilePath += GetPackageName(wstring(this->wFileName));
						outFilePath += L"\\";
						outFilePath += it.yuzu.Name.c_str();
						this->AddPath(outFilePath.c_str());

						if (GlobalData::GetGlobalData()->PsbFlagOn(PSB_RAW) ||
							GlobalData::GetGlobalData()->PsbFlagOn(PSB_ALL))
						{
							
							HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
							if (hFile == INVALID_HANDLE_VALUE)
							{
								Info.clear();
								Info = L"Failed to write ";
								Info += it.yuzu.Name.c_str();
								Info += L" \n";
								OutputString(Info.c_str());
								Failed.push_back(it.info.FileName);
								continue;
							}
							ULONG size, os, tmp;
							unsigned int last_eql_cnt = 0;
							char last_char = 0;
							static char buffer[1024 * 64];

							while (now < ss)
							{
								st->Read(&buffer, sizeof(buffer), &size);
								now += size;
								tmp = 0;
								while (tmp < size)
								{
									WriteFile(hFile, buffer, size, &os, NULL);
									tmp += os;
								}
							}
							CloseHandle(hFile);
						}
						
						if (GlobalData::GetGlobalData()->PsbFlagOn(PSB_ALL) ||
							GlobalData::GetGlobalData()->PsbFlagOn(PSB_TEXT) ||
							GlobalData::GetGlobalData()->PsbFlagOn(PSB_DECOM) ||
							GlobalData::GetGlobalData()->PsbFlagOn(PSB_ANM))
						{
							ExtractPsb(st, GlobalData::GetGlobalData()->PsbFlagOn(PSB_IMAGE) ||
										   GlobalData::GetGlobalData()->PsbFlagOn(PSB_ANM),
										   GlobalData::GetGlobalData()->PsbFlagOn(PSB_DECOM),
										   ExtName, outFilePath.c_str());
						}
					}
					else if (GlobalData::GetGlobalData()->GetTextFlag() == TEXT_DECODE &&
						(
						lstrcmpW(ExtName.c_str(), L".KSD") == 0 ||
						lstrcmpW(ExtName.c_str(), L".KDT") == 0 ||
						lstrcmpW(ExtName.c_str(), L".TXT") == 0 ||
						lstrcmpW(ExtName.c_str(), L".KS") == 0  ||
						lstrcmpW(ExtName.c_str(), L".CSV") == 0 ||
						lstrcmpW(ExtName.c_str(), L".PSB") == 0 ||
						lstrcmpW(ExtName.c_str(), L".FUNC") == 0 ||
						lstrcmpW(ExtName.c_str(), L".STAND") == 0 ||
						lstrcmpW(ExtName.c_str(), L".ASD") == 0 ||
						lstrcmpW(ExtName.c_str(), L".INI") == 0 ||
						lstrcmpW(ExtName.c_str(), L".TJS") == 0 ))
					{
						IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);

						if (st == NULL)
						{
							Info.clear();
							Info = L"Failed to open ";
							Info += outFile.c_str();
							Info += L" \n";
							OutputString(Info.c_str());
							continue;
						}
						OutputString(L"Decoding Text...\n", 1);

						STATSTG t;
						st->Stat(&t, STATFLAG_DEFAULT);
						unsigned long long ss = t.cbSize.QuadPart, now = 0;

						wstring outFilePath = L"outPath\\";
						outFilePath += GetPackageName(wstring(this->wFileName));
						outFilePath += L"\\";
						outFilePath += it.yuzu.Name.c_str();
						this->AddPath(outFilePath.c_str());

						PBYTE OriBuffer = nullptr;
						ULONG OriSize = t.cbSize.LowPart;
						ULONG ReadSize = 0;
						OriBuffer = (PBYTE)CMem::Alloc(OriSize);
						if (OriBuffer == nullptr)
						{
							goto TextIgnOpM2;
						}
						st->Read(OriBuffer, OriSize, &ReadSize);

						if (DecodeText(OriBuffer, OriSize, Gui, (outFilePath + L".txt").c_str()) != -1)
						{
							if (OriBuffer)
							{
								CMem::Free(OriBuffer);
								OriBuffer = nullptr;
							}
						}
						else
						{
							goto TextIgnOpM2;
						}
					}
					//Other Format
					else
					{
					TextIgnOpM2:

						IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);

						if (st == NULL)
						{
							outFile = out + it.info.FileName.c_str();
							st = TVPCreateIStream(outFile, TJS_BS_READ);
							if (st == NULL)
							{
								Info.clear();
								Info = L"Failed to open ";
								Info += outFile.c_str();
								Info += L" \n";
								OutputString(Info.c_str());
								Failed.push_back(it.yuzu.Name);
								continue;
							}
						}

						STATSTG t;
						st->Stat(&t, STATFLAG_DEFAULT);
						unsigned long long ss = t.cbSize.QuadPart, now = 0;

						wstring outFilePath = L"outPath\\";

						outFilePath += GetPackageName(wstring(this->wFileName));
						outFilePath += L"\\";
						outFilePath += it.yuzu.Name.c_str();
						this->AddPath(outFilePath.c_str());
						HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hFile == INVALID_HANDLE_VALUE)
						{
							Info.clear();
							Info = L"Failed to write ";
							Info += it.yuzu.Name.c_str();
							Info += L" \n";
							OutputString(Info.c_str());
							continue;
						}

						ULONG size, os, tmp;
						unsigned int last_eql_cnt = 0;
						char last_char = 0;
						static char buffer[1024 * 64];

						while (now < ss)
						{
							st->Read(&buffer, sizeof(buffer), &size);
							now += size;
							tmp = 0;
							while (tmp < size)
							{
								WriteFile(hFile, buffer, size, &os, NULL);
								tmp += os;
							}
						}
						CloseHandle(hFile);
					}
					iPos++;
				}
				else //Normal Format(startup.tjs)
				{
					SetCurFile(iPos);
					ttstr outFile = out + it.info.FileName.c_str();
					wstring Info = L"[M2-Normal]";
					Info += outFile.c_str();
					Info += L"\n";
					OutputString(Info.c_str());
					IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);

					if (st == NULL)
					{
						Info.clear();
						Info = L"Failed to open ";
						Info += outFile.c_str();
						Info += L" \n";
						OutputString(Info.c_str());
						continue;
					}

					STATSTG t;
					st->Stat(&t, STATFLAG_DEFAULT);
					unsigned long long ss = t.cbSize.QuadPart, now = 0;

					wstring outFilePath = L"outPath\\";

					outFilePath += GetPackageName(wstring(this->wFileName));
					outFilePath += L"\\";
					outFilePath += it.info.FileName.c_str();
					this->AddPath(outFilePath.c_str());
					HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile == INVALID_HANDLE_VALUE)
					{
						Info.clear();
						Info = L"Failed to write ";
						Info += it.info.FileName.c_str();
						Info += L" \n";
						OutputString(Info.c_str());
						Failed.push_back(it.info.FileName);
						//continue;
						continue;
					}

					ULONG size, os, tmp;
					unsigned int last_eql_cnt = 0;
					char last_char = 0;
					static char buffer[1024 * 64];

					while (now < ss)
					{
						st->Read(&buffer, sizeof(buffer), &size);
						now += size;
						tmp = 0;
						while (tmp < size)
						{
							WriteFile(hFile, buffer, size, &os, NULL);
							tmp += os;
						}
					}
					CloseHandle(hFile);
					iPos++;
				}
			}
		}
		else
		{
			for (auto it : ItemNormalVector)
			{
				SetCurFile(iPos);
				ttstr outFile = out + it.info.FileName.c_str();
				wstring Info = L"[Normal]";
				Info += outFile.c_str();
				Info += L"\n";
				OutputString(Info.c_str());

				wstring ExtName = GetExtensionUpperV2(it.info.FileName);
				if (lstrcmpW(ExtName.c_str(), L".PNG") == 0)
				{
					if (GlobalData::GetGlobalData()->GetPngFlag() == PNG_SYS ||
						GlobalData::GetGlobalData()->GetPngFlag() == PNG_ALL)
					{
						OutputString(L"Using Build-in Decode Mode[PNG]\n");

						WCHAR CurDir[MAX_PATH] = { 0 };
						GetCurrentDirectoryW(MAX_PATH, CurDir);
						wstring outFilePath = CurDir;
						outFilePath += L"\\outPath\\";
						outFilePath += GetPackageName(wstring(this->wFileName));
						outFilePath += L"\\";
						outFilePath += it.info.FileName.c_str();

						wstring OutDebug1(L"Ori Path : ");
						OutDebug1 += outFilePath;
						OutDebug1 += L"\n";
						OutputString(OutDebug1.c_str());
						OutputString(FormatPathFull(outFilePath.c_str()).c_str());
						OutputString(L"\n");

						this->AddPath(outFilePath.c_str());
						SavePng(GetPackageName(it.info.FileName).c_str(), FormatPathFull(outFilePath.c_str()).c_str());
					}
					else if (GlobalData::GetGlobalData()->GetPngFlag() == PNG_RAW ||
							 GlobalData::GetGlobalData()->GetPngFlag() == PNG_ALL)
					{
						IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);

						if (st == NULL)
						{
							Info.clear();
							Info = L"Failed to open ";
							Info += outFile.c_str();
							Info += L" \n";
							OutputString(Info.c_str());
							continue;
						}

						STATSTG t;
						st->Stat(&t, STATFLAG_DEFAULT);
						unsigned long long ss = t.cbSize.QuadPart, now = 0;

						wstring outFilePath = L"outPath\\";
						outFilePath += GetPackageName(wstring(this->wFileName));
						outFilePath += L"\\";
						outFilePath += it.info.FileName.c_str();
						this->AddPath(outFilePath.c_str());

						HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hFile == INVALID_HANDLE_VALUE)
						{
							Info.clear();
							Info = L"Failed to write ";
							Info += it.info.FileName.c_str();
							Info += L" \n";
							OutputString(Info.c_str());
							Failed.push_back(it.info.FileName);
							continue;
						}

						ULONG size, os, tmp;
						unsigned int last_eql_cnt = 0;
						char last_char = 0;
						static char buffer[1024 * 64];

						while (now < ss)
						{
							st->Read(&buffer, sizeof(buffer), &size);
							now += size;
							tmp = 0;
							while (tmp < size)
							{
								WriteFile(hFile, buffer, size, &os, NULL);
								tmp += os;
							}
						}
						CloseHandle(hFile);
					}
				}
				else if (lstrcmpW(ExtName.c_str(), L".TLG") == 0)
				{
					if (GlobalData::GetGlobalData()->GetTlgFlag() == TLG_RAW)
					{
						OutputString(L"Decoding tlg image to raw file...\n");
						IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);

						if (st == NULL)
						{
							Info.clear();
							Info = L"Failed to open ";
							Info += outFile.c_str();
							Info += L" \n";
							OutputString(Info.c_str());
							continue;
						}

						STATSTG t;
						st->Stat(&t, STATFLAG_DEFAULT);
						unsigned long long ss = t.cbSize.QuadPart, now = 0;

						wstring outFilePath = L"outPath\\";
						outFilePath += GetPackageName(wstring(this->wFileName));
						outFilePath += L"\\";
						outFilePath += it.info.FileName.c_str();
						this->AddPath(outFilePath.c_str());
						HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hFile == INVALID_HANDLE_VALUE)
						{
							Info.clear();
							Info = L"Failed to write ";
							Info += it.info.FileName.c_str();
							Info += L" \n";
							OutputString(Info.c_str());
							Failed.push_back(it.info.FileName);
							continue;
						}

						ULONG size, os, tmp;
						unsigned int last_eql_cnt = 0;
						char last_char = 0;
						static char buffer[1024 * 64];

						while (now < ss)
						{
							st->Read(&buffer, sizeof(buffer), &size);
							now += size;
							tmp = 0;
							while (tmp < size)
							{
								WriteFile(hFile, buffer, size, &os, NULL);
								tmp += os;
							}
						}
						CloseHandle(hFile);
					}
					else if (GlobalData::GetGlobalData()->GetTlgFlag() == TLG_BUILDIN)
					{
						OutputString(L"Decoding tlg image to bmp file(Build-in)...\n");
						IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);
				
						if (st == NULL)
						{
							Info.clear();
							Info = L"Failed to open ";
							Info += outFile.c_str();
							Info += L" \n";
							OutputString(Info.c_str());
							continue;
						}

						STATSTG t;
						st->Stat(&t, STATFLAG_DEFAULT);
						unsigned long long ss = t.cbSize.QuadPart, now = 0;

						wstring outFilePath = L"outPath\\";
						outFilePath += GetPackageName(wstring(this->wFileName));
						outFilePath += L"\\";
						outFilePath += it.info.FileName.c_str();
						this->AddPath(outFilePath.c_str());

						ULONG size, os;
						unsigned int last_eql_cnt = 0;
						char last_char = 0;
						
						ULONG Size = t.cbSize.LowPart;
						PBYTE Buffer = nullptr;
						Buffer = (PBYTE)CMem::Alloc(Size);
						WCHAR BmpInfo[100] = { 0 };
						wsprintfW(BmpInfo, L"TLG Raw Size : %d\n", Size);
						OutputString(BmpInfo);

						if (Buffer == nullptr)
						{
							MessageBoxW(Gui, L"Failed to Allocate memory\n"
								L"for tlg Build-in Decoder", L"KrkrExtract", MB_OK);
							continue;
						}
						PBYTE OutBuffer = nullptr;
						ULONG OutSize = 0;
						BOOL  TempDecode = TRUE;

						st->Read(Buffer, Size, &size);

						PBYTE Magic = Buffer;
						if (RtlCompareMemory(Magic, KRKR2_TLG5_MAGIC, 6) == 6)
						{
							if (!DecodeTLG5(Buffer, Size, (PVOID*)&OutBuffer, &OutSize))
							{
								TempDecode = FALSE;
								OutSize    = Size;
								OutBuffer  = Buffer;
							}
						}
						else if (RtlCompareMemory(Magic, KRKR2_TLG6_MAGIC, 6) == 6)
						{
							if (!DecodeTLG6(Buffer, Size, (PVOID*)&OutBuffer, &OutSize))
							{
								TempDecode = FALSE;
								OutSize = Size;
								OutBuffer = Buffer;
							}
						}//KRKR2_TLG0_MAGIC
						else if (RtlCompareMemory(Magic, KRKR2_TLG0_MAGIC, 6) == 6)
						{
							//Dummy Header?
							//I hope this header will never used for extraction in the future.
							if (RtlCompareMemory(Buffer + 0xF, KRKR2_TLG5_MAGIC, 6) == 6)
							{
								if (!DecodeTLG5(Buffer + 0xF, Size - 0xF, (PVOID*)&OutBuffer, &OutSize))
								{
									TempDecode = FALSE;
									OutSize = Size;
									OutBuffer = Buffer;
								}
							}
							else
							{
								if (!DecodeTLG6(Buffer + 0xF, Size - 0xF, (PVOID*)&OutBuffer, &OutSize))
								{
									TempDecode = FALSE;
									OutSize = Size;
									OutBuffer = Buffer;
								}
							}
						}
						else
						{
							TempDecode = FALSE;
							OutSize    = Size;
							OutBuffer  = Buffer;
						}

						HANDLE hFile = INVALID_HANDLE_VALUE;

						if (!TempDecode)
						{
							hFile = CreateFileW(outFilePath.c_str(),
								GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						}
						else
						{
							hFile = CreateFileW((outFilePath + L".bmp").c_str(),
								GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						}

						if (hFile == INVALID_HANDLE_VALUE)
						{
							Info.clear();
							Info = L"Failed to write ";
							Info += it.info.FileName.c_str();
							if (TempDecode)
							{
								Info += L".bmp";
							}
							Info += L" \n";
							OutputString(Info.c_str());
							Failed.push_back(it.info.FileName);
							continue;
						}

						WriteFile(hFile, OutBuffer, OutSize, &os, NULL);
						CloseHandle(hFile);
						CMem::Free(Buffer);
						if (TempDecode)
						{
							CMem::Free(OutBuffer);
						}
					}
					else if (GlobalData::GetGlobalData()->GetTlgFlag() == TLG_SYS)
					{
						OutputString(L"Using System Decode Mode[TLG]\n");

						WCHAR CurDir[MAX_PATH] = { 0 };
						GetCurrentDirectoryW(MAX_PATH, CurDir);
						wstring outFilePath = CurDir;
						outFilePath += L"\\outPath\\";
						outFilePath += GetPackageName(wstring(this->wFileName));
						outFilePath += L"\\";
						outFilePath += it.info.FileName.c_str();

						wstring OutDebug1(L"Ori Path : ");
						OutDebug1 += outFilePath;
						OutDebug1 += L"\n";
						OutputString(OutDebug1.c_str());
						OutputString(FormatPathFull(outFilePath.c_str()).c_str());
						OutputString(L"\n");

						this->AddPath(outFilePath.c_str());
						SavePng(GetPackageName(it.info.FileName).c_str(), FormatPathFull((outFilePath + L".png").c_str()).c_str());
					}
					//TLG_PNG
					else
					{
						OutputString(L"Decoding tlg image to png file(Build-in)...\n");
						IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);
						
						if (st == NULL)
						{
							Info.clear();
							Info = L"Failed to open ";
							Info += outFile.c_str();
							Info += L" \n";
							OutputString(Info.c_str());
							continue;
						}

						STATSTG t;
						st->Stat(&t, STATFLAG_DEFAULT);
						unsigned long long ss = t.cbSize.QuadPart, now = 0;

						wstring outFilePath = L"outPath\\";
						outFilePath += GetPackageName(wstring(this->wFileName));
						outFilePath += L"\\";
						outFilePath += it.info.FileName.c_str();
						this->AddPath(outFilePath.c_str());

						ULONG size, os;
						unsigned int last_eql_cnt = 0;
						char last_char = 0;

						ULONG Size = t.cbSize.LowPart;
						PBYTE Buffer = nullptr;
						Buffer = (PBYTE)CMem::Alloc(Size);
						if (Buffer == nullptr)
						{
							MessageBoxW(Gui, L"Failed to Allocate memory\n"
								L"for tlg Build-in Decoder", L"KrkrExtract", MB_OK);
						}
						PBYTE OutBuffer = nullptr;
						ULONG OutSize = 0;
						BOOL  TempDecode = TRUE;

						st->Read(Buffer, Size, &size);

						PBYTE Magic = Buffer;
						if (RtlCompareMemory(Magic, KRKR2_TLG5_MAGIC, 6) == 6)
						{
							if (!DecodeTLG5(Buffer, Size, (PVOID*)&OutBuffer, &OutSize))
							{
								TempDecode = FALSE;
								OutSize = Size;
								OutBuffer = Buffer;
							}
						}
						else if (RtlCompareMemory(Magic, KRKR2_TLG6_MAGIC, 6) == 6)
						{
							if (!DecodeTLG6(Buffer, Size, (PVOID*)&OutBuffer, &OutSize))
							{
								TempDecode = FALSE;
								OutSize = Size;
								OutBuffer = Buffer;
							}
						}//KRKR2_TLG0_MAGIC
						else if (RtlCompareMemory(Magic, KRKR2_TLG0_MAGIC, 6) == 6)
						{
							//Dummy Header?
							//I hope this header will never used for extraction in the future.
							if (RtlCompareMemory(Buffer + 0xF, KRKR2_TLG5_MAGIC, 6) == 6)
							{
								if (!DecodeTLG5(Buffer + 0xF, Size - 0xF, (PVOID*)&OutBuffer, &OutSize))
								{
									TempDecode = FALSE;
									OutSize = Size;
									OutBuffer = Buffer;
								}
							}
							else
							{
								if (!DecodeTLG6(Buffer + 0xF, Size - 0xF, (PVOID*)&OutBuffer, &OutSize))
								{
									TempDecode = FALSE;
									OutSize = Size;
									OutBuffer = Buffer;
								}
							}
						}
						else
						{
							TempDecode = FALSE;
							OutSize = Size;
							OutBuffer = Buffer;
						}
						
						HANDLE hFile = INVALID_HANDLE_VALUE;

						if (TempDecode)
						{
							if (Bmp2PNG(OutBuffer, OutSize, (outFilePath + L".png").c_str()) != B2P_ERROR)
							{
								//None
							}
							else
							{
								hFile = CreateFileW(outFilePath.c_str(),
									GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

								if (hFile == INVALID_HANDLE_VALUE)
								{
									Info.clear();
									Info = L"Failed to write ";
									Info += it.info.FileName.c_str();
									if (TempDecode)
									{
										Info += L".png";
									}
									Info += L" \n";
									OutputString(Info.c_str());
									Failed.push_back(it.info.FileName);
									continue;
								}

								WriteFile(hFile, OutBuffer, OutSize, &os, NULL);
								CloseHandle(hFile);
							}
						}
						else
						{
							hFile = CreateFileW(outFilePath.c_str(),
								GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						
							if (hFile == INVALID_HANDLE_VALUE)
							{
								Info.clear();
								Info = L"Failed to write ";
								Info += it.info.FileName.c_str();
								Info += L" \n";
								OutputString(Info.c_str());
								Failed.push_back(it.info.FileName);
								continue;
							}

							WriteFile(hFile, OutBuffer, OutSize, &os, NULL);
							CloseHandle(hFile);
						}

						CMem::Free(Buffer);
						if (TempDecode)
						{
							CMem::Free(OutBuffer);
						}
					}
				}
				else if (ExtName == L".PSB" ||
						 ExtName == L".SCN" ||
						 ExtName == L".PIMG" ||
						 ExtName == L".MTN")
				{
					IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);
				
					if (st == NULL)
					{
						Info.clear();
						Info = L"Failed to open ";
						Info += outFile.c_str();
						Info += L" \n";
						OutputString(Info.c_str());
						continue;
					}
					

					STATSTG t;
					st->Stat(&t, STATFLAG_DEFAULT);
					unsigned long long ss = t.cbSize.QuadPart, now = 0;

					wstring outFilePath = L"outPath\\";
					outFilePath += GetPackageName(wstring(this->wFileName));
					outFilePath += L"\\";
					outFilePath += it.info.FileName.c_str();
					this->AddPath(outFilePath.c_str());

					if (GlobalData::GetGlobalData()->PsbFlagOn(PSB_RAW) ||
						GlobalData::GetGlobalData()->PsbFlagOn(PSB_ALL))
					{
						HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hFile == INVALID_HANDLE_VALUE)
						{
							Info.clear();
							Info = L"Failed to write ";
							Info += it.info.FileName.c_str();
							Info += L" \n";
							OutputString(Info.c_str());
							Failed.push_back(it.info.FileName);
							continue;
						}
						
						ULONG size, os, tmp;
						unsigned int last_eql_cnt = 0;
						char last_char = 0;
						static char buffer[1024 * 64];

						while (now < ss)
						{
							st->Read(&buffer, sizeof(buffer), &size);
							now += size;
							tmp = 0;
							while (tmp < size)
							{
								WriteFile(hFile, buffer, size, &os, NULL);
								tmp += os;
							}
						}
						CloseHandle(hFile);
					}

					if (GlobalData::GetGlobalData()->PsbFlagOn(PSB_ALL) ||
						GlobalData::GetGlobalData()->PsbFlagOn(PSB_TEXT) ||
						GlobalData::GetGlobalData()->PsbFlagOn(PSB_DECOM) ||
						GlobalData::GetGlobalData()->PsbFlagOn(PSB_ANM))
					{
						ExtractPsb(st, GlobalData::GetGlobalData()->PsbFlagOn(PSB_IMAGE) ||
									   GlobalData::GetGlobalData()->PsbFlagOn(PSB_ANM),
									   GlobalData::GetGlobalData()->PsbFlagOn(PSB_DECOM),
									   ExtName, outFilePath.c_str());
					}
				}
				else if (GlobalData::GetGlobalData()->GetTextFlag() == TEXT_DECODE &&
					    (
						 lstrcmpW(ExtName.c_str(), L".KSD") == 0 ||
					     lstrcmpW(ExtName.c_str(), L".KDT") == 0 ||
						 lstrcmpW(ExtName.c_str(), L".TXT") == 0 ||
						 lstrcmpW(ExtName.c_str(), L".KS" ) == 0 ||
						 lstrcmpW(ExtName.c_str(), L".CSV") == 0 ||
						 lstrcmpW(ExtName.c_str(), L".FUNC") == 0 ||
						 lstrcmpW(ExtName.c_str(), L".STAND") == 0 ||
						 lstrcmpW(ExtName.c_str(), L".ASD") == 0   ||
						 lstrcmpW(ExtName.c_str(), L".INI") == 0   ||
						 lstrcmpW(ExtName.c_str(), L".TJS") == 0))
				{
					IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);
					
					if (st == NULL)
					{
						Info.clear();
						Info = L"Failed to open ";
						Info += outFile.c_str();
						Info += L" \n";
						OutputString(Info.c_str());
						continue;
					}

					OutputString(L"Decoding Text...\n", 1);

					STATSTG t;
					st->Stat(&t, STATFLAG_DEFAULT);
					unsigned long long ss = t.cbSize.QuadPart, now = 0;

					wstring outFilePath = L"outPath\\";
					outFilePath += GetPackageName(wstring(this->wFileName));
					outFilePath += L"\\";
					outFilePath += it.info.FileName.c_str();
					this->AddPath(outFilePath.c_str());

					PBYTE OriBuffer = nullptr;
					ULONG OriSize   = t.cbSize.LowPart;
					ULONG ReadSize  = 0;
					OriBuffer = (PBYTE)CMem::Alloc(OriSize);
					if (OriBuffer == nullptr)
					{
						goto TextIgnOp;
					}

					st->Read(OriBuffer, OriSize, &ReadSize);
					
					if (DecodeText(OriBuffer, OriSize, Gui, (outFilePath + L".txt").c_str()) != -1)
					{
						if (OriBuffer)
						{
							CMem::Free(OriBuffer);
							OriBuffer = nullptr;
						}
					}
					else
					{
					TextIgnOp:

						HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hFile == INVALID_HANDLE_VALUE)
						{
							Info.clear();
							Info = L"Failed to write ";
							Info += it.info.FileName.c_str();
							Info += L" \n";
							OutputString(Info.c_str());
							Failed.push_back(it.info.FileName);
							continue;
						}
						
						ULONG size, os, tmp;
						unsigned int last_eql_cnt = 0;
						char last_char = 0;
						static char buffer[1024 * 64];

						while (now < ss)
						{
							st->Read(&buffer, sizeof(buffer), &size);
							now += size;
							tmp = 0;
							while (tmp < size)
							{
								WriteFile(hFile, buffer, size, &os, NULL);
								tmp += os;
							}
						}
						CloseHandle(hFile);
					}
				}
				//Normal Format
				else
				{
					IStream * st = TVPCreateIStream(outFile, TJS_BS_READ);
					
					if (st == NULL)
					{
						Info.clear();
						Info = L"Failed to open ";
						Info += outFile.c_str();
						Info += L" \n";
						OutputString(Info.c_str());
						continue;
					}

					STATSTG t;
					st->Stat(&t, STATFLAG_DEFAULT);
					unsigned long long ss = t.cbSize.QuadPart, now = 0;

					wstring outFilePath = L"outPath\\";
					outFilePath += GetPackageName(wstring(this->wFileName));
					outFilePath += L"\\";
					outFilePath += it.info.FileName.c_str();
					this->AddPath(outFilePath.c_str());
					HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile == INVALID_HANDLE_VALUE)
					{
						Info.clear();
						Info = L"Failed to write ";
						Info += it.info.FileName.c_str();
						Info += L" \n";
						OutputString(Info.c_str());
						Failed.push_back(it.info.FileName);
						continue;
					}
					
					ULONG size, os, tmp;
					unsigned int last_eql_cnt = 0;
					char last_char = 0;
					static char buffer[1024 * 64];

					while (now < ss)
					{
						st->Read(&buffer, sizeof(buffer), &size);
						now += size;
						tmp = 0;
						while (tmp < size)
						{
							WriteFile(hFile, buffer, size, &os, NULL);
							tmp += os;
						}
					}
					CloseHandle(hFile);
				}
				iPos++;
			}
		}

	}
	else
	{
		WinFile FakeWorker;
		HRESULT FakeWorkerFlag = FakeWorker.Open(this->wFileName, WinFile::FileRead);
		for (auto it : ItemVector)
		{
			ULONG os = 0;
			wstring outFilePath = L"outPath\\";
			outFilePath += GetPackageName(wstring(this->wFileName));
			outFilePath += L"\\";
			outFilePath += it.info.FileName.c_str();
			this->AddPath(outFilePath.c_str());
			HANDLE hFile = CreateFileW(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (it.info.EncryptedFlag & 7)
			{
				PBYTE OutBuffer = nullptr, WriteBuffer = nullptr;
				OutBuffer = (PBYTE)CMem::Alloc(it.info.ArchiveSize.LowPart);
				WriteBuffer = (PBYTE)CMem::Alloc(it.info.OriginalSize.LowPart);
				FakeWorker.Seek(it.segm.segm[0].Offset.LowPart, FILE_BEGIN);
				FakeWorker.Read(OutBuffer, it.info.ArchiveSize.LowPart);
				ULONG OriSize = it.info.OriginalSize.LowPart, ArcSize = it.info.ArchiveSize.LowPart;
				uncompress(WriteBuffer, &OriSize, OutBuffer, ArcSize);

				DecryptWorker(0, WriteBuffer, it.info.ArchiveSize.LowPart, it.adlr.Hash);
				WriteFile(hFile, WriteBuffer, it.info.OriginalSize.LowPart, &os, NULL);

				CMem::Free(WriteBuffer);
				CMem::Free(OutBuffer);
			}
			else
			{
				PBYTE WriteBuffer = nullptr;
				WriteBuffer = (PBYTE)CMem::Alloc(it.info.OriginalSize.LowPart);
				FakeWorker.Seek(it.segm.segm[0].Offset.LowPart, FILE_BEGIN);
				FakeWorker.Read(WriteBuffer, it.info.OriginalSize.LowPart);

				DecryptWorker(0, WriteBuffer, it.info.ArchiveSize.LowPart, it.adlr.Hash);
				WriteFile(hFile, WriteBuffer, it.info.OriginalSize.LowPart, &os, NULL);

				CMem::Free(WriteBuffer);
			}
			CloseHandle(hFile);
		}
		FakeWorker.Release();
	}

	for (auto iter : Failed)
	{
		wstring OutString = L"Failed to open : " + iter;
		OutString += L"\n";
		OutputString(OutString.c_str());
	}

	GlobalData::GetGlobalData()->isRunning = FALSE;
	this->Reset();
	return S_OK;
}
