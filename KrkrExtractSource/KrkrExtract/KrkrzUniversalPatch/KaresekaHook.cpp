#include "KaresekaHook.h"
#include "MyHook.h"
#include "StreamHolderXP3.h"
#include "IStreamExXP3.h"
#include "tp_stub.h"
#include <string>
#include <vector>
#include "CompilerSet.h"

#pragma comment(lib, "jsoncpp.lib")

using std::wstring;

PVOID GetTVPCreateStreamCall()
{
	KaresekaHook* Kareseka;
	PVOID         CallIStreamStub, CallIStream, CallTVPCreateStreamCall;
	ULONG         OpSize, OpOffset;
	WORD          WordOpcode;

	static char funcname[] = "IStream * ::TVPCreateIStream(const ttstr &,tjs_uint32)";

	Kareseka = GetKareseka();

	LOOP_ONCE
	{
		CallTVPCreateStreamCall = NULL;

		CallIStreamStub = TVPGetImportFuncPtr(funcname);
		if (!CallIStreamStub)
			break;

		CallIStream = NULL;
		OpOffset = 0;

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
					Kareseka->StubHostAlloc = (FuncHostAlloc)GetCallDestination(((ULONG_PTR)CallIStream + OpOffset));
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
				Kareseka->IStreamAdapterVtable = *(PULONG_PTR)((PBYTE)CallIStream + OpOffset);
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

	if (Kareseka->StubHostAlloc && Kareseka->IStreamAdapterVtable)
		return CallTVPCreateStreamCall;
	else
		return NULL;
}


tTJSBinaryStream* FASTCALL CallTVPCreateStream(const ttstr& FilePath)
{
	tTJSBinaryStream* Stream;
	KaresekaHook*     Handle;

	Handle = GetKareseka();

	if (Handle->StubTVPCreateStream == NULL)
		Handle->StubTVPCreateStream = (FuncCreateStream)GetTVPCreateStreamCall();

	Stream = NULL;

	if (Handle->StubTVPCreateStream == NULL)
		return Stream;

	return Handle->StubTVPCreateStream(FilePath, TJS_BS_READ);
}

IStream* FASTCALL ConvertBStreamToIStream(tTJSBinaryStream* BStream)
{
	KaresekaHook* Kareseka;
	IStream*      Stream;
	PVOID         CallHostAlloc;
	ULONG_PTR     IStreamAdapterVTableOffset;


	Kareseka = GetKareseka();

	CallHostAlloc              = Kareseka->StubHostAlloc;
	IStreamAdapterVTableOffset = Kareseka->IStreamAdapterVtable;
	Stream = NULL;

	INLINE_ASM
	{
		push 0xC;
		call CallHostAlloc;
		add  esp, 0x4;
		test eax, eax;
		jz   NO_CREATE_STREAM;
		mov  esi, IStreamAdapterVTableOffset;
		mov  dword ptr[eax], esi; //Vtable 
		mov  esi, BStream;
		mov  dword ptr[eax + 4], esi; //StreamHolder
		mov  dword ptr[eax + 8], 1;   //ReferCount
		mov  Stream, eax;

	NO_CREATE_STREAM:
	}

	return Stream;
}


class tTJSCriticalSection
{
	CRITICAL_SECTION CS;
public:
	tTJSCriticalSection()  { InitializeCriticalSection(&CS); }
	~tTJSCriticalSection() { DeleteCriticalSection(&CS); }

	void Enter() { EnterCriticalSection(&CS); }
	void Leave() { LeaveCriticalSection(&CS); }
};

class tTJSCriticalSectionHolder
{
	tTJSCriticalSection *Section;
public:
	tTJSCriticalSectionHolder(tTJSCriticalSection &cs)
	{
		Section = &cs;
		Section->Enter();
	}

	~tTJSCriticalSectionHolder()
	{
		Section->Leave();
	}

};


KaresekaHook* KaresekaHook::Handle = NULL;

KaresekaHook::KaresekaHook()
{
	StubTVPCreateStream  = NULL;
	StubHostAlloc        = NULL;
	StubV2Link           = NULL;
	IStreamAdapterVtable = NULL;
	TVPFunctionExporter  = NULL;
	m_SelfModule         = NULL;
	FileSystemInited     = FALSE;
}

KaresekaHook* FASTCALL GetKareseka()
{
	if (KaresekaHook::Handle == NULL)
		KaresekaHook::Handle = new KaresekaHook();

	return KaresekaHook::Handle;
}

std::wstring GetKrkrFileName(LPCWSTR Name)
{
	std::wstring Info(Name);

	if (Info.find_last_of(L">") != std::wstring::npos)
		Info = Info.substr(Info.find_last_of(L">") + 1, std::wstring::npos);

	if (Info.find_last_of(L"/") != std::wstring::npos)
		Info = Info.substr(Info.find_last_of(L"/") + 1, std::wstring::npos);

	return Info;
}

void FileNameToLower(std::wstring& FileName)
{
	for (auto& Item : FileName)
	{
		if (Item <= L'Z' && Item >= L'A')
			Item += towlower(Item);
	}
}


NTSTATUS NTAPI QueryFileAPI(LPCWSTR FileName, PBYTE& Buffer, ULONG& Size)
{
	ULONG64       Hash;
	KaresekaHook* Handle;


	Handle = GetKareseka();
	return Handle->QueryFile(FileName, FileName, Buffer, Size, Hash);
}


static tTJSCriticalSection LocalCreateStreamCS;

IStream* KaresekaHook::CreateLocalStream(LPCWSTR lpFileName)
{

	tTJSCriticalSectionHolder CSHolder(LocalCreateStreamCS);

	NTSTATUS              Status;
	ULONG                 FileSize;
	PBYTE                 FileBuffer;
	PBYTE                 TempBuffer;
	ULONG                 TempSize;
	PBYTE                 TextBuffer;
	ULONG                 TextSize;
	ULONG64               Hash;
	std::wstring          FileName, FileNameText;
	StreamHolderXP3*      Holder;
	IStreamAdapterXP3*    StreamAdapter;

	FileName = GetKrkrFileName(lpFileName);
	
	LOOP_ONCE
	{
		StreamAdapter = nullptr;

		FileNameToLower(FileName);
		auto TextIndex = TextList.find(FileName);
		auto CompIndex = JITList.find(FileName);

		LOOP_ONCE
		{
			if (TextIndex == TextList.end() && CompIndex == JITList.end())
			{
				Status = QueryFile(lpFileName, &FileName[0], FileBuffer, FileSize, Hash);
			}
			else if (TextIndex != TextList.end())
			{
				TempBuffer = NULL;
				TempSize = 0;
				Status = CompilePsbFull(FileName, TempBuffer, TempSize, QueryFileAPI);
				if (NT_FAILED(Status))
				{
					if (TempBuffer)
						FreeMemoryP(TempBuffer);

					break;
				}

				TextBuffer = NULL;
				TextSize   = 0;
				FileNameText = FileName + L".txt";
				Status = QueryFile(FileNameText.c_str(), &FileNameText[0], TextBuffer, TextSize, Hash);
				if (NT_FAILED(Status) || !TextBuffer || !TextSize)
				{
					if (TempBuffer)
						FreeMemoryP(TempBuffer);

					break;
				}

				Status = CompilePsbText(TempBuffer, TempSize, TextBuffer, TextSize, FileBuffer, FileSize);
				if (Status != 0)
				{
					if (TempBuffer)
						FreeMemoryP(TempBuffer);

					if (TextBuffer)
						FreeMemoryP(TextBuffer);

					if (FileBuffer)
						FreeMemoryP(FileBuffer);

					FileBuffer = NULL;
					FileSize   = 0;

					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else if (CompIndex != JITList.end())
			{
				Status = CompilePsbFull(FileName, FileBuffer, FileSize, QueryFileAPI);
				if (NT_FAILED(Status))
				{
					if (FileBuffer)
						FreeMemoryP(FileBuffer);

					FileBuffer = NULL;
					FileSize = 0;
				}
			}
		}

		if (NT_FAILED(Status) || !FileBuffer || !FileSize)
			break;

		Holder        = new StreamHolderXP3(FileBuffer, FileSize);
		StreamAdapter = new IStreamAdapterXP3(Holder);
	}
	return StreamAdapter;
}


tTJSBinaryStream* FASTCALL HookTVPCreateStream(const ttstr & _name, tjs_uint32 flags)
{
	KaresekaHook*      Kareseka;
	tTJSBinaryStream*  Stream;
	IStream*           IStream;

	Kareseka = GetKareseka();

	LOOP_ONCE
	{
		Stream  = NULL;
		IStream = NULL;

		if (flags == TJS_BS_READ)
			IStream = Kareseka->CreateLocalStream(_name.c_str());

		if (IStream)
		{
			
			Stream = TVPCreateBinaryStreamAdapter(IStream);
			if (Stream)
				break;
		}

		Stream = Kareseka->StubTVPCreateStream(_name, flags);
	}
	return Stream;
}

BOOL Inited = FALSE;

HRESULT WINAPI HookV2Link(iTVPFunctionExporter *exporter)
{
	NTSTATUS       Status;
	KaresekaHook*  Kareseka;

	if (!Inited)
	{
		Kareseka = GetKareseka();
		TVPInitImportStub(exporter);
		Kareseka->TVPFunctionExporter = exporter;
		Kareseka->StubTVPCreateStream = (FuncCreateStream)GetTVPCreateStreamCall();

		INLINE_PATCH_DATA f[] =
		{
			{ Kareseka->StubTVPCreateStream, HookTVPCreateStream, (PVOID*)&(Kareseka->StubTVPCreateStream) }
		};

		Status = InlinePatchMemory(f, countof(f));
		Inited = TRUE;
	}
	return Kareseka->StubV2Link(exporter);
}


int WINAPI HookMultiByteToWideChar(
	UINT   CodePage,
	DWORD  dwFlags,
	LPCSTR lpMultiByteStr,
	int    cbMultiByte,
	LPWSTR lpWideCharStr,
	int    cchWideChar
	)
{
	switch (CodePage)
	{
	case CP_ACP:
	case CP_OEMCP:
	case CP_THREAD_ACP:
		CodePage = 932;
		break;

	default:
		break;
	}

	return
		MultiByteToWideChar(
		CodePage,
		dwFlags,
		lpMultiByteStr,
		cbMultiByte,
		lpWideCharStr,
		cchWideChar
		);
}


NTSTATUS KaresekaHook::QueryFile(LPCWSTR QueryPathName, LPCWSTR FileName, PBYTE& FileBuffer, ULONG& FileSize, ULONG64& Hash)
{
	NtFileDisk File;
	NTSTATUS   Status;
	WCHAR      FullFileName[MAX_PATH];

	if (!FileName)
		return STATUS_NO_SUCH_FILE;

	FileBuffer = NULL;
	FileSize   = 0;
	Hash       = 0;


	if (!FileSystemInited)
	{
		LOOP_ONCE
		{
			FormatStringW(FullFileName, L"ProjectDir\\%s", FileName);
			Status = File.Open(FullFileName);
			if (NT_FAILED(Status))
				break;

			FileSize = File.GetSize32();
			FileBuffer = (PBYTE)AllocateMemoryP(FileSize);
			if (!FileBuffer)
			{
				FileSize = 0;
				Status   = STATUS_NO_MEMORY;
				break;
			}

			File.Read(FileBuffer, FileSize);
		}
		File.Close();
		return Status;
	}
	else
	{
		FileBuffer = NULL;
		FileSize   = 0;
		return XmoeQueryFile(FileName, &FileBuffer, &FileSize);
	}
}


NTSTATUS KaresekaHook::InitKrkrHook(LPCWSTR lpFileName, PVOID Module)
{
	NTSTATUS    Status;
	ULONG_PTR   Length;
	ULONG64     Extension;
	DWORD       ThreadId;
	PVOID       pV2Link;

	LOOP_ONCE
	{
		Status = STATUS_ALREADY_REGISTERED;
		if (Inited == TRUE)
			break;

		Status = STATUS_UNSUCCESSFUL;
		if (Module == NULL)
			break;

		Length = StrLengthW(lpFileName);
		if (Length <= 4)
			break;

		Extension = *(PULONG64)&lpFileName[Length - 4];

		if (Extension != TAG4W('.dll') && Extension != TAG4W('.tpm'))
			break;
		
		pV2Link = Nt_GetProcAddress(Module, "V2Link");
		if (pV2Link == NULL)
			break;

		INLINE_PATCH_DATA f[] =
		{
			{ pV2Link, HookV2Link, (PVOID*)&StubV2Link }
		};

		Status = InlinePatchMemory(f, countof(f));
		Inited = TRUE;
	}
	return Status;
}


HMODULE WINAPI HookLoadLibraryW(LPCWSTR lpLibFileName)
{
	ULONG_PTR     LengthOfName;
	WCHAR         SelfModuleName[MAX_PATH];
	HMODULE       Module;
	KaresekaHook*  Kareseka;


	Kareseka = GetKareseka();
	LengthOfName = StrLengthW(lpLibFileName);

	Module = LoadLibraryW(lpLibFileName);
	Kareseka->InitKrkrHook(lpLibFileName, Module);
	return Module;
}

PVOID NTAPI XmoeAllocateMemory(ULONG_PTR Size)
{
	return AllocateMemoryP(Size);
}

VOID  NTAPI XmoeFreeMemory(PVOID Mem)
{
	FreeMemoryP(Mem);
}

#pragma comment(linker, "/EXPORT:XmoeLinker=_XmoeLinker@0,PRIVATE")
EXTERN_C MY_DLL_EXPORT VOID NTAPI XmoeLinker()
{
}

static BYTE Utf8Bom[3] = { 0xEF, 0xBB, 0xBF };

BOOL KaresekaHook::Init(HMODULE hModule)
{
	NTSTATUS      Status;
	LPCWSTR       Message;
	PVOID         FakeCompiler;
	PVOID         ExeModule, FsModule;
	PBYTE         Buffer;
	ULONG         Size;
	ULONG64       Hash;

	m_SelfModule = hModule;
	ExeModule = Nt_GetExeModuleHandle();

	IAT_PATCH_DATA f[] =
	{
		{ ExeModule, LoadLibraryW,        HookLoadLibraryW,        "Kernel32.dll" },
		{ ExeModule, MultiByteToWideChar, HookMultiByteToWideChar, "Kernel32.dll" }
	};

	LOOP_ONCE
	{
		Status = IATPatchMemory(f, countof(f));
		if (NT_FAILED(Status))
		{
			MessageBoxW(NULL, L"Couldn't patch memory!", L"KrkrUniversalPatch", MB_OK | MB_ICONERROR);
			Ps::ExitProcess(0);
		}

		FsModule = Nt_LoadLibrary(L"KrkrFile.dll");
		if (!FsModule)
			break;

		XmoeInitFileSystem = (API_POINTER(InitFileSystem))Nt_GetProcAddress(FsModule, "XmoeInitFileSystem");
		XmoeQueryFile      = (API_POINTER(::QueryFile))Nt_GetProcAddress(FsModule, "QueryFile");

		if (!XmoeQueryFile || !XmoeQueryFile)
		{
			MessageBoxW(NULL, L"Invalid FileSystem module!", L"KrkrUniversalPatch", MB_OK | MB_ICONERROR);
			break;
		}

		Status = XmoeInitFileSystem(XmoeAllocateMemory, XmoeFreeMemory);
		if (NT_FAILED(Status))
		{
			MessageBoxW(NULL, L"Couldn't initialize filesystem!", L"KrkrUniversalPatch", MB_OK | MB_ICONERROR);
			break;
		}
		FileSystemInited = TRUE;
	}

	auto FileNameToLowerUtf8 = [](LPCSTR FileName)->std::string
	{
		std::string Result;

		for (ULONG i = 0; i < StrLengthA(FileName); i++)
		{
			if (FileName[i] <= 'Z' && FileName[i] >= 'A')
				Result += tolower(FileName[i]);
			else
				Result += FileName[i];
		}
		return Result;
	};


	auto GetLines = [](std::vector<std::string>& ReadPool, PBYTE Buffer, ULONG Size)->BOOL
	{
		std::string ReadLine;
		ULONG       iPos = 0;
		ReadLine.clear();

		LOOP_FOREVER
		{
			if (iPos >= Size)
			break;

			if (Buffer[iPos] == '\r')
			{
				if (!memcmp(ReadLine.c_str(), Utf8Bom, 3))
					ReadLine = &ReadLine[3];

				ReadPool.push_back(ReadLine);
				ReadLine.clear();
				iPos++;

				if (Buffer[iPos] == '\n')
					iPos++;
			}

			if (Buffer[iPos] == '\n')
			{
				if (!memcmp(ReadLine.c_str(), Utf8Bom, 3))
					ReadLine = &ReadLine[3];

				ReadPool.push_back(ReadLine);
				ReadLine.clear();
				iPos++;
			}

			ReadLine += Buffer[iPos];
			iPos++;
		}

			if (ReadLine.length())
			{
				if (!memcmp(ReadLine.c_str(), Utf8Bom, 3))
					ReadLine = &ReadLine[3];

				ReadPool.push_back(ReadLine);
			}

		return TRUE;
	};

	JITList.clear();
	Buffer = NULL;
	Size = 0;

	if (NT_SUCCESS(QueryFile(L"compiler_jit.ini", L"_compiler_jit.ini", Buffer, Size, Hash)))
	{
		std::vector<std::string> ReadPool;

		GetLines(ReadPool, Buffer, Size);
		for (auto& Item : ReadPool)
		{
			WCHAR FileName[MAX_PATH];
			RtlZeroMemory(FileName, sizeof(FileName));

			Item = FileNameToLowerUtf8(Item.c_str());
			MultiByteToWideChar(CP_UTF8, 0, Item.c_str(), Item.length(), FileName, countof(FileName));

			JITList.insert(FileName);
		}
		FreeMemoryP(Buffer);
	}

	if (NT_SUCCESS(QueryFile(L"compiler_text.ini", L"_compiler_text.ini", Buffer, Size, Hash)))
	{
		std::vector<std::string> ReadPool;

		GetLines(ReadPool, Buffer, Size);
		for (auto& Item : ReadPool)
		{
			WCHAR FileName[MAX_PATH];
			RtlZeroMemory(FileName, sizeof(FileName));

			Item = FileNameToLowerUtf8(Item.c_str());
			MultiByteToWideChar(CP_UTF8, 0, Item.c_str(), Item.length(), FileName, countof(FileName));

			TextList.insert(FileName);
		}
		FreeMemoryP(Buffer);
	}

	return NT_SUCCESS(Status);
}

BOOL KaresekaHook::UnInit()
{
	return TRUE;
}

