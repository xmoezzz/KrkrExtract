#include "KrkrDumper.h"
#include "resource.h"
#include "MyHook.h"
#include "KrkrExtend.h"
#include "zlib.h"
#include <WindowsX.h>

/********************************************/

KrkrDumper::KrkrDumper()
{
	hThread = INVALID_HANDLE_VALUE;
}


KrkrDumper::~KrkrDumper()
{
}



Void NTAPI KrkrDumper::InternalReset()
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
		PrintConsoleW(L"Analyze ok...\n");
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


Void NTAPI KrkrDumper::AddPath(LPWSTR FileName)
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

Void NTAPI KrkrDumper::SetFile(LPCWSTR lpFile)
{
	RtlZeroMemory(FileName, countof(FileName) * sizeof(WCHAR));
	StrCopyW(FileName, lpFile);
}


NTSTATUS NTAPI KrkrDumper::ProcessXP3Archive(LPCWSTR lpFileName, NtFileDisk& file)
{
	NTSTATUS                Status;
	GlobalData*             Handle;
	ULONG                   Count;
	KRKR2_XP3_HEADER        XP3Header;
	KRKR2_XP3_DATA_HEADER   DataHeader;
	PBYTE                   Indexdata;
	LARGE_INTEGER           BeginOffset, Offset;
	CHAR                    M2ChunkInfo[8];


	Handle = GlobalData::GetGlobalData();

	Count = 0;
	BeginOffset.QuadPart = 0;
	Handle = GlobalData::GetGlobalData();
	Status = file.Read(&XP3Header, sizeof(XP3Header));
	RtlZeroMemory(M2ChunkInfo, sizeof(M2ChunkInfo));

	if (NT_FAILED(Status))
	{
		if (Handle->DebugOn)
			PrintConsoleW(L"Invaild Package\n");

		return Status;
	}

	//Exe Built-in Package Support
	if ((*(PUSHORT)XP3Header.Magic) == IMAGE_DOS_SIGNATURE)
	{
		Status = FindEmbededXp3OffsetSlow(FileName, &BeginOffset);

		if (NT_FAILED(Status))
		{
			if (Handle->DebugOn)
				PrintConsoleW(L"No a Built-in Package\n");

			return Status;
		}

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
	PBYTE pCompress = (PBYTE)AllocateMemoryP((ULONG)CompresseBufferSize);
	PBYTE pDecompress = (PBYTE)AllocateMemoryP((ULONG)DecompressBufferSize);
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
			pCompress = (PBYTE)ReAllocateMemoryP(pCompress, (ULONG)CompresseBufferSize);
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
				pDecompress = (PBYTE)ReAllocateMemoryP(pDecompress, (ULONG)DecompressBufferSize);
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
				pDecompress = (PBYTE)ReAllocateMemoryP(pDecompress, (ULONG)DecompressBufferSize);
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
				Result = InitIndexFileFirst(pDecompress, DataHeader.OriginalSize.LowPart);
				break;

			case FALSE:
				Result = InitIndexFile_SenrenBanka(pDecompress, DataHeader.OriginalSize.LowPart, file);
				break;
			}

			if (Result)
			{
				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"Normal XP3 or M2 XP3\n");
				break;
			}
		}
		else
		{
			Result = InitIndex_NekoVol0(pDecompress, DataHeader.OriginalSize.LowPart);
			if (Result)
			{
				if (GlobalData::GetGlobalData()->DebugOn)
					PrintConsoleW(L"M2 XP3(since nekopara vol2)\n");
				break;
			}
		}

	} while (DataHeader.bZlib & 0x80);


	FreeMemoryP(pCompress);
	FreeMemoryP(pDecompress);

	return STATUS_SUCCESS;
}


//Full Path Name
void FormatPath(wstring& package, ttstr& out)
{
	out.Clear();
	out = L"file://./";
	for (unsigned int iPos = 0; iPos < package.length(); iPos++)
	{
		if (package[iPos] == L':')
			continue;

		if (package[iPos] == L'\\')
			out += L'/';
		else
			out += package[iPos];
	}
	out += L'>';
}

wstring FormatPathFull(LPCWSTR path)
{
	WCHAR  Buffer[MAX_PATH];

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
			RtlZeroMemory(Buffer, countof(Buffer) * sizeof(WCHAR));
			Nt_GetCurrentDirectory(countof(Buffer), Buffer);
			wstring res = FormatPathFull((wstring(Buffer) + L'/' + path).c_str());
			return res;
		}
	}
	return L"";
}


wstring GetPackageName(wstring& FileName)
{
	wstring            temp(FileName);
	wstring::size_type pos = temp.find_last_of(L'\\');

	if (pos != wstring::npos)
		temp = temp.substr(pos + 1, wstring::npos);

	wstring            temp2(temp);
	wstring::size_type pos2 = temp2.find_last_of(L'/');
	if (pos2 != wstring::npos)
		temp2 = temp2.substr(pos2 + 1, wstring::npos);
	
	return temp2;
}

wstring GetPackageName(LPCWSTR FileName)
{
	return GetPackageName(wstring(FileName));
}

wstring GetExtensionUpperV2(wstring& FileName)
{
	wstring            OutName;
	wstring::size_type pos = FileName.find_last_of(L'.');
	if (pos != wstring::npos)
		OutName = FileName.substr(pos, wstring::npos);

	for (auto& Char : OutName)
		Char = CHAR_UPPER(Char);
	
	return OutName;
}

Void FormatArc(wstring& package, ttstr& out)
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


NTSTATUS NTAPI KrkrDumper::ProcessFile(IStream* Stream, LPCWSTR OutFileName, XP3Index& it)
{
	NTSTATUS         Status;
	STATSTG          Stat;
	NtFileDisk       File;
	LARGE_INTEGER    Tranferred, WriteSize, TempSize, Offset;
	ULONG            ReadSize;

	static BYTE Buffer[1024 * 64];

	Offset.QuadPart = 0;
	Stream->Seek(Offset, FILE_BEGIN, NULL);
	Stream->Stat(&Stat, STATFLAG_DEFAULT);
	Tranferred.QuadPart = 0;

	Status = File.Create(OutFileName);
	if (NT_FAILED(Status))
	{
		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"Failed to write %s[%08x]\n", OutFileName, Status);

		return Status;
	}

	while (Tranferred.QuadPart < (LONG64)Stat.cbSize.QuadPart)
	{
		Stream->Read(&Buffer, sizeof(Buffer), &ReadSize);
		Tranferred.QuadPart += ReadSize;
		TempSize.QuadPart = 0;
		while (TempSize.QuadPart < ReadSize)
		{
			File.Write(Buffer, ReadSize, &WriteSize);
			TempSize.QuadPart += WriteSize.QuadPart;
		}
	}
	File.Close();
	return Status;
}


NTSTATUS NTAPI KrkrDumper::ProcessPSB(IStream* Stream, LPCWSTR OutFileName, XP3Index& it, wstring& ExtName)
{
	NTSTATUS    Status;
	NtFileDisk  File;
	STATSTG     Stat;
	PBYTE       Buffer;
	ULONG       ReadSize;

	Stream->Stat(&Stat, STATFLAG_DEFAULT);

	Status = STATUS_ABANDONED;
	if (GlobalData::GetGlobalData()->PsbFlagOn(PSB_RAW) ||
		GlobalData::GetGlobalData()->PsbFlagOn(PSB_ALL))
	{
		return ProcessFile(Stream, OutFileName, it);
	}

	if (GlobalData::GetGlobalData()->PsbFlagOn(PSB_ALL) ||
		GlobalData::GetGlobalData()->PsbFlagOn(PSB_TEXT) ||
		GlobalData::GetGlobalData()->PsbFlagOn(PSB_DECOM) ||
		GlobalData::GetGlobalData()->PsbFlagOn(PSB_ANM))
	{
		Status = ExtractPsb(Stream, GlobalData::GetGlobalData()->PsbFlagOn(PSB_IMAGE) ||
			GlobalData::GetGlobalData()->PsbFlagOn(PSB_ANM),
			GlobalData::GetGlobalData()->PsbFlagOn(PSB_DECOM),
			ExtName, OutFileName);

		Status = Status >= 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	}
	return Status;
}

NTSTATUS NTAPI KrkrDumper::ProcessTLG(IStream* Stream, LPCWSTR OutFileName, XP3Index& it)
{
	NTSTATUS         Status;
	STATSTG          Stat;
	LARGE_INTEGER    Tranferred, WriteSize, TempSize, Offset;
	ULONG            ReadSize, OutSize;
	PBYTE            Buffer, RawBuffer, OutBuffer;
	BOOL             TempDecode;
	NtFileDisk       File;
	GlobalData*      Handle;

	Handle = GlobalData::GetGlobalData();

	Stream->Stat(&Stat, STATFLAG_DEFAULT);

	Buffer = (PBYTE)AllocateMemoryP(Stat.cbSize.LowPart);
	if (!Buffer)
	{
		MessageBoxW(Handle->MainWindow, L"Failed to Allocate memory for tlg Decoder", L"KrkrExtract", MB_OK);
		return STATUS_UNSUCCESSFUL;
	}

	Status = STATUS_SUCCESS;
	Stream->Read(Buffer, Stat.cbSize.LowPart, &ReadSize);
	RawBuffer = Buffer;

	switch (Handle->GetTlgFlag())
	{
	case TLG_RAW:
		Offset.QuadPart = 0;
		Stream->Seek(Offset, FILE_BEGIN, NULL);
		FreeMemoryP(RawBuffer);
		return ProcessFile(Stream, OutFileName, it);

	case TLG_SYS:
		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"Using System Decode Mode[TLG]\n");

		FreeMemoryP(RawBuffer);
		SavePng(GetPackageName(OutFileName).c_str(), FormatPathFull((wstring(OutFileName) + L".png").c_str()).c_str());
		return Status;


	case TLG_BUILDIN:
	case TLG_PNG:
	{

		OutBuffer = NULL;
		OutSize = 0;
		TempDecode = TRUE;

		if (RtlCompareMemory(Buffer, KRKR2_TLG0_MAGIC, 6) == 6)
			Buffer += 0xF;

		if (RtlCompareMemory(Buffer, KRKR2_TLG5_MAGIC, 6) == 6)
		{
			if (!DecodeTLG5(Buffer, Stat.cbSize.LowPart, (PVOID*)&OutBuffer, &OutSize))
			{
				TempDecode = FALSE;
				OutSize = Stat.cbSize.LowPart;
				OutBuffer = Buffer;
			}
		}
		else if (RtlCompareMemory(Buffer, KRKR2_TLG6_MAGIC, 6) == 6)
		{
			if (!DecodeTLG6(Buffer, Stat.cbSize.LowPart, (PVOID*)&OutBuffer, &OutSize))
			{
				TempDecode = FALSE;
				OutSize = Stat.cbSize.LowPart;
				OutBuffer = Buffer;
			}
		}
		else
		{
			TempDecode = FALSE;
			OutSize = Stat.cbSize.LowPart;
			OutBuffer = Buffer;
		}

		if (Handle->GetTlgFlag() == TLG_BUILDIN)
		{
			if (Handle->DebugOn)
				PrintConsoleW(L"Decoding tlg image to bmp...\n");

			if (!TempDecode)
				Status = File.Create(OutFileName);
			else
				Status = File.Create((wstring(OutFileName) + L".bmp").c_str());

			if (NT_FAILED(Status))
			{
				FreeMemoryP(RawBuffer);
				if (TempDecode)
					FreeMemoryP(OutBuffer);

				File.Close();
				return STATUS_UNSUCCESSFUL;
			}

			File.Write(OutBuffer, OutSize);

			FreeMemoryP(RawBuffer);
			if (TempDecode)
				FreeMemoryP(OutBuffer);
		}
		else if (Handle->GetTlgFlag() == TLG_PNG)
		{
			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleW(L"Decoding tlg image to png file(Build-in)...\n");

			if (TempDecode)
			{
				if (Bmp2PNG(OutBuffer, OutSize, (wstring(OutFileName) + L".png").c_str()) < 0)
				{
					Status = File.Create((wstring(OutFileName) + L".bmp").c_str());

					if (NT_FAILED(Status))
						return Status;

					File.Write(OutBuffer, OutSize);
					File.Close();
				}

				FreeMemoryP(RawBuffer);
				FreeMemoryP(OutBuffer);
			}
			else
			{
				FreeMemoryP(RawBuffer);
				Offset.QuadPart = 0;
				Stream->Seek(Offset, FILE_BEGIN, NULL);
				return ProcessFile(Stream, OutFileName, it);
			}
		}
	}
	break;
	}
	return Status;
}

NTSTATUS NTAPI KrkrDumper::ProcessPNG(IStream* Stream, LPCWSTR OutFileName, XP3Index& it)
{
	NTSTATUS         Status;
	STATSTG          Stat;
	NtFileDisk       File;
	LARGE_INTEGER    Tranferred, WriteSize, TempSize;
	ULONG            ReadSize;

	if (GlobalData::GetGlobalData()->GetPngFlag() == PNG_SYS)
	{
		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"Using Build-in Decode Mode\n");

		SavePng(GetPackageName(OutFileName).c_str(), FormatPathFull(OutFileName).c_str());
	}
	else if (GlobalData::GetGlobalData()->GetPngFlag() == PNG_RAW)
	{
		Status = ProcessFile(Stream, OutFileName, it);
	}
	return Status;
}



NTSTATUS NTAPI KrkrDumper::ProcessTEXT(IStream* Stream, LPCWSTR OutFileName, XP3Index& it)
{
	NTSTATUS     Status;
	STATSTG      Stat;
	ULONG        ReadSize;
	PBYTE        OriBuffer;
	WCHAR        FileName[MAX_PATH];

	Stream->Stat(&Stat, STATFLAG_DEFAULT);
	RtlZeroMemory(FileName, countof(FileName) * sizeof(WCHAR));
	StrCopyW(FileName, OutFileName);

	LOOP_ONCE
	{
		Status = STATUS_SUCCESS;
		OriBuffer = (PBYTE)AllocateMemoryP(Stat.cbSize.LowPart);
		if (!OriBuffer)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		Stream->Read(OriBuffer, Stat.cbSize.LowPart, &ReadSize);

		Status = STATUS_SUCCESS;
		if (DecodeText(OriBuffer, Stat.cbSize.LowPart, FileName) != -1)
			FreeMemoryP(OriBuffer);
		else
			Status = ProcessFile(Stream, FileName, it);
	}
	return Status;
}


NTSTATUS NTAPI KrkrDumper::DumpFileByIStream(ttstr M2Prefix, ttstr NormalPrefix)
{
	NTSTATUS          Status;
	GlobalData*       Handle;
	ttstr             OutFileName;
	wstring           OutFilePath, OutFilePathFull;
	ULONG             Index, Pos;
	wstring           ExtName, FixedPathName;
	IStream*          Stream;
	tTJSBinaryStream* BStream;
	vector<wstring>   Failed;
	WCHAR             CurDir[MAX_PATH];

	RtlZeroMemory(CurDir, countof(CurDir) * sizeof(WCHAR));
	Nt_GetCurrentDirectory(MAX_PATH, CurDir);

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

			ExtName = GetExtensionUpperV2(it.yuzu.Name);
			OutFilePathFull = OutFilePath + it.yuzu.Name;
		}
		else
		{
			OutFileName = NormalPrefix + it.info.FileName.c_str();

			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleW(L"[Normal]%s\n", OutFileName.c_str());

			ExtName = GetExtensionUpperV2(it.info.FileName);
			OutFilePathFull = OutFilePath + it.info.FileName;
		}

		AddPath(&OutFilePathFull[0]);
		Stream = TVPCreateIStream(OutFileName, TJS_BS_READ);
		if (Stream == NULL)
		{
			if (it.info.FileName.length() == 0 || it.info.FileName.find(L".", 0) == wstring::npos)
				OutFileName = it.yuzu.Name.c_str();
			else
				OutFileName = it.info.FileName.c_str();

//Handware ex
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

		if (ExtName == L".PNG")
		{
			Status = ProcessPNG(Stream, OutFilePathFull.c_str(), it);
		}
		else if (ExtName == L".TLG")
		{
			Status = ProcessTLG(Stream, OutFilePathFull.c_str(), it);
		}
		else if (ExtName == L".PSB" ||
				 ExtName == L".SCN" ||
				 ExtName == L".MTN" ||
				 ExtName == L".PIMG")
		{
			Status = ProcessPSB(Stream, OutFilePathFull.c_str(), it, ExtName);
		}
		else if (GlobalData::GetGlobalData()->GetTextFlag() == TEXT_DECODE &&
			(
			ExtName == L".KSD"   ||
			ExtName == L".KDT"   ||
			ExtName == L".TXT"   ||
			ExtName == L".KS"    ||
			ExtName == L".CSV"   ||
			ExtName == L".PSB"   ||
			ExtName == L".FUNC"  ||
			ExtName == L".STAND" ||
			ExtName == L".ASD"   ||
			ExtName == L".INI"   ||
			ExtName == L".TJS"))
		{
			Status = ProcessTEXT(Stream, OutFilePathFull.c_str(), it);
		}
		else
		{
			Status = ProcessFile(Stream, OutFilePathFull.c_str(), it);
		}

		if (NT_FAILED(Status))
			Failed.push_back(OutFilePathFull);

		Index++;
	}

	if (GlobalData::GetGlobalData()->DebugOn)
	{
		for (auto& iter : Failed)
			PrintConsoleW(L"Failed to open : %s\n", iter.c_str());
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

	Nt_GetCurrentDirectory(MAX_PATH, CurPath);

	Status = FakeWorker.Open(FileName);
	if (NT_FAILED(Status))
		return Status;

	for (auto& it : Handle->ItemVector)
	{
		ULONG os = 0;
		wstring outFilePath = CurPath;
		outFilePath += L"\\outPath\\";
		outFilePath += GetPackageName(wstring(FileName));
		outFilePath += L"\\";
		outFilePath += it.info.FileName.c_str();
		AddPath(&outFilePath[0]);
		File.Create(outFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (it.info.EncryptedFlag & 7)
		{
			OutBuffer = (PBYTE)AllocateMemoryP(it.info.ArchiveSize.LowPart);
			WriteBuffer = (PBYTE)AllocateMemoryP(it.info.OriginalSize.LowPart);

			FakeWorker.Seek(it.segm.segm[0].Offset.LowPart, FILE_BEGIN);
			FakeWorker.Read(OutBuffer, it.info.ArchiveSize.LowPart);

			OriSize = it.info.OriginalSize.LowPart;
			ArcSize = it.info.ArchiveSize.LowPart;

			uncompress(WriteBuffer, &OriSize, OutBuffer, ArcSize);

			DecryptWorker(0, WriteBuffer, it.info.ArchiveSize.LowPart, it.adlr.Hash);
			File.Write(WriteBuffer, it.info.OriginalSize.LowPart);

			FreeMemoryP(WriteBuffer);
			FreeMemoryP(OutBuffer);
		}
		else
		{
			WriteBuffer = (PBYTE)AllocateMemoryP(it.info.OriginalSize.LowPart);
			FakeWorker.Seek(it.segm.segm[0].Offset.LowPart, FILE_BEGIN);
			FakeWorker.Read(WriteBuffer, it.info.OriginalSize.LowPart);

			DecryptWorker(0, WriteBuffer, it.info.ArchiveSize.LowPart, it.adlr.Hash);
			File.Write(WriteBuffer, it.info.OriginalSize.LowPart);

			FreeMemory(WriteBuffer);
		}
		File.Close();
	}
	FakeWorker.Close();
	return Status;
}

//检查png!!
NTSTATUS NTAPI KrkrDumper::DumpFile()
{
	NTSTATUS        Status;
	GlobalData*     Handle;
	ttstr           M2Prefix;
	ttstr           NormalPrefix;

	Handle = GlobalData::GetGlobalData();

	FormatPath(wstring(FileName), NormalPrefix);
	FormatArc(wstring(FileName), M2Prefix);

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
	Status = ParseXP3File(FileName);
	Status = DumpFile();
	return Status;
}


/**************************************************/

static KrkrDumper LocalKrkrDumper;

DWORD WINAPI ExtractThread(LPVOID lParam)
{
	return LocalKrkrDumper.DoDump();
}



HANDLE NTAPI StartDumper(LPCWSTR lpFileName)
{
	NTSTATUS     Status;
	GlobalData*  Handle;

	Handle = GlobalData::GetGlobalData();

	LOOP_ONCE
	{
		LocalKrkrDumper.InternalReset();
		LocalKrkrDumper.SetFile(lpFileName);
		Status = Nt_CreateThread(ExtractThread, NULL, FALSE, NtCurrentProcess(), &LocalKrkrDumper.hThread);

		if (NT_FAILED(Status))
		{
			MessageBoxW(NULL, L"Failed to Start Extraction Thread!", L"KrkrExtract", MB_OK);
			break;
		}
	}
	return LocalKrkrDumper.hThread;
}

