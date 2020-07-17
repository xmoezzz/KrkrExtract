#include "XP3Parser.h"
#include "GlobalInit.h"
#include "zlib.h"

static WCHAR* ProtectionInfo = L"$$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ This is a protected archive. $$$ 著作者はこのアーカイブが正規の利用方法以外の方法で展開されることを望んでいません。 $$$ Warning! Extracting this archive may infringe on author's rights. 警告 このアーカイブを展開することにより、あなたは著作者の権利を侵害するおそれがあります。.txt";

NTSTATUS NTAPI FindEmbededXp3OffsetSlow(LPCWSTR FileName, PLARGE_INTEGER Offset)
{
	HRESULT         Status;
	BYTE            Buffer[0x10000], *Xp3Signature;
	LARGE_INTEGER   BytesRead;
	NtFileDisk      file;

	BytesRead.QuadPart = 0;

	BYTE             Mark[] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };
	KRKR2_XP3_HEADER XP3Header = { Mark, (ULONG64)0 };

	LOOP_ONCE
	{
		Status = file.Open(FileName);
		if (NT_FAILED(Status))
			break;

		Status = file.Read(Buffer, sizeof(IMAGE_DOS_HEADER));
		if (NT_FAILED(Status))
			break;

		if (((PIMAGE_DOS_HEADER)Buffer)->e_magic != IMAGE_DOS_SIGNATURE)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		Status = file.Seek(0x10, FILE_BEGIN);
		if (NT_FAILED(Status))
			break;

		Status = STATUS_SUCCESS;
		for (ULONG64 FileSize = file.GetSize64(); FileSize > 0; FileSize -= sizeof(Buffer))
		{
			Status = file.Read(Buffer, sizeof(Buffer));
			if (NT_FAILED(Status))
				goto EndOfParse;

			if (BytesRead.QuadPart < 0x10)
				goto EndOfParse;

			Xp3Signature = Buffer;
			for (ULONG_PTR Count = sizeof(Buffer) / 0x10; Count; Xp3Signature += 0x10, --Count)
			{
				if (sizeof(XP3Header.Magic) != RtlCompareMemory(Xp3Signature, XP3Header.Magic, sizeof(XP3Header.Magic)))
				{
					Offset->QuadPart = file.GetCurrentPos64() - sizeof(Buffer) + (Xp3Signature - Buffer);
					Status = STATUS_SUCCESS;
					goto EndOfParse;
				}
			}
		}
	}

	EndOfParse:
	file.Close();
	return Status;
}

#define iPosAdd(x) iPos+= (ULONG)x
#define PtrInc(x) PtrOffset += (ULONG)x

BOOL WINAPI DetectCompressedChunk(PBYTE pDecompress, ULONG Size)
{
	GlobalData*  Handle;
	ULONG        PtrOffset;
	BOOL         FirstTraveled, NextTraveled, IsFirstChunk;
	BOOL         NotAdd;
	WCHAR        ZeroEnd;

	if (GlobalData::GetGlobalData()->DebugOn)
		PrintConsoleW(L"Detacting Index Info...\n");

	Handle = GlobalData::GetGlobalData();

	FirstTraveled = FALSE;
	NextTraveled  = FALSE;
	IsFirstChunk  = FALSE;
	PtrOffset     = 0;
	 
	while (PtrOffset < Size)
	{
		NotAdd = FALSE;
		XP3Index item;

		if (Handle->DebugOn)
			PrintConsoleW(L"[Start]Pos : %08x\n", PtrOffset);

		if (*(PDWORD)(pDecompress + PtrOffset) == CHUNK_MAGIC_YUZU       ||
			*(PDWORD)(pDecompress + PtrOffset) == CHUNK_MAGIC_KRKRZ_M2   ||
			*(PDWORD)(pDecompress + PtrOffset) == CHUNK_MAGIC_KRKRZ_NEKO ||
			*(PDWORD)(pDecompress + PtrOffset) == CHUNK_MAGIC_KRKRZ_FENG ||
			*(PDWORD)(pDecompress + PtrOffset) == Handle->M2ChunkMagic)
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
			if (*(PDWORD)(pDecompress + CurOffset) != TAG4('File'))
			{
				if (Handle->DebugOn)
					PrintConsoleW(L"[info]No File Chunk Magic(at bad chunk detaction)\n");

				MessageBoxW(NULL, L"Internal Error", L"KrkrExtract", MB_OK);

				return TRUE;
			}

			CurOffset += 4;
			RtlCopyMemory(&IndexLength, pDecompress + CurOffset, 8);
			CurOffset += 8;
			ULONG EndOffset = CurOffset + IndexLength.LowPart;

			while (CurOffset < EndOffset)
			{
				if (*(PDWORD)(pDecompress + CurOffset) == TAG4('info'))
				{
					CurOffset += 4;
					RtlCopyMemory(&IndexLength, pDecompress + CurOffset, 8);
					CurOffset += 8;
					CurOffset += 4;
					CurOffset += 8 * 2;
					CurOffset += 2;

					if (!StrNCompareW((LPCWSTR)(pDecompress + CurOffset), ProtectionInfo, StrLengthW(ProtectionInfo)))
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
			item.yuzu.Hash = *(PDWORD)(pDecompress + PtrOffset);
			PtrOffset += 4;
			item.yuzu.Len = *(PUSHORT)(pDecompress + PtrOffset);
			PtrOffset += 2;
			wstring M2FileName((WCHAR*)(pDecompress + PtrOffset), item.yuzu.Len);
			item.yuzu.Name = M2FileName;

			ZeroEnd = *(WCHAR*)(pDecompress + PtrOffset + item.yuzu.Len * 2);

			if (!ZeroEnd)
				PtrOffset += (item.yuzu.Len + 1) * 2;
			else
				PtrOffset += (item.yuzu.Len) * 2;

			Handle->IsM2Format = TRUE;

			if (!StrCompareW(item.yuzu.Name.c_str(), ProtectionInfo))
				NotAdd = TRUE;
		}

		if (*(PDWORD)(pDecompress + PtrOffset) != CHUNK_MAGIC_FILE)
		{
			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleW(L"[info]No File Chunk Magic\n");

			MessageBoxW(NULL, L"Internal Error", L"KrkrExtract", MB_OK);
			GlobalData::GetGlobalData()->isRunning = FALSE;
			return FALSE;
		}

		//File Maigc
		PtrInc(4);
		ULONG64 FileChunkSize64 = 0;
		CopyMemory(&FileChunkSize64, (PtrOffset + pDecompress), 8);
		PtrInc(8);

		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"FileChunkSize : %08x\n", (ULONG32)FileChunkSize64);

		ULONG SavePtrOffset = PtrOffset;
		ULONG32 FileChunkSize32 = (ULONG32)FileChunkSize64;
		ULONG iPos = 0;

		while (iPos < FileChunkSize32)
		{
			if (*(PDWORD)(PtrOffset + pDecompress) == CHUNK_MAGIC_INFO)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 InfoChunkSize = 0;
				RtlCopyMemory(&InfoChunkSize, (PtrOffset + pDecompress), sizeof(ULONG64));

				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"Info Chunk : %08x\n", InfoChunkSize);

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
					|| Handle->RawExtract)
				{
					Handle->RawExtract = TRUE;
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

					WCHAR FakeNameStr[60];
					RtlZeroMemory(FakeNameStr, countof(FakeNameStr) * sizeof(WCHAR));
					FormatStringW(FakeNameStr, L"%08X-%08X-%08X", FakeNameSegm1, FakeNameSegm2, FakeNameSegm3);
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

					if (!StrCompareW(ProtectionInfo, WideFileName.c_str()))
					{
						NotAdd = TRUE;
					}
				}
			}
			else if (*(PDWORD)(PtrOffset + pDecompress) == CHUNK_MAGIC_SEGM)
			{
				PtrInc(4);
				iPosAdd(4);
				item.segm.ChunkSize.QuadPart = *(PULONG64)(PtrOffset + pDecompress);

				if (Handle->DebugOn)
					PrintConsoleW(L"Segm Chunk : %08x\n", item.segm.ChunkSize.LowPart);

				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.segm.segm[0].bZlib), (PtrOffset + pDecompress), sizeof(BOOL));
				CopyMemory(&(item.segm.segm[0].Offset), (PtrOffset + pDecompress + sizeof(BOOL)), sizeof(ULONG64));
				CopyMemory(&(item.segm.segm[0].OriginalSize), (PtrOffset + pDecompress + sizeof(BOOL) + sizeof(ULONG64)), sizeof(ULONG64));
				CopyMemory(&(item.segm.segm[0].ArchiveSize), (PtrOffset + pDecompress + sizeof(BOOL) + sizeof(ULONG64) * 2), sizeof(ULONG64));
				PtrInc(item.segm.ChunkSize.LowPart);
				iPosAdd(item.segm.ChunkSize.LowPart);
			}
			else if (*(PDWORD)(PtrOffset + pDecompress) == CHUNK_MAGIC_ADLR)
			{
				PtrInc(4);
				iPosAdd(4);

				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);

				if (Handle->DebugOn)
					PrintConsoleW(L"Adlr Chunk : %08x\n", TempChunkSize);

				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.adlr.Hash), (PtrOffset + pDecompress), 4);
				PtrInc(4);
				iPosAdd(4);
			}
			else if (*(PDWORD)(PtrOffset + pDecompress) == CHUNK_MAGIC_TIME)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);

				if (Handle->DebugOn)
					PrintConsoleW(L"Time Chunk : %08x\n", TempChunkSize);

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


				if (Handle->DebugOn)
					PrintConsoleW(L"Found Unknown Chunk at %08x\n", PtrOffset);

				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"Unknown Chunk [%c%c%c%c] : %08x\n", UnknownMagic[0], UnknownMagic[1],
					UnknownMagic[2], UnknownMagic[3], TempChunkSize);

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

	switch (Handle->M2ChunkMagic)
	{
	case 0:
		return TRUE;

	default:
		return NextTraveled;
	}
}


BOOL WINAPI InitIndexFileFirst(PBYTE pDecompress, ULONG Size)
{
	GlobalData*   Handle;
	ULONG         PtrOffset;
	BOOL          NotAdd;

	Handle = GlobalData::GetGlobalData();

	if (Handle->DebugOn)
		PrintConsoleW(L"Init Index Info...\n");

	PtrOffset = 0;
	while (PtrOffset < Size)
	{
		NotAdd = FALSE;

		XP3Index item;

		if (Handle->DebugOn)
			PrintConsoleW(L"[Start]Pos : %08x\n", PtrOffset);

		if (*(PDWORD)(pDecompress + PtrOffset) == CHUNK_MAGIC_YUZU       ||
			*(PDWORD)(pDecompress + PtrOffset) == CHUNK_MAGIC_KRKRZ_M2   ||
			*(PDWORD)(pDecompress + PtrOffset) == CHUNK_MAGIC_KRKRZ_NEKO ||
			*(PDWORD)(pDecompress + PtrOffset) == CHUNK_MAGIC_KRKRZ_FENG ||
			*(PDWORD)(pDecompress + PtrOffset) == Handle->M2ChunkMagic)
		{
			PtrOffset += 4;
			ULARGE_INTEGER IndexLength;
			RtlCopyMemory(&IndexLength, pDecompress + PtrOffset, 8);

			//look forward the related file chunk
			ULONG CurOffset = PtrOffset + IndexLength.LowPart + 8;
			if (*(PDWORD)(pDecompress + CurOffset) != CHUNK_MAGIC_FILE)
			{
				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"[info]No File Chunk Magic(at bad chunk detaction)\n");

				MessageBoxW(NULL, L"Internal Error", L"KrkrExtract", MB_OK);
				return FALSE;
			}

			CurOffset += 4;
			RtlCopyMemory(&IndexLength, pDecompress + CurOffset, 8);
			CurOffset += 8;
			ULONG EndOffset = CurOffset + IndexLength.LowPart;

			while (CurOffset < EndOffset)
			{
				if (*(PDWORD)(pDecompress + CurOffset) == CHUNK_MAGIC_FILE)
				{
					CurOffset += 4;
					RtlCopyMemory(&IndexLength, pDecompress + CurOffset, 8);
					CurOffset += 8;
					CurOffset += 4;
					CurOffset += 8 * 2;
					CurOffset += 2;

					if (!StrCompareW((LPCWSTR)(pDecompress + CurOffset), ProtectionInfo))
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

			Handle->IsM2Format = TRUE;

			if (!StrCompareW(item.yuzu.Name.c_str(), ProtectionInfo))
			{
				NotAdd = TRUE;
			}
		}
		else
		{
			item.isM2Format = FALSE;
			Handle->IsM2Format = FALSE;
		}

		if (item.isM2Format)
		{
			if (GlobalData::GetGlobalData()->DebugOn)
			{
				PrintConsoleW(L"Found M2-SubChunk\n");
				PrintConsoleW(L"[Middle]Pos : %08x\n", PtrOffset);
			}
		}
		else
		{
			if (GlobalData::GetGlobalData()->DebugOn)
			{
				PrintConsoleW(L"Normal\n");
				PrintConsoleW(L"[Middle]Pos : %08x\n", PtrOffset);
			}
		}

		if (*(PDWORD)(pDecompress + PtrOffset) != CHUNK_MAGIC_FILE)
		{
			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleW(L"[info]No File Chunk Magic\n");

			MessageBoxW(NULL, L"Internal Error", L"KrkrExtract", MB_OK);
			GlobalData::GetGlobalData()->isRunning = FALSE;
			return FALSE;
		}

		//File Maigc
		PtrInc(4);
		ULONG64 FileChunkSize64 = 0;
		CopyMemory(&FileChunkSize64, (PtrOffset + pDecompress), 8);
		PtrInc(8);

		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"FileChunkSize : %08x\n", (ULONG32)FileChunkSize64);

		ULONG SavePtrOffset = PtrOffset;
		ULONG32 FileChunkSize32 = (ULONG32)FileChunkSize64;
		ULONG iPos = 0;

		while (iPos < FileChunkSize32)
		{
			if (*(PDWORD)(PtrOffset + pDecompress) == CHUNK_MAGIC_INFO)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 InfoChunkSize = 0;
				RtlCopyMemory(&InfoChunkSize, (PtrOffset + pDecompress), sizeof(ULONG64));

				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"Info Chunk : %08x\n", InfoChunkSize);

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
					|| Handle->RawExtract)
				{
					Handle->RawExtract = TRUE;
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
					FormatStringW(FakeNameStr, L"%08X-%08X-%08X", FakeNameSegm1, FakeNameSegm2, FakeNameSegm3);
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

					if (!StrCompareW(ProtectionInfo, WideFileName.c_str()))
					{
						NotAdd = TRUE;
					}
				}
			}
			else if (*(PDWORD)(PtrOffset + pDecompress) == CHUNK_MAGIC_SEGM)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);
				item.segm.ChunkSize.QuadPart = TempChunkSize;

				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"Segm Chunk : %08x\n", TempChunkSize);

				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.segm.segm[0].bZlib), (PtrOffset + pDecompress), sizeof(BOOL));
				CopyMemory(&(item.segm.segm[0].Offset), (PtrOffset + pDecompress + sizeof(BOOL)), sizeof(ULONG64));
				CopyMemory(&(item.segm.segm[0].OriginalSize), (PtrOffset + pDecompress + sizeof(BOOL) + sizeof(ULONG64)), sizeof(ULONG64));
				CopyMemory(&(item.segm.segm[0].ArchiveSize), (PtrOffset + pDecompress + sizeof(BOOL) + sizeof(ULONG64) * 2), sizeof(ULONG64));
				PtrInc(TempChunkSize);
				iPosAdd(TempChunkSize);
			}
			else if (*(PDWORD)(PtrOffset + pDecompress) == CHUNK_MAGIC_ADLR)
			{
				PtrInc(4);
				iPosAdd(4);

				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);

				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"Adlr Chunk : %08x\n", TempChunkSize);

				PtrInc(8);
				iPosAdd(8);
				CopyMemory(&(item.adlr.Hash), (PtrOffset + pDecompress), 4);
				PtrInc(4);
				iPosAdd(4);
			}
			else if (*(PDWORD)(PtrOffset + pDecompress) == CHUNK_MAGIC_TIME)
			{
				PtrInc(4);
				iPosAdd(4);
				ULONG64 TempChunkSize = 0;
				CopyMemory(&TempChunkSize, (PtrOffset + pDecompress), 8);

				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"Time Chunk : %08x\n", TempChunkSize);

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

				if (GlobalData::GetGlobalData()->DebugOn)
				{
					PrintConsoleW(L"Found Unknown Chunk at %08x\n", PtrOffset);
					PrintConsoleW(L"Unknown Chunk [%c%c%c%c] : %08x\n", UnknownMagic[0], UnknownMagic[1],
						UnknownMagic[2], UnknownMagic[3], TempChunkSize);
				}

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
				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"[Found File:]%s\n", item.yuzu.Name.c_str());
			}
			else
			{
				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"[Found File:]%s\n", item.info.FileName.c_str());
			}
			Handle->ItemVector.push_back(item);
		}

		if (GlobalData::GetGlobalData()->DebugOn)
		{
			PrintConsoleW(L"In Index : %s\n", item.info.FileName.c_str());
			PrintConsoleW(L"[End]Pos : %08x\n", PtrOffset);
		}
	}
	//End loop

	Handle->SetCount(Handle->ItemVector.size());

	if (GlobalData::GetGlobalData()->DebugOn)
		PrintConsoleW(L"File Count : [%d]\n", Handle->ItemVector.size());

	return TRUE;
}


BOOL WINAPI InitIndexFile_SenrenBanka(PBYTE pDecompress, ULONG Size, NtFileDisk& File)
{
	KRKRZ_COMPRESSED_INDEX  CompressedIndex;
	PBYTE                   CompressedBuffer;
	PBYTE                   IndexBuffer;
	ULONG                   DecompSize, iPos;
	DWORD                   Hash;
	XP3Index                Item;
	ULARGE_INTEGER          ChunkSize;
	USHORT                  NameLength;
	GlobalData*             Handle;

	Handle = GlobalData::GetGlobalData();

	RtlCopyMemory(&CompressedIndex, pDecompress, sizeof(KRKRZ_COMPRESSED_INDEX));
	File.Seek(CompressedIndex.Offset.LowPart, FILE_BEGIN);

	iPos = 0;
	DecompSize       = CompressedIndex.DecompressedSize;
	IndexBuffer      = (PBYTE)AllocateMemoryP(CompressedIndex.DecompressedSize);
	CompressedBuffer = (PBYTE)AllocateMemoryP(CompressedIndex.CompressedSize);

	if (!IndexBuffer || !CompressedBuffer)
	{
		MessageBoxW(NULL, L"Insufficient memory", L"KrkrExtract", MB_OK);

		if (IndexBuffer)      FreeMemoryP(IndexBuffer);
		if (CompressedBuffer) FreeMemoryP(CompressedBuffer);
		return FALSE;
	}

	File.Read(CompressedBuffer, CompressedIndex.CompressedSize);


	if ((DecompSize = uncompress((PBYTE)IndexBuffer, (PULONG)&DecompSize,
		(PBYTE)CompressedBuffer, CompressedIndex.CompressedSize)) != Z_OK)
	{
		MessageBoxW(NULL, L"Failed to decompress special chunk", L"KrkrExtract", MB_OK);
		return FALSE;
	}

	Handle->IsM2Format = TRUE;

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
		Item.yuzu.Len = NameLength;
		Item.yuzu.Name = FileName;

		Item.isM2Format = TRUE;

		Handle->ItemVector.push_back(Item);
	}

	Handle->CountFile = Handle->ItemVector.size();
	FreeMemoryP(IndexBuffer);
	FreeMemoryP(CompressedBuffer);

	return TRUE;
}

//M2
ULONG WINAPI FindChunkMagicFirst(PBYTE pDecompress, ULONG Size)
{
	GlobalData*      Handle;
	ULONG            PtrOffset;
	ULONG64          FileChunkSize;
	ULARGE_INTEGER   ChunkSize;
	ULONG            PackType;
	
	PackType = PackInfo::NormalPack;
	Handle = GlobalData::GetGlobalData();

	Handle->M2ChunkMagic = 0;

	PtrOffset = 0;
	while (PtrOffset < Size)
	{
		switch (*(PDWORD)(PtrOffset + pDecompress))
		{
		//normal chunk
		case CHUNK_MAGIC_FILE:
		case CHUNK_MAGIC_INFO:
		case CHUNK_MAGIC_SEGM:
		case CHUNK_MAGIC_ADLR:
		case CHUNK_MAGIC_TIME:
			PtrInc(4);
			ChunkSize.QuadPart = *(PULONG64)(pDecompress + PtrOffset);
			PtrInc(ChunkSize.LowPart);
			PtrInc(8);
		break;

		default:
			Handle->M2ChunkMagic = *(PDWORD)(pDecompress + PtrOffset);
			return PackInfo::KrkrZ;
		break;
		}
	}
	return PackType;
}

BOOL WINAPI InitIndex_NekoVol0(PBYTE pDecompress, ULONG Size)
{
	GlobalData*  Handle;
	ULONG        iPos, SavePos;
	XP3Index     item;

	Handle = GlobalData::GetGlobalData();

	Handle->IsM2Format = TRUE;
	Handle->CountFile  = 0;

	iPos = 0;

	LOOP_FOREVER
	{
		if (*(PDWORD)(pDecompress + iPos) == Handle->M2ChunkMagic)
		{
			iPos += 4;
			ULONG64 ChunkSize = 0;
			RtlCopyMemory(&ChunkSize, (pDecompress + iPos), 8);
			iPos += 8;
			SavePos = iPos;
			
			item.yuzu.ChunkSize.QuadPart = ChunkSize;
			ULONG HashValue = 0;
			RtlCopyMemory(&HashValue, (pDecompress + iPos), 4);
			iPos += 4;
			item.yuzu.Hash = HashValue;
			USHORT FileNameLen = 0;
			RtlCopyMemory(&FileNameLen, (pDecompress + iPos), 2);
			iPos += 2;
			wstring FileName((WCHAR*)(pDecompress + iPos), FileNameLen);
			item.yuzu.Name = FileName;
			iPos = SavePos;
			iPos += (ULONG)ChunkSize;
			item.isM2Format = TRUE;
			Handle->ItemVector.push_back(item);
			Handle->CountFile++;
		}
		else
		{
			break;
		}
	}
	return TRUE;
}

//Return TRUE -> 老版本的封包

#if 0
BOOL WINAPI IsCompatXP3(PBYTE Data, ULONG Size, DWORD* pMagic)
{
	DWORD          ChunkMagic, NextChunkMagic, ThirdChunkMagic;
	ULONG          iPos;
	ULARGE_INTEGER ChunkSize;
	GlobalData*    Handle;

	Handle = GlobalData::GetGlobalData();

	Handle->M2ChunkMagic = 0;

	if (pMagic)   *pMagic = 0;
	if (Size < 4) return TRUE;


	iPos = 4;
	ChunkMagic = *(PDWORD)Data;
		
	RtlCopyMemory(&ChunkSize, Data + iPos, 8);
	iPos += 8;
	iPos += ChunkSize.LowPart;
	NextChunkMagic = *(PDWORD)(Data + iPos);
	iPos += 4;
	RtlCopyMemory(&ChunkSize, Data + iPos, 8);
	iPos += 8;
	iPos += ChunkSize.LowPart;
	ThirdChunkMagic = *(PDWORD)(Data + iPos);

	if (ChunkMagic == NextChunkMagic && NextChunkMagic == ThirdChunkMagic)
	{
		if (ChunkMagic == TAG4('File'))
			return TRUE;
		else
		{
			Handle->M2ChunkMagic = ChunkMagic;
			if (pMagic)  *pMagic = ChunkMagic;
			return FALSE;
		}
	}
	else
	{
		if (ChunkMagic != TAG4('File'))
		{
			Handle->M2ChunkMagic = ChunkMagic;
			if (pMagic)  *pMagic = ChunkMagic;
		}
		else if (NextChunkMagic != TAG4('File'))
		{
			Handle->M2ChunkMagic = NextChunkMagic;
			if (pMagic)  *pMagic = NextChunkMagic;
		}
		else if (ThirdChunkMagic != TAG4('File'))
		{
			Handle->M2ChunkMagic = ThirdChunkMagic;
			if (pMagic)  *pMagic = ThirdChunkMagic;
		}
		return TRUE;
	}
}
#else

BOOL WINAPI IsCompatXP3(PBYTE Data, ULONG Size, DWORD* pMagic)
{
	DWORD          ChunkMagic, NextChunkMagic, ThirdChunkMagic;
	ULONG          iPos;
	BOOL           FirstVisited, IsNekoVol0;
	ULARGE_INTEGER ChunkSize;
	vector<BOOL>   VisitArray;
	GlobalData*    Handle;

	Handle = GlobalData::GetGlobalData();

	Handle->M2ChunkMagic = 0;
	FirstVisited = FALSE;
	IsNekoVol0   = FALSE;

	if (pMagic)   *pMagic = 0;
	if (Size < 4) return TRUE;

	iPos     = 0;
	while (iPos < Size)
	{
		ChunkMagic = *(PDWORD)Data;
		iPos += 4;
		RtlCopyMemory(&ChunkSize, Data + iPos, 8);
		iPos += 8;
		iPos += ChunkSize.LowPart;

		if (ChunkMagic != TAG4('File'))
		{
			if (!FirstVisited)
			{
				Handle->M2ChunkMagic = ChunkMagic;
				if (pMagic)  *pMagic = ChunkMagic;
			}
			VisitArray.push_back(TRUE);
		}
		else
		{
			VisitArray.push_back(FALSE);
		}
	}

	if (VisitArray.size() == 0 || VisitArray.size() == 1)
		return TRUE;

	//无视，因为兼容
	if (VisitArray.size() == 2)
	{
		//iPos = 0;
		return TRUE;
	}

	iPos = 1;
	while (iPos < VisitArray.size())
	{
		if (VisitArray[iPos - 1] == TRUE && VisitArray[iPos] == TRUE)
		{
			IsNekoVol0 = TRUE;
			break;
		}
		iPos++;
	}

	if (IsNekoVol0)
		return FALSE;
	else
		return TRUE;
}

#endif

