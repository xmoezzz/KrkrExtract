#include "KrkrDumper.h"
#include "resource.h"
#include "KrkrExtend.h"
#include "zlib.h"
#include "MyLib.h"
#include <WindowsX.h>

/********************************************/


NTSTATUS WINAPI DecompilePsbJson(IStream* PsbStream, LPCWSTR BasePath, LPCWSTR FileName);
unsigned char *getDataFromLz4(const unsigned char *buff, unsigned long &size);
unsigned char *getDataFromMDF(const unsigned char *buff, unsigned long &size);



KrkrDumper::KrkrDumper()
{
	IsIndex = FALSE;
	hThread = INVALID_HANDLE_VALUE;

	RtlZeroMemory(FileName, sizeof(FileName));
	RtlZeroMemory(FileNameIndex, sizeof(FileNameIndex));
}


KrkrDumper::~KrkrDumper()
{
}



VOID NTAPI KrkrDumper::InternalReset()
{
	GlobalData*   Handle;

	Handle = GlobalData::GetGlobalData();
	SetFile(NULL);
	Handle->Reset();
}


/*
exe::TVPCreateIStream.stub
CPU Disasm
Address                 Hex dump                       Command                                                    Comments
0134D830                /. /55                         push ebp
0134D831                |. |8BEC                       mov ebp,esp
0134D833                |. |8B55 0C                    mov edx,dword ptr [ebp+0C]
0134D836                |. |8B4D 08                    mov ecx,dword ptr [ebp+8]
0134D839                |. |E8 E2800000                call 01355920   exe::TVPCreateIStream
0134D83E                |. |5D                         pop ebp
0134D83F                \. |C2 0800                    retn 8

exe::TVPCreateIStream fastcall
01355920                /$  55                         push ebp
01355921                |.  8BEC                       mov ebp,esp
01355923                |.  6A FF                      push -1
01355925                |.  68 BB405C01                push 015C40BB                                              ; Entry point
0135592A                |.  64:A1 00000000             mov eax,dword ptr fs:[0]
01355930                |.  50                         push eax
01355931                |.  83EC 0C                    sub esp,0C
01355934                |.  53                         push ebx
01355935                |.  56                         push esi
01355936                |.  57                         push edi
01355937                |.  A1 B0486901                mov eax,dword ptr [16948B0]
0135593C                |.  33C5                       xor eax,ebp
0135593E                |.  50                         push eax
0135593F                |.  8D45 F4                    lea eax,[ebp-0C]
01355942                |.  64:A3 00000000             mov dword ptr fs:[0],eax
01355948                |.  8965 F0                    mov dword ptr [ebp-10],esp
0135594B                |.  C745 EC 00000000           mov dword ptr [ebp-14],0
01355952                |.  C745 FC 00000000           mov dword ptr [ebp-4],0
01355959                |.  E8 8296FEFF                call 0133EFE0   exe::TVPCreateStream(fastcall)


0135595E                |.  8BF0                       mov esi,eax
01355960                |.  8975 EC                    mov dword ptr [ebp-14],esi
01355963                |.  C745 FC FFFFFFFF           mov dword ptr [ebp-4],-1
0135596A                |.  6A 0C                      push 0C
0135596C                |.  E8 77882000                call 0155E1E8   call operator.new
01355971                |.  83C4 04                    add esp,4
*/

//只能在krkrz模式下工作，因为krkrz是使用msvc编译的
//bcb将会被这个function分割成两段和一个call，比较脑残
//而且fastcall的顺序是eax, edx
PVOID GetTVPCreateStreamCall()
{
	PVOID CallIStreamStub, CallIStream, CallTVPCreateStreamCall;
	ULONG OpSize, OpOffset;
	WORD  WordOpcode;

	static char funcname[] = "IStream * ::TVPCreateIStream(const ttstr &,tjs_uint32)";

	LOOP_ONCE
	{
		CallTVPCreateStreamCall = NULL;

		CallIStreamStub = TVPGetImportFuncPtr(funcname);
		if (!CallIStreamStub)
			break;

		CallIStream = NULL;
		OpOffset    = 0;

		LOOP_FOREVER
		{
			if (((PBYTE)CallIStreamStub + OpOffset)[0] == 0xCC)
				break;

			WordOpcode = *(PWORD)((ULONG_PTR)CallIStreamStub + OpOffset);
			//mov edx,dword ptr [ebp+0xC]
			if (WordOpcode == 0x558B)
			{
				OpOffset += 2;
				if (((PBYTE)CallIStreamStub + OpOffset)[0] == 0xC)
				{
					OpOffset++;
					WordOpcode = *(PWORD)((ULONG_PTR)CallIStreamStub + OpOffset);
					//mov edx,dword ptr [ebp+0x8]
					if (WordOpcode == 0x4D8B)
					{
						OpOffset += 2;
						if (((PBYTE)CallIStreamStub + OpOffset)[0] == 0x8)
						{
							OpOffset++;
							if (((PBYTE)CallIStreamStub + OpOffset)[0] == CALL)
							{
								CallIStream = (PVOID)GetCallDestination(((ULONG_PTR)CallIStreamStub + OpOffset));
								OpOffset += 5;
								break;
							}
						}
					}
				}
			}
			//the next opcode
			OpSize = GetOpCodeSize32(((PBYTE)CallIStreamStub + OpOffset));
			OpOffset += OpSize;
		}

		if (!CallIStream)
			break;

		OpOffset = 0;
		LOOP_FOREVER
		{
			if (((PBYTE)CallIStream + OpOffset)[0] == 0xC3)
				break;

			//find the first call
			if (((PBYTE)CallIStream + OpOffset)[0] == CALL)
			{
				CallTVPCreateStreamCall = (PVOID)GetCallDestination(((ULONG_PTR)CallIStream + OpOffset));
				OpOffset += 5;
				break;
			}

			//the next opcode
			OpSize = GetOpCodeSize32(((PBYTE)CallIStream + OpOffset));
			OpOffset += OpSize;
		}

		LOOP_FOREVER
		{
			if (((PBYTE)CallIStream + OpOffset)[0] == 0xC3)
				break;

			if (((PBYTE)CallIStream + OpOffset)[0] == CALL)
			{
				//push 0xC
				//call HostAlloc
				//add esp, 0x4
				if (((PBYTE)CallIStream + OpOffset - 2)[0] == 0x6A &&
					((PBYTE)CallIStream + OpOffset - 2)[1] == 0x0C)
				{
					GlobalData::GetGlobalData()->StubHostAlloc = (FuncHostAlloc)GetCallDestination(((ULONG_PTR)CallIStream + OpOffset));
					OpOffset += 5;
				}
				break;
			}

			//the next opcode
			OpSize = GetOpCodeSize32(((PBYTE)CallIStream + OpOffset));
			OpOffset += OpSize;
		}

		LOOP_FOREVER
		{
			if (((PBYTE)CallIStream + OpOffset)[0] == 0xC3)
				break;

			//mov eax, mem.offset
			if (((PBYTE)CallIStream + OpOffset)[0] == 0xC7 &&
				((PBYTE)CallIStream + OpOffset)[1] == 0x00)
			{
				OpOffset += 2;
				GlobalData::GetGlobalData()->IStreamAdapterVtable = *(PULONG_PTR)((PBYTE)CallIStream + OpOffset);
				OpOffset += 4;
				break;
			}

			//the next opcode
			OpSize = GetOpCodeSize32(((PBYTE)CallIStream + OpOffset));
			OpOffset += OpSize;
		}
	}


	//Find virtual table offset
	//IStreamAdapter

	if (GlobalData::GetGlobalData()->StubHostAlloc && GlobalData::GetGlobalData()->IStreamAdapterVtable)
	{
		//PrintConsoleW(L"Analyze ok...\n");
		return CallTVPCreateStreamCall;
	}
	else
	{
		return NULL;
	}
}

tTJSBinaryStream* FASTCALL CallTVPCreateStream(const ttstr& FilePath)
{
	tTJSBinaryStream* Stream;
	GlobalData*       Handle;

	Handle = GlobalData::GetGlobalData();

	if (Handle->StubTVPCreateStream == NULL)
		Handle->StubTVPCreateStream = (FuncCreateStream)GetTVPCreateStreamCall();

	Stream = NULL;

	if (Handle->StubTVPCreateStream == NULL)
		return Stream;

	return Handle->StubTVPCreateStream(FilePath, TJS_BS_READ);
}


IStream* FASTCALL ConvertBStreamToIStream(tTJSBinaryStream* BStream)
{
	IStream*  Stream;
	PVOID     CallHostAlloc;
	ULONG_PTR IStreamAdapterVTableOffset;

	CallHostAlloc              = GlobalData::GetGlobalData()->StubHostAlloc;
	IStreamAdapterVTableOffset = GlobalData::GetGlobalData()->IStreamAdapterVtable;
	Stream = NULL;

	INLINE_ASM
	{
		push 0xC;
		call CallHostAlloc;
		add  esp, 0x4;
		test eax, eax;
		jz   NO_CREATE_STREAM;
		mov  esi, IStreamAdapterVTableOffset;
		mov  dword ptr [eax], esi; //Vtable 
		mov  esi, BStream;
		mov  dword ptr [eax + 4], esi; //StreamHolder
		mov  dword ptr [eax + 8], 1;   //ReferCount
		mov  Stream, eax;

		NO_CREATE_STREAM:
	}

	return Stream;
}

NTSTATUS WINAPI KrkrDumper::ParseXP3File(PWCHAR lpFileName)
{
	NTSTATUS   Status;
	NtFileDisk file;
	
	LOOP_ONCE
	{
		Status = file.Open(lpFileName);
		if (NT_FAILED(Status))
			break;

		Status = ProcessXP3Archive(lpFileName, file);
	}
	file.Close();
	return Status;
}

NTSTATUS WINAPI KrkrDumper::ParseXP3iFile(PWCHAR lpFileName)
{
	NTSTATUS    Status;
	NtFileDisk  File;
	PBYTE       Buffer;
	ULONG       Size, iPos, SavePos;
	XP3Index    item;
	GlobalData* Handle;

	Handle = GlobalData::GetGlobalData();
	Handle->IsM2Format = TRUE;

	Status = File.Open(lpFileName);
	if (NT_FAILED(Status))
	{
		PrintConsoleW(L"File.Open failed with status : %08x\n", Status);
		return Status;
	}

	Size = File.GetSize32();
	Buffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
	if (!Buffer)
	{
		PrintConsoleW(L"Memory allocation failed with status : %08x\n", Status);
		File.Close();
		return STATUS_NO_MEMORY;
	}
	File.Read(Buffer, Size);

	iPos = 0;
	while (iPos < Size)
	{
		iPos += 4;
		ULONG64 ChunkSize = 0;
		RtlCopyMemory(&ChunkSize, (Buffer + iPos), 8);
		iPos += 8;
		SavePos = iPos;
		item.yuzu.ChunkSize.QuadPart = ChunkSize;
		ULONG HashValue = 0;
		RtlCopyMemory(&HashValue, (Buffer + iPos), 4);
		iPos += 4;
		item.yuzu.Hash = HashValue;
		USHORT FileNameLen = 0;
		RtlCopyMemory(&FileNameLen, (Buffer + iPos), 2);
		iPos += 2;
		wstring FileName((PCWSTR)(Buffer + iPos), FileNameLen);
		item.yuzu.Name = FileName;
		iPos = SavePos;
		iPos += (ULONG)ChunkSize;
		item.isM2Format = TRUE;
		
		if (!(FileName.length() > 3 && FileName[0] == L'$' && FileName[1] == L'$' && FileName[2] == L'$'))
		{
			Handle->ItemVector.push_back(item);
			Handle->CountFile++;
		}
	}

	HeapFree(GetProcessHeap(), 0, Buffer);
	File.Close();
	return STATUS_SUCCESS;
}

VOID NTAPI KrkrDumper::AddPath(LPWSTR FileName)
{
	ULONG iPos = 0;

	WCHAR NewPath[MAX_PATH];

	while (FileName[iPos] != NULL)
	{
		if (FileName[iPos] == L'/' || FileName[iPos] == L'\\')
		{
			RtlZeroMemory(NewPath, countof(NewPath) * sizeof(WCHAR));
			RtlCopyMemory(NewPath, FileName, iPos * sizeof(WCHAR));
			lstrcatW(NewPath, L"\\");
			
			if(!PathIsDirectoryW(NewPath))
				CreateDirectoryW(NewPath, NULL);

			FileName[iPos] = L'\\';
		}
		iPos++;
	}
}

VOID NTAPI KrkrDumper::SetFile(LPCWSTR lpFile)
{
	RtlZeroMemory(FileName, countof(FileName) * sizeof(WCHAR));
	if (lpFile) lstrcpyW(FileName, lpFile);
}

#include "json\json.h"
#include "PbdDecode.h"

NTSTATUS NTAPI KrkrDumper::ProcessXP3Archive(LPCWSTR lpFileName, NtFileDisk& file)
{
	NTSTATUS                Status;
	GlobalData*             Handle;
	ULONG                   Count;
	KRKR2_XP3_HEADER        XP3Header;
	KRKR2_XP3_DATA_HEADER   DataHeader;
	LARGE_INTEGER           BeginOffset, Offset;
	CHAR                    M2ChunkInfo[8];


	Handle = GlobalData::GetGlobalData();


	Count = 0;
	BeginOffset.QuadPart = 0;
	Status = file.Read(&XP3Header, sizeof(XP3Header));
	RtlZeroMemory(M2ChunkInfo, sizeof(M2ChunkInfo));

	if (NT_FAILED(Status))
	{
		if (Handle->DebugOn)
			PrintConsoleW(L"Invaild Package\n");

		return Status;
	}

	if ((*(PUSHORT)XP3Header.Magic) == IMAGE_DOS_SIGNATURE)
	{
		Status = FindEmbededXp3OffsetSlow(file, &BeginOffset);

		if (NT_FAILED(Status))
		{
			if (Handle->DebugOn)
				PrintConsoleW(L"No an embeded Package %p\n", Status);

			return Status;
		}

		if (Handle->DebugOn)
			PrintConsoleW(L"Embeded Package %p\n", BeginOffset.LowPart);

		file.Seek(BeginOffset, FILE_BEGIN);
		file.Read(&XP3Header, sizeof(XP3Header));
	}
	else
	{
		BeginOffset.QuadPart = 0;
	}

	Status = STATUS_UNSUCCESSFUL;

	if (RtlCompareMemory(StaticXP3V2Magic, XP3Header.Magic, sizeof(StaticXP3V2Magic)) != sizeof(StaticXP3V2Magic))
	{
		if (Handle->DebugOn)
			PrintConsoleW(L"No a XP3 Package!\n");

		return Status;
	}

	ULONG64 CompresseBufferSize = 0x1000;
	ULONG64 DecompressBufferSize = 0x1000;
	PBYTE pCompress   = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (ULONG)CompresseBufferSize);
	PBYTE pDecompress = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (ULONG)DecompressBufferSize);
	DataHeader.OriginalSize = XP3Header.IndexOffset;

	if (Handle->DebugOn)
		PrintConsoleW(L"Index Offset %08x\n", (ULONG32)XP3Header.IndexOffset.QuadPart);
	
	BOOL Result = FALSE;
	do
	{
		Offset.QuadPart = DataHeader.OriginalSize.QuadPart + BeginOffset.QuadPart;
		file.Seek(Offset, FILE_BEGIN);
		if (NT_FAILED(file.Read(&DataHeader, sizeof(DataHeader))))
		{
			if (Handle->DebugOn)
				PrintConsoleW(L"Couldn't Read Index Header\n");

			return Status;
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
			file.Seek(Offset, FILE_CURRENT);
		}

		file.Read(pCompress, DataHeader.ArchiveSize.LowPart);

		BOOL EncodeMark = DataHeader.bZlib & 7;

		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"Index Encode %x\n", DataHeader.bZlib);
		
		if (EncodeMark == FALSE)
		{
			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleW(L"Index : Raw Data\n");

			if (DataHeader.ArchiveSize.LowPart > DecompressBufferSize)
			{
				DecompressBufferSize = DataHeader.ArchiveSize.LowPart;
				pDecompress = (PBYTE)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pDecompress, (ULONG)DecompressBufferSize);
			}

			RtlCopyMemory(pDecompress, pCompress, DataHeader.ArchiveSize.LowPart);
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
				MessageBoxW(NULL, L"Unknown compression method...", 0, 0);
				return STATUS_UNSUCCESSFUL;
			}
		}
		
		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"Index Size %08x\n", (ULONG32)DataHeader.OriginalSize.LowPart);

		if (IsCompatXP3(pDecompress, DataHeader.OriginalSize.LowPart, &Handle->M2ChunkMagic))
		{
			FindChunkMagicFirst(pDecompress, DataHeader.OriginalSize.LowPart);

			CopyMemory(M2ChunkInfo, &Handle->M2ChunkMagic, 4);
			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleA("Chunk : %s\n", M2ChunkInfo);
			
			//再次检查Chunk，如果SP Chunk只是出现在文件头，并且只是出现一次，那么就是千恋万花的新格式。
			switch (DetectCompressedChunk(pDecompress, DataHeader.OriginalSize.LowPart))
			{
			case TRUE:
				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"Index Normal/M2\n");

				Result = InitIndexFileFirst(pDecompress, DataHeader.OriginalSize.LowPart);
				break;

			case FALSE:
				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"Index SenrenBanka\n");

				Result = InitIndexFile_SenrenBanka(pDecompress, DataHeader.OriginalSize.LowPart, file);
				break;
			}

			if (Result)
			{
				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"Normal XP3 or M2 XP3(ok)\n");
				break;
			}
		}
		else
		{
			CopyMemory(M2ChunkInfo, &Handle->M2ChunkMagic, 4);
			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleA("Chunk : %s\n", M2ChunkInfo);

			Result = InitIndex_NekoVol0(pDecompress, DataHeader.OriginalSize.LowPart);
			if (Result)
			{
				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"M2 XP3(since nekopara vol0)\n");
				break;
			}
		}

	} while (DataHeader.bZlib & 0x80);


	HeapFree(GetProcessHeap(), 0, pCompress);
	HeapFree(GetProcessHeap(), 0, pDecompress);
	return STATUS_SUCCESS;
}


VOID FASTCALL FormatM2PackName(wstring& PackageName, ttstr& OutName)
{
	OutName.Clear();
	OutName = L"archive://./";
	OutName += GetPackageName(PackageName).c_str();
	OutName += L"/";
}


VOID DecryptWorker(ULONG64 EncryptOffset, PBYTE pBuffer, ULONG BufferSize, ULONG Hash)
{
	tTVPXP3ExtractionFilterInfo Info(0, pBuffer, BufferSize, Hash);
	if (GlobalData::GetGlobalData()->pfGlobalXP3Filter != NULL)
	{
		GlobalData::GetGlobalData()->pfGlobalXP3Filter(&Info);
	}
}


NTSTATUS NTAPI KrkrDumper::DumpFileByIStream(ttstr M2Prefix, ttstr NormalPrefix)
{
	NTSTATUS                               Status;
	GlobalData*                            Handle;
	ttstr                                  OutFileName;
	wstring                                OutFilePath, OutFilePathFull;
	ULONG                                  Index, Pos;
	wstring                                ExtName, FixedPathName;
	IStream*                               Stream;
	tTJSBinaryStream*                      BStream;
	vector<wstring>                        Failed;
	WCHAR                                  CurDir[MAX_PATH];
	STATSTG                                Stat;

	RtlZeroMemory(CurDir, countof(CurDir) * sizeof(WCHAR));
	GetCurrentDirectoryW(MAX_PATH, CurDir);

	Handle = GlobalData::GetGlobalData();

	
	FixedPathName = GetPackageName(wstring(FileName));
	Pos = FixedPathName.find_last_of(L'.');
	if (Pos != wstring::npos)
		FixedPathName = FixedPathName.substr(0, Pos);

	OutFilePath += CurDir;
	OutFilePath += L"\\KrkrExtract_Output\\";
	OutFilePath += FixedPathName;
	OutFilePath += L"\\";

	Index = 1;
	for (auto& it : Handle->ItemVector)
	{
		Handle->SetCurFile(Index);

		if (Handle->IsM2Format && it.isM2Format)
		{
			OutFileName = M2Prefix + it.yuzu.Name.c_str();

			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleW(L"[M2]%s\n", OutFileName.c_str());

			ExtName = GetExtensionUpper(it.yuzu.Name);
			OutFilePathFull = OutFilePath + it.yuzu.Name;
		}
		else
		{
			OutFileName = NormalPrefix + it.info.FileName.c_str();

			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleW(L"[Normal]%s\n", OutFileName.c_str());

			ExtName = GetExtensionUpper(it.info.FileName);
			OutFilePathFull = OutFilePath + it.info.FileName;
		}

		AddPath(&OutFilePathFull[0]);
		Stream = TVPCreateIStream(OutFileName, TJS_BS_READ);
		if (Stream == NULL)
		{
			if (it.info.FileName.length() == 0 || it.info.FileName.find_first_of(L".") == std::wstring::npos)
				OutFileName = it.yuzu.Name.c_str();
			else
				OutFileName = it.info.FileName.c_str();

			BStream = NULL;
			BStream = CallTVPCreateStream(OutFileName);

			if (BStream == NULL)
			{
				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"Failed to open %s\n", OutFileName.c_str());

				Status = STATUS_UNSUCCESSFUL;
				continue;
			}
			else
			{
				Stream = ConvertBStreamToIStream(BStream);

				if (!Stream)
				{
					if (GlobalData::GetGlobalData()->DebugOn)
						PrintConsoleW(L"Failed to open %s\n", OutFileName.c_str());

					Status = STATUS_UNSUCCESSFUL;
					continue;
				}
			}
		}

		if (ExtName == L"PNG")
		{
			Status = ProcessPNG(Stream, OutFilePathFull.c_str());
		}
		else if (ExtName == L"TLG")
		{
			Status = ProcessTLG(Stream, OutFilePathFull.c_str());
		}
		else if (ExtName == L"PBD")
		{
			Status = ProcessPBD(Stream, OutFilePathFull.c_str());
		}
		else if (ExtName == L"PSB" ||
				 ExtName == L"SCN" ||
				 ExtName == L"MTN" ||
				 ExtName == L"PIMG")
		{
			Status = ProcessPSB(Stream, OutFilePathFull.c_str(), ExtName);
		}
		else if (Handle->GetTextFlag() == TEXT_DECODE &&
			(
			ExtName == L"KSD"   ||
			ExtName == L"KDT"   ||
			ExtName == L"TXT"   ||
			ExtName == L"KS"    ||
			ExtName == L"CSV"   ||
			ExtName == L"PSB"   ||
			ExtName == L"FUNC"  ||
			ExtName == L"STAND" ||
			ExtName == L"ASD"   ||
			ExtName == L"INI"   ||
			ExtName == L"TJS"))
		{
			if (ExtName == L"TJS" && Handle->GetTjsFlag() == TJS2_DECOM)
			{
				BYTE           Buffer[8];
				LARGE_INTEGER  Offset;
				ULARGE_INTEGER NewOffset;
				ULONG          Bytes;

				static BYTE TjsMark[8] = { 0x54, 0x4A, 0x53, 0x32, 0x31, 0x30, 0x30, 0x00 };

				Stream->Stat(&Stat, STATFLAG_DEFAULT);
				if (Stat.cbSize.LowPart > 8)
				{
					Stream->Read(Buffer, 8, &Bytes);
				}
				Offset.QuadPart = 0;
				Stream->Seek(Offset, FILE_BEGIN, &NewOffset);

				if (Stat.cbSize.LowPart < 8)
				{
					Status = ProcessTEXT(Stream, OutFilePathFull.c_str());
				}
				else
				{
					Status = ProcessTEXT(Stream, OutFilePathFull.c_str());
				}
			}
			else
			{
				Status = ProcessTEXT(Stream, OutFilePathFull.c_str());
			}
			
		}
		else if (ExtName == L"AMV")
		{
			Status = ProcessFile(Stream, OutFilePathFull.c_str());
		}
		else
		{
			if (ExtName == L"TJS" && Handle->GetTjsFlag() == TJS2_DECOM)
			{
				BYTE           Buffer[8];
				LARGE_INTEGER  Offset;
				ULARGE_INTEGER NewOffset;
				ULONG          Bytes;

				static BYTE TjsMark[8] = { 0x54, 0x4A, 0x53, 0x32, 0x31, 0x30, 0x30, 0x00 };

				Stream->Stat(&Stat, STATFLAG_DEFAULT);
				if (Stat.cbSize.LowPart > 8)
				{
					Stream->Read(Buffer, 8, &Bytes);
				}
				Offset.QuadPart = 0;
				Stream->Seek(Offset, FILE_BEGIN, &NewOffset);

				if (Stat.cbSize.LowPart < 8)
				{
					Status = ProcessFile(Stream, OutFilePathFull.c_str());
				}
				else
				{
					Status = ProcessFile(Stream, OutFilePathFull.c_str());
				}
			}
			else
			{
				Status = ProcessFile(Stream, OutFilePathFull.c_str());
			}
		}

		if (NT_FAILED(Status))
			Failed.push_back(OutFilePathFull);

		Index++;
	}

	if (GlobalData::GetGlobalData()->DebugOn)
	{
		for (auto& Item : Failed)
			PrintConsoleW(L"Failed to open : %s\n", Item);
	}
	return Status;
}

NTSTATUS NTAPI KrkrDumper::DumpFileByRawFile()
{
	NTSTATUS      Status;
	GlobalData*   Handle;
	NtFileDisk    FakeWorker, File;
	PBYTE         WriteBuffer, OutBuffer;
	ULONG         OriSize, ArcSize;
	WCHAR         CurPath[MAX_PATH];

	Handle = GlobalData::GetGlobalData();
	RtlZeroMemory(CurPath, countof(CurPath)*sizeof(WCHAR));

	GetCurrentDirectoryW(MAX_PATH, CurPath);

	Status = FakeWorker.Open(FileName);
	if (NT_FAILED(Status))
		return Status;

	for (auto& it : Handle->ItemVector)
	{
		wstring OutputFilePath = CurPath;
		OutputFilePath += L"\\KrkrExtract_Output\\";
		OutputFilePath += GetPackageName(wstring(FileName));
		OutputFilePath += L"\\";
		OutputFilePath += it.info.FileName;
		AddPath((LPWSTR)OutputFilePath.c_str());
		File.Create(OutputFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (it.info.EncryptedFlag & 7)
		{
			OutBuffer   = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, it.info.ArchiveSize.LowPart);
			WriteBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, it.info.OriginalSize.LowPart);

			FakeWorker.Seek(it.segm.segm[0].Offset.LowPart, FILE_BEGIN);
			FakeWorker.Read(OutBuffer, it.info.ArchiveSize.LowPart);

			OriSize = it.info.OriginalSize.LowPart;
			ArcSize = it.info.ArchiveSize.LowPart;

			uncompress(WriteBuffer, &OriSize, OutBuffer, ArcSize);

			DecryptWorker(0, WriteBuffer, it.info.ArchiveSize.LowPart, it.adlr.Hash);
			File.Write(WriteBuffer, it.info.OriginalSize.LowPart);

			HeapFree(GetProcessHeap(), 0, WriteBuffer);
			HeapFree(GetProcessHeap(), 0, OutBuffer);
		}
		else
		{
			WriteBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, it.info.OriginalSize.LowPart);
			FakeWorker.Seek(it.segm.segm[0].Offset.LowPart, FILE_BEGIN);
			FakeWorker.Read(WriteBuffer, it.info.OriginalSize.LowPart);

			DecryptWorker(0, WriteBuffer, it.info.ArchiveSize.LowPart, it.adlr.Hash);
			File.Write(WriteBuffer, it.info.OriginalSize.LowPart);

			HeapFree(GetProcessHeap(), 0, WriteBuffer);
		}
		File.Close();
	}
	FakeWorker.Close();
	return Status;
}

NTSTATUS NTAPI KrkrDumper::DumpFile()
{
	NTSTATUS        Status;
	GlobalData*     Handle;
	ttstr           M2Prefix;
	ttstr           NormalPrefix;

	Handle = GlobalData::GetGlobalData();

	FormatPath(wstring(FileName), NormalPrefix);
	FormatM2PackName(wstring(FileName), M2Prefix);

	LOOP_ONCE
	{
		Status = STATUS_SUCCESS;
		if (Handle->ItemVector.size() == 0)
		{
			GlobalData::GetGlobalData()->isRunning = FALSE;
			break;
		}

		if (!Handle->RawExtract)
			Status = DumpFileByIStream(M2Prefix, NormalPrefix);
		else
			Status = DumpFileByRawFile();
	}
	Handle->Reset();
	return Status;
}

NTSTATUS NTAPI KrkrDumper::DoDump()
{
	NTSTATUS     Status;
	GlobalData*  Handle;

	Handle = GlobalData::GetGlobalData();

	Handle->isRunning = TRUE;
	Handle->DisableAll(Handle->MainWindow);
	
	if (!IsIndex)
		Status = ParseXP3File(FileName);
	else
		Status = ParseXP3iFile(FileNameIndex);

	Status = DumpFile();
	return Status;
}


/**************************************************/

static KrkrDumper* LocalKrkrDumper = NULL;

DWORD WINAPI ExtractThread(LPVOID lParam)
{
	return LocalKrkrDumper->DoDump();
}



HANDLE NTAPI StartDumper(LPCWSTR lpFileName)
{
	NTSTATUS     Status;
	GlobalData*  Handle;

	Handle = GlobalData::GetGlobalData();

	if (LocalKrkrDumper == NULL)
		LocalKrkrDumper = new KrkrDumper;

	LOOP_ONCE
	{
		LocalKrkrDumper->InternalReset();
		LocalKrkrDumper->SetFile(lpFileName);
		LocalKrkrDumper->IsIndex = FALSE;
		Status = Nt_CreateThread(ExtractThread, NULL, FALSE, NtCurrentProcess(), &LocalKrkrDumper->hThread);

		if (NT_FAILED(Status))
		{
			MessageBoxW(NULL, L"Failed to Start Extraction Thread!", L"KrkrExtract", MB_OK);
			break;
		}
	}
	return LocalKrkrDumper->hThread;
}


HANDLE NTAPI StartMiniDumper(LPCWSTR lpFileName)
{
	NTSTATUS     Status;
	GlobalData*  Handle;

	Handle = GlobalData::GetGlobalData();

	if (LocalKrkrDumper == NULL)
		LocalKrkrDumper = new KrkrDumper;

	LOOP_ONCE
	{
		LocalKrkrDumper->InternalReset();

		auto GetFileNameP = [](PCWSTR Name)->std::wstring
		{
			std::wstring FileName(Name);
			auto Index = FileName.find_last_of(L'.');
			if (Index != std::wstring::npos)
				return FileName.substr(0, Index);

			return FileName;
		};
		
		auto FileName = GetFileNameP(lpFileName) + L".xp3";
		LocalKrkrDumper->SetFile(FileName.c_str());
		RtlZeroMemory(LocalKrkrDumper->FileNameIndex, countof(LocalKrkrDumper->FileNameIndex) * sizeof(WCHAR));
		lstrcpyW(LocalKrkrDumper->FileNameIndex, lpFileName);
		LocalKrkrDumper->IsIndex = TRUE;
		Status = Nt_CreateThread(ExtractThread, NULL, FALSE, NtCurrentProcess(), &LocalKrkrDumper->hThread);
		
		if (NT_FAILED(Status))
		{
			MessageBoxW(NULL, L"Failed to Start Extraction Thread!", L"KrkrExtract", MB_OK);
			break;
		}
	}
	return LocalKrkrDumper->hThread;
}
