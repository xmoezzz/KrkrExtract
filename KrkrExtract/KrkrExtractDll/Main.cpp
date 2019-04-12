#include <my.h>
#include <Psapi.h>
#include "hook.h"
#include "KrkrExtract.h"
#include "FakePNG.h"
#include "tp_stub.h"
#include "mt64.h"
#include "MultiThread.h"
#include "XP3Parser.h"
#include "shlwapi.h"

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "zlibstatic.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "jsoncpp.lib")


extern "C" HRESULT NTAPI V2Link(iTVPFunctionExporter *exporter)
{
	TVPInitImportStub(exporter);
	return S_OK;
}

extern "C" MY_DLL_EXPORT HRESULT NTAPI V2Unlink()
{
	return S_OK;
}


ULONG_PTR g_FakeExtractionFilter = NULL;
TVPXP3ArchiveExtractionFilterFunc   g_RealFilter = NULL;
ULONG_PTR                           g_FakeReturnAddress = NULL;

ASM VOID WINAPI FakeExtractionFilterAsm(tTVPXP3ExtractionFilterInfo * /* info */)
{
	INLINE_ASM
	{
		mov      ecx, g_RealFilter;
		jecxz    NO_EXT_FILTER;
		mov      eax, g_FakeReturnAddress;
		mov      [esp], eax;
		jmp      ecx;

	NO_EXT_FILTER:
		ret 4;
	}
}

VOID WINAPI FakeExtractionFilterWithException(tTVPXP3ExtractionFilterInfo *info)
{
	SEH_TRY
	{
		FakeExtractionFilterAsm(info);
	}
	SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

ASM VOID WINAPI FakeExtractionFilter(tTVPXP3ExtractionFilterInfo* /* info */)
{
	INLINE_ASM
	{
		push[esp + 4];
		call g_FakeExtractionFilter;
		ret  4;
	}
}

ULONG_PTR FindReturnAddressWorker()
{
	ULONG        State;
	PBYTE        Buffer;
	GlobalData*  Handle;

	enum { FOUND_NONE, FOUND_C2, FOUND_04, FOUND_00 };

	Handle = GlobalData::GetGlobalData();
	Buffer = (PBYTE)GlobalData::GetGlobalData()->FirstSectionAddress;

	State = FOUND_NONE;
	for (ULONG SizeOfImage = Handle->SizeOfImage; SizeOfImage; ++Buffer, --SizeOfImage)
	{
		ULONG b = Buffer[0];

		switch (State)
		{
		case FOUND_NONE:
			if (b == 0xC2)
				State = FOUND_C2;
			break;

		case FOUND_C2:
			switch (b)
			{
			case 0x04:
				State = FOUND_04;
				break;

			case 0xC2:
				State = FOUND_C2;
				break;

			default:
				State = FOUND_NONE;
			}
			break;

		case FOUND_04:
			switch (b)
			{
			case 0x00:
				State = FOUND_00;
				break;

			case 0xC2:
				State = FOUND_C2;
				break;

			default:
				State = FOUND_NONE;
			}
			break;

		case FOUND_00:
			return (ULONG_PTR)Buffer - 3;
		}
	}
	return NULL;
}

ULONG_PTR FindReturnAddress()
{
	__try
	{
		ULONG_PTR ReturnAddress = FindReturnAddressWorker();

		if (ReturnAddress != NULL)
		{
			g_FakeExtractionFilter = (ULONG_PTR)FakeExtractionFilterAsm;
			return ReturnAddress;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
	return NULL;
}


TVPXP3ArchiveExtractionFilterFunc WINAPI InitFakeFilter(PBOOL Result)
{
	PIMAGE_DOS_HEADER       DosHeader;
	PIMAGE_NT_HEADERS       NtHeader;
	PIMAGE_SECTION_HEADER   SectionHeader;
	GlobalData*             Handle;

	Handle = GlobalData::GetGlobalData();
	g_FakeReturnAddress = FindReturnAddress();
	if (!g_FakeExtractionFilter)
	{
		*Result = FALSE;
		return NULL;
	}

	DosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
	NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);
	SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeader + NtHeader->FileHeader.SizeOfOptionalHeader + sizeof(*NtHeader) - sizeof(NtHeader->OptionalHeader));

	Handle->FirstSectionAddress = SectionHeader->VirtualAddress + (ULONG_PTR)DosHeader;
	Handle->SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
	*Result = TRUE;
	return FakeExtractionFilter;
}


API_POINTER(LoadLibraryA) StubLoadLibraryA = NULL;
API_POINTER(LoadLibraryW) StubLoadLibraryW = NULL;

PVOID WINAPI HookLoadLibraryA(LPCSTR lpFileName)
{
	PVOID   Result;
	PWSTR   UnicodeName;
	ULONG   Length, OutLength;
	
	Length      = (lstrlenA(lpFileName) + 1) * 2;
	UnicodeName = (PWSTR)AllocStack(Length);

	RtlZeroMemory(UnicodeName, Length);
	RtlMultiByteToUnicodeN(UnicodeName, Length, &OutLength, (PSTR)lpFileName, Length / 2 - 1);

	Result = StubLoadLibraryW(UnicodeName);

	//some smc will check self, just ingore.
	if (LookupImportTable(Result, "KERNEL32.dll", "FlushInstructionCache") != IMAGE_INVALID_VA)
		return Result;
	
	GlobalData::GetGlobalData()->InitHook(UnicodeName, Result);
	return Result;
}


PVOID WINAPI HookLoadLibraryW(LPCWSTR lpFileName)
{
	PVOID   Result;

	Result = StubLoadLibraryW(lpFileName);

	//some smc will check self, just ingore.
	if (LookupImportTable(Result, "KERNEL32.dll", "FlushInstructionCache") != IMAGE_INVALID_VA)
		return (HMODULE)Result;

	GlobalData::GetGlobalData()->InitHook(lpFileName, Result);

	return Result;
}

XP3Filter pfTVPSetXP3ArchiveExtractionFilter = nullptr;

void WINAPI HookTVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter filter)
{
	BOOL          Result;
	GlobalData*   Handle;

	Handle = GlobalData::GetGlobalData();
	g_RealFilter = filter;

	tTVPXP3ArchiveExtractionFilter FakeFilter = InitFakeFilter(&Result);
	if (Result)
		InterlockedExchangePointer((PVOID volatile*)&(Handle->pfGlobalXP3Filter), (void*)FakeFilter);
	else
		InterlockedExchangePointer((PVOID volatile*)&(Handle->pfGlobalXP3Filter), (void*)filter);

	if (Handle->DebugOn)
		PrintConsoleW(L"Filter ProcAddr : %p\n", filter);

	pfTVPSetXP3ArchiveExtractionFilter(filter);
}

typedef iTVPFunctionExporter*   (WINAPI *TVPGetFunctionExporterFunc)();
TVPGetFunctionExporterFunc pfTVPGetFunctionExporter = nullptr;

//多次调用的
iTVPFunctionExporter* WINAPI HookTVPGetFunctionExporter()
{
	iTVPFunctionExporter* Result;
	PVOID                 Target;
	GlobalData*           Handle;

	static char funcname[] = "void ::TVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter)";

	Handle = GlobalData::GetGlobalData();
	Result = pfTVPGetFunctionExporter();
	Handle->TVPFunctionExporter = Result;
	
	if (!Handle->Inited)
	{
		InitExporterByExeModule(Result);
		ImageWorkerV2Link(Result);
		InitLayer();
		Handle->FakePngWorkerInited = TRUE;
		
		pfTVPSetXP3ArchiveExtractionFilter = (XP3Filter)TVPGetImportFuncPtr(funcname);
		Target = pfTVPSetXP3ArchiveExtractionFilter;
		
		if (pfTVPSetXP3ArchiveExtractionFilter != nullptr)
		{
			Mp::PATCH_MEMORY_DATA p[] =
			{
				Mp::FunctionJumpVa(Target, HookTVPSetXP3ArchiveExtractionFilter, &pfTVPSetXP3ArchiveExtractionFilter)
			};
			
			Mp::PatchMemory(p, countof(p));
		}
		Handle->InitHookNull();
		Handle->Inited = TRUE;
	}
	return Result;
}


FARPROC WINAPI HookGetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	if (!lstrcmpA(lpProcName, "GetSystemWow64DirectoryA"))
		return NULL;
	
	return GlobalData::GetGlobalData()->StubGetProcAddress(hModule, lpProcName);
}


API_POINTER(CreateFileW) StubCreateFileW = NULL;

HANDLE WINAPI HookCreateFileW(
	LPCWSTR               lpFileName, 
	DWORD                 dwDesiredAccess, 
	DWORD                 dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
	DWORD                 dwCreationDisposition, 
	DWORD                 dwFlagsAndAttributes, 
	HANDLE                hTemplateFile)
{
	wstring     FileName(lpFileName), InternalFileName;
	ULONG_PTR   Index;
	GlobalData* Handle;
	HANDLE      Result;
	MODULEINFO  ModuleInfo;

	Handle = GlobalData::GetGlobalData();

	Index = FileName.find_last_of(L'/');
	if (Index != std::wstring::npos)
		FileName = FileName.substr(Index + 1, std::wstring::npos);

	Index = FileName.find_last_of(L"\\");
	if (Index != std::wstring::npos)
		FileName = FileName.substr(Index + 1, std::wstring::npos);

	RtlZeroMemory(&ModuleInfo, sizeof(MODULEINFO));
	GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(NULL), &ModuleInfo, sizeof(MODULEINFO));
	
	if (IN_RANGE((ULONG_PTR)ModuleInfo.lpBaseOfDll, (ULONG_PTR)_ReturnAddress(), (ULONG_PTR)ModuleInfo.lpBaseOfDll + ModuleInfo.SizeOfImage))
		PrintConsoleW(L"CreateFile : %s\n", lpFileName);

	Result = StubCreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
		lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	//trace this handle??
	if (!lstrcmpiW(FileName.c_str(), Handle->CurrentTempFileName.c_str()) && Result != INVALID_HANDLE_VALUE && Result != 0)
		InterlockedExchangePointer(&(Handle->CurrentTempHandle), Result);

	return Result;
}



BOOL WINAPI CreateProcessW2(
	_In_opt_ LPCWSTR lpApplicationName,
	_Inout_opt_ LPWSTR lpCommandLine,
	_In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
	_In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
	_In_ BOOL bInheritHandles,
	_In_ DWORD dwCreationFlags,
	_In_opt_ LPVOID lpEnvironment,
	_In_opt_ LPCWSTR lpCurrentDirectory,
	_In_ LPSTARTUPINFOW lpStartupInfo,
	_Out_ LPPROCESS_INFORMATION lpProcessInformation)
{
	HANDLE   NewToken;

	NewToken = NULL;
	return GlobalData::GetGlobalData()->StubCreateProcessInternalW(
		NULL,
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation,
		&NewToken);
}


BOOL
WINAPI
HookCreateProcessInternalW(
HANDLE                  hToken,
LPCWSTR                 lpApplicationName,
LPWSTR                  lpCommandLine,
LPSECURITY_ATTRIBUTES   lpProcessAttributes,
LPSECURITY_ATTRIBUTES   lpThreadAttributes,
BOOL                    bInheritHandles,
ULONG                   dwCreationFlags,
LPVOID                  lpEnvironment,
LPCWSTR                 lpCurrentDirectory,
LPSTARTUPINFOW          lpStartupInfo,
LPPROCESS_INFORMATION   lpProcessInformation,
PHANDLE                 phNewToken
)
{
	BOOL             Result, IsSuspended;
	NTSTATUS         Status;
	UNICODE_STRING   FullDllPath;

	RtlInitUnicodeString(&FullDllPath, GlobalData::GetGlobalData()->SelfPath);

	IsSuspended = !!(dwCreationFlags & CREATE_SUSPENDED);
	dwCreationFlags |= CREATE_SUSPENDED;
	Result = GlobalData::GetGlobalData()->StubCreateProcessInternalW(
		hToken,
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation,
		phNewToken);

	if (!Result)
		return Result;

	Status = InjectDllToRemoteProcess(
		lpProcessInformation->hProcess,
		lpProcessInformation->hThread,
		&FullDllPath,
		IsSuspended
	);

	return TRUE;
}


/*
(stub function)
find the 2nd call:

.text:00445ADE                 call    TJS__TJSAllocStringHeap
.text:00445AE3                 push    [ebp+arg_8]
.text:00445AE6                 mov     esi, eax
.text:00445AE8                 push    ebx
.text:00445AE9                 mov     ecx, esi
.text:00445AEB                 call    TJS__tTJSVariantString__SetString (thiscall)

stub:
static void __stdcall TVP_Stub_c95bd66d95c153cdac41b5243e555f5f(tTJSString * _this, const tjs_char * str , int n)
{
::new (_this) tTJSString(str, n);
}


*/


/*
V5.0:
Hook tString constructor


tTJSCriticalSection StringPushCS;

//TJS::tTJSVariantString::SetString
PVOID StubNewStringWithLength = NULL;

Void NTAPI NewStringWithLength_Internal(PCWSTR String, ULONG Length)
{
	tTJSCriticalSectionHolder Holder(StringPushCS);
	BOOL   FindDot;

	if (Length &&
		String &&
		Length <= MAX_PATH )
		//&& IN_RANGE((ULONG_PTR)Nt_GetExeModuleHandle(), (ULONG_PTR)_ReturnAddress(), 0x200000))
	{
		FindDot = FALSE;
		for (ULONG i = 0; i < Length; i++)
		{
			if (String[i] == L'.')
			{
				FindDot = TRUE;
				break;
			}
		}

		PrintConsoleW(L"%s\n", String);

		if (FindDot)
			GlobalData::GetGlobalData()->FileNameList.push_back(std::wstring(String, Length));
	}
}



ASM Void NewStringWithLength()
{
	INLINE_ASM
	{
		push ebp;
		mov  ebp, esp;

		pushad;
		pushfd;
		push dword ptr [ebp + 0xC];
		push dword ptr [ebp + 0x8];
		call NewStringWithLength_Internal;
		popfd;
		popad;
		
		push dword ptr[ebp + 0xC];
		push dword ptr[ebp + 0x8];
		call StubNewStringWithLength;

		pop ebp;
		retn 8;
	}
}


PVOID KrkrExporter = NULL;
BOOL  InitHookPureAsm_Once = FALSE;
Void NTAPI InitHookPureAsm_Internal(iTVPFunctionExporter* Exportor)
{
	PBYTE       TStringConstructorStub;
	GlobalData* Handle;
	ULONG       iPos, OpSize, CallCount;
	ULONG_PTR   AddressOfCallNewString;

	Handle = GlobalData::GetGlobalData();

	if (InitHookPureAsm_Once == FALSE)
	{
		TVPInitImportStub(Exportor);

		//show the window
		Handle->InitHookNull();

		//try to get file name

		static char TStringConstructor[] = "tTJSString::tTJSString(const tjs_char *,int)";

		LOOP_ONCE
		{
			TStringConstructorStub = (PBYTE)TVPGetImportFuncPtr(TStringConstructor);
			if (!TStringConstructorStub)
				break;

			//Analyze internal proc offset then hook it, build a hash map to holder all string
			iPos      = 0;
			CallCount = 0;
			AddressOfCallNewString = 0;
			while (iPos < 0x200 && TStringConstructorStub[iPos] != 0xCC)
			{
				OpSize = GetOpCodeSize32(&TStringConstructorStub[iPos]);
				if (TStringConstructorStub[iPos] == CALL)
				{
					CallCount++;
					if (CallCount == 2)
					{
						AddressOfCallNewString = GetCallDestination((ULONG_PTR)TStringConstructorStub + iPos);
						break;
					}
				}
				iPos += OpSize;
			}

			if (!AddressOfCallNewString)
				break;

			Mp::PATCH_MEMORY_DATA StringPatch[] = 
			{
				Mp::FunctionJumpVa((PVOID)AddressOfCallNewString, NewStringWithLength, &StubNewStringWithLength)
			};

			Mp::PatchMemory(StringPatch, countof(StringPatch));
		}

		InterlockedExchange((LONG volatile *)&InitHookPureAsm_Once, TRUE);
	}
}

PVOID InitHookPureAsm_End = NULL;
ASM Void InitHookPureAsm()
{
	INLINE_ASM
	{
		mov eax, KrkrExporter;

		pushad;
		pushfd;

		push eax;
		call InitHookPureAsm_Internal

		popfd;
		popad;
		mov eax, KrkrExporter;
	}
}

*/

NTSTATUS WINAPI InitHook()
{
	NtFileDisk            File;
	BOOL                  Success;
	NTSTATUS              Status;
	PVOID                 Kernel32Handle, Target;
	GlobalData*           Handle;
	PVOID                 ExporterPointer;
	ULONG                 Size;
	ULONG64               Crc;
	PIMAGE_DOS_HEADER     DosHeader;
	PIMAGE_NT_HEADERS32   NtHeader;
	PIMAGE_SECTION_HEADER SectionHeader;
	PBYTE                 CurrentSection;
	ULONG_PTR             CurrentSectionSize;
	ULONG_PTR             CurrentCodePtr, CodeSize;

	Handle = GlobalData::GetGlobalData();
	Kernel32Handle = Nt_LoadLibrary(L"KERNEL32.dll");


	PVOID hModule = GetModuleHandleW(NULL);
	*(FARPROC *)&pfTVPGetFunctionExporter = (FARPROC)Nt_GetProcAddress(hModule, "TVPGetFunctionExporter");

	//Krkr2 & normal Krkrz Module
	if (pfTVPGetFunctionExporter != nullptr)
	{
		Target = pfTVPGetFunctionExporter;
		Mp::PATCH_MEMORY_DATA p[] =
		{
			Mp::FunctionJumpVa(Target, HookTVPGetFunctionExporter, &pfTVPGetFunctionExporter)
		};

		if (Handle->DebugOn)
			PrintConsoleW(L"Normal init...\n");

		Mp::PatchMemory(p, countof(p));
	}
	else
	{
		Success = FALSE;

		if (!Success)
		{
			//Nt_DeleteFile(L"KrkrExtract.db");

			Mp::PATCH_MEMORY_DATA f[] =
			{
				Mp::FunctionJumpVa(LoadLibraryA, HookLoadLibraryA, &StubLoadLibraryA),
				Mp::FunctionJumpVa(LoadLibraryW, HookLoadLibraryW, &StubLoadLibraryW)
			};

			if (Handle->DebugOn)
				PrintConsoleW(L"Special init...\n");

			Mp::PatchMemory(f, countof(f));
		}

		Mp::PATCH_MEMORY_DATA CommonPatch[] =
		{
			Mp::FunctionJumpVa(CreateFileW, HookCreateFileW, &StubCreateFileW)
		};

		Mp::PatchMemory(CommonPatch, countof(CommonPatch));
	}

	*(PVOID*)&Handle->StubCreateProcessInternalW = Nt_GetProcAddress(Kernel32Handle, "CreateProcessInternalW");
	Target = Handle->StubCreateProcessInternalW;

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(Target, HookCreateProcessInternalW, &Handle->StubCreateProcessInternalW)
	};

	Mp::PatchMemory(f, countof(f));

	//module path check
	if (Nt_CurrentPeb()->OSMajorVersion > 6 || (Nt_CurrentPeb()->OSMajorVersion == 6 && Nt_CurrentPeb()->OSMinorVersion >= 2))
	{
		Mp::PATCH_MEMORY_DATA ooxx[] =
		{
			Mp::FunctionJumpVa(GetProcAddress, HookGetProcAddress, &Handle->StubGetProcAddress)
		};

		Mp::PatchMemory(ooxx, countof(ooxx));
	}

	return STATUS_SUCCESS;
}


API_POINTER(MultiByteToWideChar) StubMultiByteToWideChar = NULL;
INT NTAPI HookMultiByteToWideChar(
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
		StubMultiByteToWideChar(
		CodePage,
		dwFlags,
		lpMultiByteStr,
		cbMultiByte,
		lpWideCharStr,
		cchWideChar
		);
}


VOID FASTCALL EnumerateFileInDirectory(LPCWSTR szPath, std::vector<std::wstring>& FileList)
{
	WIN32_FIND_DATAW FindFileData;
	HANDLE           hListFile;
	WCHAR            FilePath[MAX_PATH];
	NtFileDisk       File;
	NTSTATUS         Status;

	wsprintfW(FilePath, L"%s\\*.*", szPath);
	hListFile = FindFirstFile(FilePath, &FindFileData);

	auto IsXp3File = [](HANDLE hFile)->BOOL
	{
		BOOL          Success;
		LARGE_INTEGER OldOffset, NewOffset, Size;
		BYTE          Mark[0x1000];
		BYTE          Buffer[0x10000], *Xp3Signature;
		DWORD         BytesRead;

		static BYTE   XP3Header[] = { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 };

		if (hFile == 0 || hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		OldOffset.QuadPart = 0;
		NewOffset.QuadPart = 0;
		SetFilePointerEx(hFile, NewOffset, &OldOffset, FILE_CURRENT);
		GetFileSizeEx(hFile, &Size);
		NewOffset.QuadPart = 0;
		Success = FALSE;
		SetFilePointerEx(hFile, NewOffset, NULL, FILE_BEGIN);
		if (Size.QuadPart > sizeof(StaticXP3V2Magic))
		{
			if (Size.QuadPart >= 0x1000)
			{
				ReadFile(hFile, Mark, sizeof(Mark), NULL, NULL);
				if (RtlCompareMemory(Mark, StaticXP3V2Magic, sizeof(StaticXP3V2Magic)) == sizeof(StaticXP3V2Magic))
				{
					Success = TRUE;
				}
				else
				{
					LOOP_ONCE
					{
						if (((PIMAGE_DOS_HEADER)Mark)->e_magic != IMAGE_DOS_SIGNATURE)
						break;

						SetFilePointer(hFile, 0x10, NULL, FILE_BEGIN);
						for (LONG64 FileSize = Size.QuadPart; FileSize > 0; FileSize -= sizeof(Buffer))
						{
							ReadFile(hFile, Buffer, sizeof(Buffer), &BytesRead, NULL);

							if (BytesRead < 0x10)
								break;

							Xp3Signature = Buffer;
							for (ULONG_PTR Count = sizeof(Buffer) / 0x10; Count; Xp3Signature += 0x10, --Count)
							{
								if (RtlCompareMemory(Xp3Signature, XP3Header, sizeof(XP3Header)) == sizeof(XP3Header))
								{
									Success = TRUE;
									break;
								}
							}
						}
					}
				}
			}
			else
			{
				ReadFile(hFile, Mark, sizeof(StaticXP3V2Magic), NULL, NULL);
				if (RtlCompareMemory(Mark, StaticXP3V2Magic, sizeof(StaticXP3V2Magic)) == sizeof(StaticXP3V2Magic))
					Success = TRUE;
			}
		}
		SetFilePointerEx(hFile, OldOffset, NULL, FILE_BEGIN);
		return Success;
	};


	if (hListFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(lstrcmpW(FindFileData.cFileName, L".") == 0 ||
			   lstrcmpW(FindFileData.cFileName, L"..") == 0)
			{
				continue;
			}

			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				wsprintfW(FilePath, L"%s\\%s", szPath, FindFileData.cFileName);
				EnumerateFileInDirectory(FilePath, FileList);
			}
			else
			{
				wsprintfW(FilePath, L"%s\\%s", szPath, FindFileData.cFileName);
				Status = File.Open(FilePath);
				if(NT_SUCCESS(Status) && IsXp3File(File.GetHandle()))
					FileList.push_back(FilePath);

				File.Close();
			}

		} while (FindNextFile(hListFile, &FindFileData));
		FindClose(hListFile);
	}
}

VOID DetectCxdecAndInitEntry();

//TVP(KIRIKIRI) Z core / Scripting Platform for Win32
//TVP(KIRIKIRI) 2 core / Scripting Platform for Win32

BOOL NTAPI InitKrkrExtract(HMODULE hModule)
{
	BOOL            Success;
	GlobalData*     Handle;
	vector<wstring> FileList;
	WCHAR           ExeDirectory[MAX_PATH];
	wstring         CurFileName;
	ULONG_PTR       Index, Index2;

	static WCHAR Pattern[] = L"TVP(KIRIKIRI) Z core / Scripting Platform for Win32";

	//or search entrypoint???
	auto CheckKrkrZ = [&]()->NTSTATUS
	{
		NTSTATUS   Status;
		NtFileDisk File;
		ULONG_PTR  FileSize;
		PBYTE      FileBuffer;
		WCHAR      FileName[MAX_PATH];

		Handle = GlobalData::GetGlobalData();

		RtlZeroMemory(FileName,   countof(FileName) * sizeof(FileName[0]));
		GetModuleFileNameW(GetModuleHandleW(NULL), FileName,  MAX_PATH * 2);

		LOOP_ONCE
		{
			Status = File.Open(FileName);
			if (NT_FAILED(Status))
				break;

			Status     = STATUS_INSUFFICIENT_RESOURCES;
			FileSize   = File.GetSize32();
			FileBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FileSize);
			
			if (!FileBuffer)
				break;

			File.Read(FileBuffer, FileSize);
			if (Handle->FindCodeSlow((PCCHAR)FileBuffer, FileSize, (PCHAR)Pattern, lstrlenW(Pattern) * 2))
			{
				//ensure the codepage
				Mp::PATCH_MEMORY_DATA f[] =
				{
					Mp::FunctionJumpVa(MultiByteToWideChar, HookMultiByteToWideChar, &StubMultiByteToWideChar)
				};

				Mp::PatchMemory(f, countof(f));

				DetectCxdecAndInitEntry();
				Handle->ModuleType = ModuleVersion::Krkrz;
			}
			HeapFree(GetProcessHeap(), 0, FileBuffer);
			FileBuffer = NULL;
		}
		File.Close();
		return Status;
	};

	LOOP_ONCE
	{
		Success = FALSE;
		Handle  = GlobalData::GetGlobalData();
		if (!Handle)
			break;

		//Handle->DebugOn = TRUE;
		DeleteFileW(L"KrkrExtractFailed.txt");
		
		Handle->SetDllModule(hModule);
		
		//insert all xp3 files, including exe
		RtlZeroMemory(ExeDirectory, sizeof(ExeDirectory));
		Nt_GetExeDirectory(ExeDirectory, countof(ExeDirectory));

		Success = NT_FAILED(CheckKrkrZ()) ? FALSE : TRUE;
		Success = NT_FAILED(InitHook())   ? FALSE : TRUE;
	}

	return Success;
}

ULONG64 MurmurHash64B(const void * key, int len, ULONG seed = 0xEE6B27EB)
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	unsigned int h1 = seed ^ len;
	unsigned int h2 = 0;

	const unsigned int * data = (const unsigned int *)key;

	while (len >= 8)
	{
		unsigned int k1 = *data++;
		k1 *= m; k1 ^= k1 >> r; k1 *= m;
		h1 *= m; h1 ^= k1;
		len -= 4;

		unsigned int k2 = *data++;
		k2 *= m; k2 ^= k2 >> r; k2 *= m;
		h2 *= m; h2 ^= k2;
		len -= 4;
	}

	if (len >= 4)
	{
		unsigned int k1 = *data++;
		k1 *= m; k1 ^= k1 >> r; k1 *= m;
		h1 *= m; h1 ^= k1;
		len -= 4;
	}

	switch (len)
	{
	case 3: h2 ^= ((unsigned char*)data)[2] << 16;
	case 2: h2 ^= ((unsigned char*)data)[1] << 8;
	case 1: h2 ^= ((unsigned char*)data)[0];
		h2 *= m;
	};

	h1 ^= h2 >> 18; h1 *= m;
	h2 ^= h1 >> 22; h2 *= m;
	h1 ^= h2 >> 17; h1 *= m;
	h2 ^= h1 >> 19; h2 *= m;

	unsigned long long h = h1;

	h = (h << 32) | h2;

	return h;
}

#include "zlib.h"


BOOL IsEncryptedSenrenBanka(PBYTE pDecompress, ULONG Size, NtFileDisk& File)
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
	BOOL                    RawFailed, TotalFailed;

	Handle = GlobalData::GetGlobalData();

	RtlCopyMemory(&CompressedIndex, pDecompress, sizeof(KRKRZ_COMPRESSED_INDEX));
	File.Seek(CompressedIndex.Offset.LowPart, FILE_BEGIN);

	iPos = 0;
	DecompSize       = CompressedIndex.DecompressedSize;
	IndexBuffer      = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedIndex.DecompressedSize);
	CompressedBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedIndex.CompressedSize);

	if (!IndexBuffer || !CompressedBuffer)
	{
		if (IndexBuffer)      HeapFree(GetProcessHeap(), 0, IndexBuffer);
		if (CompressedBuffer) HeapFree(GetProcessHeap(), 0, CompressedBuffer);
		IndexBuffer      = NULL;
		CompressedBuffer = NULL;
		return FALSE;
	}

	File.Read(CompressedBuffer, CompressedIndex.CompressedSize);

	RawFailed = FALSE;
	if ((DecompSize = uncompress((PBYTE)IndexBuffer, (PULONG)&DecompSize,
		(PBYTE)CompressedBuffer, CompressedIndex.CompressedSize)) != Z_OK)
	{
		RawFailed = TRUE;
	}

	HeapFree(GetProcessHeap(), 0, IndexBuffer);
	HeapFree(GetProcessHeap(), 0, CompressedBuffer);

	return RawFailed;
}

VOID AddToSenrenBankaEntry(LPCWSTR lpFileName, PBYTE pDecompress, ULONG Size, NtFileDisk& File)
{
	KRKRZ_COMPRESSED_INDEX  CompressedIndex;
	PBYTE                   CompressedBuffer;
	ULONG                   DecompSize, iPos;
	XP3Index                Item;
	ULARGE_INTEGER          ChunkSize;
	USHORT                  NameLength;
	GlobalData*             Handle;
	BOOL                    RawFailed, TotalFailed;

	Handle = GlobalData::GetGlobalData();

	RtlCopyMemory(&CompressedIndex, pDecompress, sizeof(KRKRZ_COMPRESSED_INDEX));
	File.Seek(CompressedIndex.Offset.LowPart, FILE_BEGIN);

	iPos = 0;
	DecompSize = CompressedIndex.DecompressedSize;
	CompressedBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CompressedIndex.CompressedSize);

	if (!CompressedBuffer)
		return;
	
	File.Read(CompressedBuffer, CompressedIndex.CompressedSize);

	MemEntry Entry;
	Entry.Buffer = CompressedBuffer;
	Entry.Size = CompressedIndex.CompressedSize;
	Entry.Hash = MurmurHash64B(Entry.Buffer, Entry.Size);

	Handle->SpecialChunkMap.insert(std::make_pair(Entry.Hash, Entry));
	Handle->SpecialChunkMapBySize.insert(std::make_pair(Entry.Size, Entry));

	PrintConsoleW(L"Entry for [%s] was added\n", lpFileName);
}

VOID GetSenrenBankaEntry(LPCWSTR lpFileName)
{
	NTSTATUS                Status;
	NtFileDisk              File;
	ULONG                   Count;
	KRKR2_XP3_HEADER        XP3Header;
	KRKR2_XP3_DATA_HEADER   DataHeader;
	PBYTE                   Indexdata;
	LARGE_INTEGER           BeginOffset, Offset;
	CHAR                    M2ChunkInfo[8];
	GlobalData*             Handle;

	Handle = GlobalData::GetGlobalData();

	Status = File.Open(lpFileName);
	if (NT_FAILED(Status))
		return;

	Count = 0;
	BeginOffset.QuadPart = 0;
	Status = File.Read(&XP3Header, sizeof(XP3Header));
	RtlZeroMemory(M2ChunkInfo, sizeof(M2ChunkInfo));

	if (NT_FAILED(Status))
	{
		File.Close();
		return;
	}

	Status = STATUS_UNSUCCESSFUL;

	if (RtlCompareMemory(StaticXP3V2Magic, XP3Header.Magic, sizeof(StaticXP3V2Magic)) != sizeof(StaticXP3V2Magic))
		return;
	

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
		File.Seek(Offset, FILE_BEGIN);
		if (NT_FAILED(File.Read(&DataHeader, sizeof(DataHeader))))
		{
			if (Handle->DebugOn)
				PrintConsoleW(L"Couldn't Read Index Header\n");

			File.Close();
			HeapFree(GetProcessHeap(), 0, pCompress);
			HeapFree(GetProcessHeap(), 0, pDecompress);
			return;
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
		}

		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"Index Size %08x\n", (ULONG32)DataHeader.OriginalSize.LowPart);

		if (IsCompatXP3(pDecompress, DataHeader.OriginalSize.LowPart, &Handle->M2ChunkMagic))
		{
			FindChunkMagicFirst(pDecompress, DataHeader.OriginalSize.LowPart);

			CopyMemory(M2ChunkInfo, &Handle->M2ChunkMagic, 4);
			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleA("Chunk : %s\n", M2ChunkInfo);

			switch (DetectCompressedChunk(pDecompress, DataHeader.OriginalSize.LowPart))
			{
			case FALSE:

				PrintConsoleW(L"Pre-init : read [%s] special entry.\n", lpFileName);
				AddToSenrenBankaEntry(lpFileName, pDecompress, DataHeader.OriginalSize.LowPart, File);
				break;
			}
		}

	} while (DataHeader.bZlib & 0x80);

	File.Close();
	HeapFree(GetProcessHeap(), 0, pCompress);
	HeapFree(GetProcessHeap(), 0, pDecompress);
}

VOID ReadSenrenBanka()
{
	WIN32_FIND_DATAW          FileInfo;
	HANDLE                    FileHandle;
	NtFileDisk                File;
	std::vector<std::wstring> ArchiveList;
	
	FileHandle = FindFirstFileW(L"*.xp3", &FileInfo);
	if (FileHandle == 0 || FileHandle == INVALID_HANDLE_VALUE)
		return;

	do
	{
		PrintConsoleW(L"Pre init : found archive : %s\n", FileInfo.cFileName);
		ArchiveList.push_back(FileInfo.cFileName);
	} while (FindNextFileW(FileHandle, &FileInfo));

	for (auto& Entry : ArchiveList)
	{
		GetSenrenBankaEntry(Entry.c_str());
	}
	
	FindClose(FileHandle);
}


class HardwareBreakpoint
{
public:
	HardwareBreakpoint() { m_index = -1; }
	~HardwareBreakpoint() { Clear(); }

	// The enum values correspond to the values used by the Intel Pentium,
	// so don't change them!
	enum Condition { Write = 1, Read /* or write! */ = 3 };

	void Set(void* address, int len /* 1, 2, or 4 */, Condition when)
	{
		CONTEXT cxt;
		HANDLE thisThread = GetCurrentThread();

		switch (len)
		{
		case 1: len = 0; break;
		case 2: len = 1; break;
		case 4: len = 3; break;
		}

		// The only registers we care about are the debug registers
		cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;

		// Read the register values
		GetThreadContext(thisThread, &cxt);

		// Find an available hardware register
		for (m_index = 0; m_index < 4; ++m_index)
		{
			if ((cxt.Dr7 & (1 << (m_index * 2))) == 0)
				break;
		}

		switch (m_index)
		{
		case 0: cxt.Dr0 = (DWORD)address; break;
		case 1: cxt.Dr1 = (DWORD)address; break;
		case 2: cxt.Dr2 = (DWORD)address; break;
		case 3: cxt.Dr3 = (DWORD)address; break;
		}

		SetBits(cxt.Dr7, 16 + (m_index * 4), 2, when);
		SetBits(cxt.Dr7, 18 + (m_index * 4), 2, len);
		SetBits(cxt.Dr7, m_index * 2, 1, 1);

		// Write out the new debug registers
		SetThreadContext(thisThread, &cxt);
	}
	void Clear(PCONTEXT Context)
	{
		SetBits(Context->Dr7, m_index * 2, 1, 0);
		Context->Dr0 = 0;
		Context->Dr1 = 0;
		Context->Dr2 = 0;
		Context->Dr3 = 0;
		m_index = -1;
	}

	void Clear()
	{
		if (m_index != -1)
		{
			CONTEXT cxt;
			HANDLE thisThread = GetCurrentThread();

			// The only registers we care about are the debug registers
			cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;

			// Read the register values
			GetThreadContext(thisThread, &cxt);

			SetBits(cxt.Dr7, m_index * 2, 1, 0);

			// Write out the new debug registers
			SetThreadContext(thisThread, &cxt);

			m_index = -1;
		}
	}

protected:

	inline void SetBits(unsigned long& dw, int lowBit, int bits, int newValue)
	{
		int mask = (1 << bits) - 1; // e.g. 1 becomes 0001, 2 becomes 0011, 3 becomes 0111

		dw = (dw & ~(mask << lowBit)) | (newValue << lowBit);
	}

	int m_index; // -1 means not set; 0-3 means we've set that hardware bp
};

API_POINTER(ReadFile) StubReadFile = NULL;
HardwareBreakpoint   BreakPoint;
BOOL                 BreakOnce = FALSE;
ULONG                RetAddr = 0;
PVOID ExceptionHandler = NULL;

#define SET_DEBUG_REGISTER_FLAG 0x0000000

LONG NTAPI FindPrivateProcHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
	NTSTATUS    Status;
	GlobalData* Handle;
	DWORD       OldFlags;
	PVOID       ModuleBase;
	PDR7_INFO   Dr7;
	ULONG       iPos, OpSize, RealRetAddr;
	BOOL        FindRet;
	DWORD       PreviousProtect;

	Handle = GlobalData::GetGlobalData();
	
	PrintConsoleW(L"FindPrivateProcHandler : %08x -> %08x\n", pExceptionInfo->ExceptionRecord->ExceptionCode,
		pExceptionInfo->ContextRecord->Eip);

	if (BreakOnce == FALSE && pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
	{
		BreakPoint.Clear(pExceptionInfo->ContextRecord);
		pExceptionInfo->ContextRecord->ContextFlags |= CONTEXT_DEBUG_REGISTERS | SET_DEBUG_REGISTER_FLAG;
		
		///get call proc
		PrintConsoleW(L"eip %08x\n", pExceptionInfo->ContextRecord->Eip);
		ModuleBase = GetImageBaseAddress((PVOID)pExceptionInfo->ContextRecord->Eip);
		if (ModuleBase == NULL)
		{
			/// i will check current process memory in user land.
			/// dump all hidden modules and find the target one.

			PrintConsoleW(L"0ops...failed to get module base?\n");
			PrintConsoleW(L"->launch memory search engine...\n");
		}
		else
		{
			PrintConsoleW(L"ahhh...I FOUND YOUR HIDDEN MODULE -> %08x\n", ModuleBase);
			LdrAddRefDll(GET_MODULE_HANDLE_EX_FLAG_PIN, ModuleBase);
		}

		///[!!!] find retn
		///because ebp will be overwrote...
		iPos = pExceptionInfo->ContextRecord->Eip;
		FindRet = FALSE;


		/// [+] the best method: create sym cache
		while (iPos < pExceptionInfo->ContextRecord->Eip + 0x300)
		{
			ULONG OpSize = GetOpCodeSize32((PVOID)iPos);
			if (OpSize == 0 || OpSize == -1)
				break;

			if (OpSize == 1 && ((PBYTE)iPos)[0] == 0xC3)
			{
				FindRet = TRUE;
				RetAddr = iPos;

				BYTE Break = 0xCC;

				VirtualProtect((PVOID)RetAddr, 1, PAGE_EXECUTE_READWRITE, &PreviousProtect);
				WriteProcessMemory(GetCurrentProcess(), (LPVOID)RetAddr, &Break, 1, NULL);
				VirtualProtect((PVOID)RetAddr, 1, PreviousProtect, &PreviousProtect);
				break;
			}

			iPos += OpSize;
		}
		
		BreakOnce = TRUE;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	else if (RetAddr != 0 && pExceptionInfo->ContextRecord->Eip == RetAddr && pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT)
	{
		BYTE Break = 0xC3;

		VirtualProtect((PVOID)RetAddr, 1, PAGE_EXECUTE_READWRITE, &PreviousProtect);
		WriteProcessMemory(GetCurrentProcess(), (LPVOID)RetAddr, &Break, 1, NULL);
		VirtualProtect((PVOID)RetAddr, 1, PreviousProtect, &PreviousProtect);

		RealRetAddr = ((PDWORD)(pExceptionInfo->ContextRecord->Esp))[0];
		PrintConsoleW(L"ret addr : %08x\n", RealRetAddr);

		///validate this call first
		Handle->SpecialChunkDecoder = (GlobalData::SpecialChunkDecoderFunc)GetCallDestination(RealRetAddr - 5);
		PrintConsoleW(L"ok, I GOT YOUR ROUTINE -> %p\n", Handle->SpecialChunkDecoder);

		RetAddr = 0;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}


#include "SectionProtector.h"

BOOL SetBpOnce = FALSE;
RTL_CRITICAL_SECTION ReadFileLocker;

BOOL
WINAPI
HookReadFile(
 HANDLE hFile,
 LPVOID lpBuffer,
 DWORD nNumberOfBytesToRead,
 LPDWORD lpNumberOfBytesRead,
 LPOVERLAPPED lpOverlapped
)
{
	BOOL        Success;
	ULONG64     Hash;
	GlobalData* Handle;

	Handle = GlobalData::GetGlobalData();

	SectionProtector<PRTL_CRITICAL_SECTION> AutoLocker(&ReadFileLocker);

	Success = StubReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
	auto Entry = Handle->SpecialChunkMapBySize.find(nNumberOfBytesToRead);

	if (Success && Entry != Handle->SpecialChunkMapBySize.end() && SetBpOnce == FALSE)
	{
		Hash = MurmurHash64B(lpBuffer, nNumberOfBytesToRead);
		if (Hash == Entry->second.Hash) //also check name
		{
			SetBpOnce = TRUE;
			//hw bp
			ExceptionHandler = AddVectoredExceptionHandler(1, FindPrivateProcHandler);
			if (!ExceptionHandler)
				PrintConsoleW(L"Try to added handler, reason = %u\n", GetLastError());
			else
				PrintConsoleW(L"Exception was added.\n");

			BreakPoint.Set(lpBuffer, 1, HardwareBreakpoint::Condition::Write);
			PrintConsoleW(L"set bp : ok\n");
		}
	}
	return Success;
}


VOID DetectCxdecAndInitEntry()
{
	NTSTATUS                Status;
	NtFileDisk              File;
	ULONG                   Count;
	KRKR2_XP3_HEADER        XP3Header;
	KRKR2_XP3_DATA_HEADER   DataHeader;
	PBYTE                   Indexdata;
	LARGE_INTEGER           BeginOffset, Offset;
	CHAR                    M2ChunkInfo[8];
	GlobalData*             Handle;

	Handle = GlobalData::GetGlobalData();

	//AllocConsole();
	Status = File.Open("data.xp3");
	if (NT_FAILED(Status))
		return; 

	Count = 0;
	BeginOffset.QuadPart = 0;
	Status = File.Read(&XP3Header, sizeof(XP3Header));
	RtlZeroMemory(M2ChunkInfo, sizeof(M2ChunkInfo));

	if (NT_FAILED(Status))
	{
		File.Close();
		return;
	}

	Status = STATUS_UNSUCCESSFUL;

	if (RtlCompareMemory(StaticXP3V2Magic, XP3Header.Magic, sizeof(StaticXP3V2Magic)) != sizeof(StaticXP3V2Magic))
	{
		if (Handle->DebugOn)
			PrintConsoleW(L"No a XP3 Package!\n");

		return;
	}

	ULONG64 CompresseBufferSize = 0x1000;
	ULONG64 DecompressBufferSize = 0x1000;
	PBYTE pCompress   = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (ULONG)CompresseBufferSize);
	PBYTE pDecompress = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (ULONG)DecompressBufferSize);
	DataHeader.OriginalSize = XP3Header.IndexOffset;


	BOOL Result = FALSE;
	do
	{
		Offset.QuadPart = DataHeader.OriginalSize.QuadPart + BeginOffset.QuadPart;
		File.Seek(Offset, FILE_BEGIN);
		if (NT_FAILED(File.Read(&DataHeader, sizeof(DataHeader))))
		{
			if (Handle->DebugOn)
				PrintConsoleW(L"Couldn't Read Index Header\n");

			File.Close();
			HeapFree(GetProcessHeap(), 0, pCompress);
			HeapFree(GetProcessHeap(), 0, pDecompress);
			pCompress   = NULL;
			pDecompress = NULL;
			return;
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
		}

		if (IsCompatXP3(pDecompress, DataHeader.OriginalSize.LowPart, &Handle->M2ChunkMagic))
		{
			FindChunkMagicFirst(pDecompress, DataHeader.OriginalSize.LowPart);

			CopyMemory(M2ChunkInfo, &Handle->M2ChunkMagic, 4);
			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleA("Chunk : %s\n", M2ChunkInfo);

			switch (DetectCompressedChunk(pDecompress, DataHeader.OriginalSize.LowPart))
			{
			case FALSE:
				
				PrintConsoleW(L"Pre-init : found Special chunk.\n");

				Result = IsEncryptedSenrenBanka(pDecompress, DataHeader.OriginalSize.LowPart, File);
				if (Result)
				{
					Handle->IsSpcialChunkEncrypted = TRUE;
					PrintConsoleW(L"Pre-init : Special chunk is encryped.\n");
					ReadSenrenBanka();

					StubReadFile = ReadFile;
					Mp::PATCH_MEMORY_DATA fun[] = 
					{
						Mp::FunctionJumpVa( ReadFile, HookReadFile, &StubReadFile)
					};

					InitializeCriticalSection(&ReadFileLocker);
					Mp::PatchMemory(fun, countof(fun));
				}
				break;
			}
		}

	} while (DataHeader.bZlib & 0x80);

	File.Close();
	HeapFree(GetProcessHeap(), 0, pCompress);
	HeapFree(GetProcessHeap(), 0, pDecompress);
}

VOID InitRand(HMODULE hModule)
{
	ULONG64  Seeds[4];
	WCHAR    Path[MAX_PATH];

	RtlZeroMemory(Path, countof(Path) * 2);
	Nt_GetExeDirectory(Path, MAX_PATH);
	
	Seeds[0] = MakeQword(GetCurrentProcessId(),     GetCurrentThreadId());
	Seeds[1] = MakeQword(Nt_CurrentPeb()->ProcessHeap, Nt_CurrentTeb()->EnvironmentPointer);
	Seeds[2] = MurmurHash64B(Path, lstrlenW(Path) * 2);
	Seeds[3] = MakeQword(hModule, GetModuleHandleW(NULL));

	init_by_array64(Seeds, countof(Seeds));
}

#include "MapMarshal.h"
#include "capstone/capstone.h"

#pragma comment(lib, "capstone.lib")


auto KrkrDBG_DisasmOneInstruction(PBYTE OpCode, ULONG_PTR Size, ULONG_PTR CurrentAddress, PSTR Buffer, ULONG ccBuffer)->NTSTATUS
{
	NTSTATUS                     Status;
	csh                          CapstoneHandle;
	cs_insn*                     CapstoneInsn;
	SIZE_T                       Count;
	
	Status = STATUS_UNSUCCESSFUL;
	LOOP_ONCE
	{
		if (cs_open(CS_ARCH_X86, CS_MODE_32, &CapstoneHandle) != CS_ERR_OK)
			break;

		if (IsBadReadPtr(OpCode, Size))
			break;

		Count = cs_disasm(CapstoneHandle, OpCode, Size, CurrentAddress, 0, &CapstoneInsn);
		if (Count <= 0)
			break;

		_snprintf(Buffer, ccBuffer, "%s %s", CapstoneInsn[0].mnemonic, CapstoneInsn[0].op_str);

		cs_free(CapstoneInsn, Count);
		cs_close(&CapstoneHandle);
		Status = STATUS_SUCCESS;
	};
	return Status;
};

auto KrkrDBG_CreatePeDatabase(LPCWSTR FileName, LPCWSTR DBName)->NTSTATUS
{
	NTSTATUS                     Status;
	HANDLE                       hFile;
	ULONG                        CurrentAddress, CurrentCodeSize, Count;
	PE_DATABASE                  Header;
	PIMAGE_SECTION_HEADER        SectionHeader;
	PIMAGE_NT_HEADERS            NtHeader;
	CHAR                         DisasmLine[100];
	CMarshalMap                  DisObj;
	std::map<ULONG, CMarshalMap> MapList;
	PBYTE                        pvFile;
	LARGE_INTEGER                Size;
	NtFileDisk                   File;
	ULONG                        AllocSize;
	STATSTG                      Stat;
	LARGE_INTEGER                Offset;
	PBYTE                        Buffer;
	IStream*                     Stream;
	csh                          CapstoneHandle;
	cs_insn*                     CapstoneInsn;


	if (FileName == NULL || DBName == NULL)
		return STATUS_INVALID_PARAMETER;

	hFile = StubCreateFileW(
		FileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (hFile == 0 || hFile == INVALID_HANDLE_VALUE)
		return STATUS_NO_SUCH_FILE;
	
	GetFileSizeEx(hFile, &Size);

	HANDLE hMapFile = CreateFileMappingW(
		hFile,
		NULL,
		PAGE_READONLY,
		Size.HighPart,
		Size.LowPart,
		NULL);

	if (hMapFile == 0 || hMapFile == INVALID_HANDLE_VALUE)
		return STATUS_NO_SUCH_FILE;

	pvFile = (PBYTE)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
	if (pvFile == NULL)
	{
		NtClose(hMapFile);
		NtClose(hFile);
		return STATUS_MAPPED_FILE_SIZE_ZERO;
	}

	Header.Crc32      = 0;
	Header.EntryCount = 0;
	Header.Magic      = TAG4('Xmoe');

	NtHeader = ImageNtHeaders(pvFile);
	SectionHeader = IMAGE_FIRST_SECTION(NtHeader);

	for (DWORD i = 0; i < NtHeader->FileHeader.NumberOfSections; i++, SectionHeader++)
	{
		if (FLAG_ON(SectionHeader->Characteristics, IMAGE_SCN_CNT_CODE) ||
			FLAG_ON(SectionHeader->Characteristics, IMAGE_SCN_MEM_EXECUTE))
		{
			CurrentAddress  = NtHeader->OptionalHeader.ImageBase + SectionHeader->VirtualAddress;
			CurrentCodeSize = SectionHeader->Misc.VirtualSize;

			if (cs_open(CS_ARCH_X86, CS_MODE_32, &CapstoneHandle) != CS_ERR_OK)
				continue;

			Count = cs_disasm(CapstoneHandle, SectionHeader->PointerToRawData + pvFile, CurrentCodeSize, CurrentAddress, 0, &CapstoneInsn);
			if (Count <= 0)
				continue;

			Header.EntryCount += Count;
			for (ULONG j = 0; j < Count; j++)
			{
				wsprintfA(DisasmLine, "%s %s", CapstoneInsn[j].mnemonic, CapstoneInsn[j].op_str);

				if (j == 0)
					DisObj.PrevLength = 0;
				else
					DisObj.PrevLength = CapstoneInsn[j - 1].size;
				
				DisObj.Offset    = CapstoneInsn[j].address;
				DisObj.InsLength = CapstoneInsn[j].size;
				lstrcpyA(DisObj.DisasmLine, DisasmLine);
				MapList[CapstoneInsn[j].address] = DisObj;
			}

			cs_free(CapstoneInsn, Count);
			cs_close(&CapstoneHandle);
		}
	}

	UnmapViewOfFile(pvFile);
	NtClose(hMapFile);
	NtClose(hFile); 


	Status = File.Create(DBName);
	if (NT_FAILED(Status))
		return Status;

	File.Write(&Header, sizeof(Header));

	AllocSize = 0x1000;
	Buffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, AllocSize);
	if (Buffer == NULL)
	{
		File.Close();
		return STATUS_NO_MEMORY;
	}

	for (auto Disasm : MapList)
	{
		Stream = Disasm.second.Marshal();
		if (Stream == NULL)
			continue;

		Stream->Stat(&Stat, STATFLAG_DEFAULT);
		Offset.QuadPart = 0;
		Stream->Seek(Offset, FILE_BEGIN, 0);

		if (Stat.cbSize.QuadPart > AllocSize)
		{
			AllocSize = Stat.cbSize.QuadPart;
			Buffer = (PBYTE)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Buffer, AllocSize);
			if (Buffer == NULL)
			{
				File.Close();
				return STATUS_NO_MEMORY;
			}
		}

		Stream->Read(Buffer, Stat.cbSize.QuadPart, NULL);
		File.Write(Buffer, Stat.cbSize.QuadPart);
		Stream->Release();
	}
	HeapFree(GetProcessHeap(), 0, Buffer);
	File.Close();

	return STATUS_SUCCESS;
};


auto KrkrDBG_ReadPeDatabase(std::map<ULONG, CMarshalMap>& MapList, LPCWSTR DBName)->NTSTATUS
{
	HANDLE             hFile, hMapFile;
	LARGE_INTEGER      Size;
	PPE_DATABASE       Header;
	PBYTE              pvFile;
	ULONG              Offset;
	CMarshalMap        DisObj;
	IStream*           Stream;

	MapList.clear();

	hFile = StubCreateFileW(
		DBName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (hFile == 0 || hFile == INVALID_HANDLE_VALUE)
		return STATUS_NO_SUCH_FILE;
	
	GetFileSizeEx(hFile, &Size);

	hMapFile = CreateFileMappingW(
		hFile,
		NULL,
		PAGE_READONLY,
		Size.HighPart,
		Size.LowPart,
		NULL);

	if (hMapFile == 0 || hMapFile == INVALID_HANDLE_VALUE)
		return STATUS_NO_SUCH_FILE;

	pvFile = (PBYTE)MapViewOfFile(hMapFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	if (pvFile == NULL)
	{
		NtClose(hMapFile);
		NtClose(hFile);
		return STATUS_MAPPED_FILE_SIZE_ZERO;
	}

	Header = (PPE_DATABASE)pvFile;
	Offset = sizeof(*Header);

	for (ULONG i = 0; i < Header->EntryCount; i++)
	{
		Stream = SHCreateMemStream(pvFile + Offset, *(PDWORD)(pvFile + Offset));
		if (!Stream)
		{
			//clean-up
			UnmapViewOfFile(pvFile);
			NtClose(hMapFile);
			NtClose(hFile);
		}
		DisObj.UnMarshal(Stream);
		MapList[DisObj.Offset] = DisObj;
	}

	UnmapViewOfFile(pvFile);
	NtClose(hMapFile);
	NtClose(hFile);
	return STATUS_SUCCESS;
}

//allocate memory by caller!
auto KrkrDBG_ReadPrevInstruction(const std::map<ULONG, CMarshalMap>& Holder, ULONG CurrentOffset, PCHAR Buffer, ULONG ccBuffer)->NTSTATUS
{
	NTSTATUS   Status;
	ULONG      PreviousOffset;
	
	const auto Instruction = Holder.find(CurrentOffset);
	if (Instruction == Holder.end())
		return STATUS_UNSUCCESSFUL;

	PreviousOffset = Instruction->second.Offset - Instruction->second.PrevLength;
	const auto PrevInstruction = Holder.find(PreviousOffset);
	if (PrevInstruction == Holder.end())
		return STATUS_UNSUCCESSFUL;

	RtlZeroMemory(Buffer, ccBuffer);
	RtlCopyMemory(Buffer, PrevInstruction->second.DisasmLine, lstrlenA(PrevInstruction->second.DisasmLine));

	return STATUS_SUCCESS;
}


//allocate memory by caller!
auto KrkrDBG_ReadInstruction(const std::map<ULONG, CMarshalMap>& Holder, ULONG CurrentOffset, PCHAR Buffer, ULONG ccBuffer)->NTSTATUS
{
	ULONG      PreviousOffset;

	const auto Instruction = Holder.find(CurrentOffset);
	if (Instruction == Holder.end())
		return STATUS_UNSUCCESSFUL;

	RtlZeroMemory(Buffer, ccBuffer);
	RtlCopyMemory(Buffer, Instruction->second.DisasmLine, lstrlenA(Instruction->second.DisasmLine));

	return STATUS_SUCCESS;
}

#include <dbghelp.h>

NTSTATUS CreateMiniDump2(PEXCEPTION_POINTERS ExceptionPointers, LPCWSTR ModuleBaseName)
{
	WCHAR                          MiniDumpFile[MAX_PATH];
	NtFileDisk                     File;
	BOOL                           Success;
	NTSTATUS                       Status;
	MINIDUMP_EXCEPTION_INFORMATION ExceptionInformation;


	wsprintfW(MiniDumpFile, L"%s.dmp", ModuleBaseName);

	Status = File.Create(MiniDumpFile);
	if (NT_FAILED(Status))
		return Status;

	ExceptionInformation.ClientPointers = FALSE;
	ExceptionInformation.ExceptionPointers = ExceptionPointers;
	ExceptionInformation.ThreadId = GetCurrentThreadId();

	Success = MiniDumpWriteDump(
		NtCurrentProcess(),
		(ULONG_PTR)Nt_CurrentTeb()->ClientId.UniqueProcess,
		File,
		MiniDumpNormal,
		&ExceptionInformation,
		NULL,
		NULL
		);

	File.Close();
	return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


BOOL CALLBACK EnumerateModuleCallBack(PCSTR ModuleName, DWORD ModuleBase, ULONG ModuleSize, PVOID UserContext)
{
	SymLoadModule(GetCurrentProcess(),
		NULL,
		ModuleName,
		0,
		ModuleBase,
		ModuleSize);

	return TRUE;
}


struct ModuleTrace
{
	DWORD64 CurrentAddress;
	DWORD64 BaseAddress;
	string  ModuleName;
	wstring FullModulePath;
};


///[+] Check code here carefully!!!
LONG NTAPI KrkrUnhandledExceptionFilter(_EXCEPTION_POINTERS *ExceptionPointer)
{
	NTSTATUS              Status;
	NtFileDisk            File;
	ULONG                 Size;
	ULONG                 Count;
	STACKFRAME64          StackFrame;
	DWORD64               ModuleBase;
	IMAGEHLP_MODULEW      ImageModule;
	CHAR                  ModuleNameA[MAX_PATH];
	CHAR                  DisasmLine[400];
	WCHAR                 CurModuleName[MAX_PATH];
	INT                   Length;
	BOOL                  Success;
	ModuleTrace           FullTrace[64];
	FILE*                 DebugWriter;

	static WCHAR ExceptionInfo[] = L"KrkrExtract crashed...\n"
		                           L"Mini dump will be generated, pls send this file to developer\n";


	//[+] init debug environment
	//1. If data cache not exist, disassemble the executable file FROM DISK
	//2. load data cache to binary search tree. (we can marshal this tree into binary file)
	//3. stack trace. (get the previous instruction from binary search tree)
	//4. write to file

	//some bugs in DbgHelp.dll
	//M$ forget to set LastError value in some APIs...
	SymInitializeW(GetCurrentProcess(), NULL, TRUE);
	EnumerateLoadedModules(GetCurrentProcess(), EnumerateModuleCallBack, NULL);

	//Create mini dump first
	RtlZeroMemory(CurModuleName, sizeof(CurModuleName));
	GetModuleFileNameW(GetModuleHandleW(NULL), CurModuleName, countof(CurModuleName) - 1);
	CreateMiniDump2(ExceptionPointer, CurModuleName);


	RtlZeroMemory(&StackFrame, sizeof(StackFrame));

	StackFrame.AddrPC.Offset    = ExceptionPointer->ContextRecord->Eip;
	StackFrame.AddrFrame.Offset = ExceptionPointer->ContextRecord->Ebp;
	StackFrame.AddrStack.Offset = ExceptionPointer->ContextRecord->Esp;
	StackFrame.AddrPC.Mode      = AddrModeFlat;
	StackFrame.AddrFrame.Mode   = AddrModeFlat;
	StackFrame.AddrStack.Mode   = AddrModeFlat;

	Count = 0;

	LOOP_ONCE
	{
		while (StackWalk64(IMAGE_FILE_MACHINE_I386,
			GetCurrentProcess(),
			GetCurrentThread(),
			&StackFrame,
			ExceptionPointer->ContextRecord,
			NULL,
			&SymFunctionTableAccess64,
			&SymGetModuleBase64,
			NULL) &&
			Count < countof(FullTrace))
		{
			ModuleBase = SymGetModuleBase(GetCurrentProcess(), (DWORD)StackFrame.AddrPC.Offset);

			FullTrace[Count].BaseAddress    = ModuleBase;
			FullTrace[Count].CurrentAddress = StackFrame.AddrPC.Offset;

			if (!ModuleBase)
			{
				FullTrace[Count].ModuleName = "???";
				continue;
			}

			RtlZeroMemory(&ImageModule, sizeof(ImageModule));
			ImageModule.SizeOfStruct = sizeof(ImageModule);

			Success = SymGetModuleInfoW(GetCurrentProcess(), (DWORD)ModuleBase, &ImageModule);

			if (Success && ImageModule.BaseOfImage != NULL)
			{
				LPCWSTR lpImageName = wcsrchr(ImageModule.ImageName, L'\\');
				if (lpImageName)
					lpImageName += 1;
				else
					lpImageName = ImageModule.ImageName;

				Length = WideCharToMultiByte(CP_ACP, 0, lpImageName, lstrlenW(lpImageName), ModuleNameA, countof(ModuleNameA) - 1, NULL, NULL);
				ModuleNameA[Length] = 0;

				FullTrace[Count].ModuleName     = ModuleNameA;
				FullTrace[Count].FullModulePath = ImageModule.ImageName;
			}
			else
			{
				FullTrace[Count].ModuleName = "???";
			}
			
			Count++;
		}

		LOOP_ONCE
		{
			DebugWriter = fopen("KrkrExtract.crash.log", "wb");
			if (!DebugWriter)
				break;

			//some disassembler stuffs here
			for (ULONG i = 0; i < Count; i++)
			{
				Status = KrkrDBG_DisasmOneInstruction((PBYTE)FullTrace[i].CurrentAddress,
					GetOpCodeSize32((PBYTE)FullTrace[i].CurrentAddress),
					FullTrace[i].BaseAddress,
					DisasmLine,
					countof(DisasmLine) - 1);

				fprintf(DebugWriter, "%08x %s (%s)\r\n", FullTrace[i].CurrentAddress, DisasmLine, FullTrace[i].ModuleName.c_str());
			}
			fclose(DebugWriter);
		}
	};

	//[-] modified
	//ExceptionBox(ExceptionInfo, L"KrkrExtract Unhandled Exception");
	//Ps::ExitProcess(ExceptionPointer->ExceptionRecord->ExceptionCode);
	MessageBoxW(NULL, L"KrkrExtract Unhandled Exception", L"KrkrExtract", MB_OK | MB_ICONERROR);
	
	//[+] handled by system
	//[+] DONT handle the exception because KrkrExtract may can extract archive now
	return ExceptionContinueExecution;
}


#include "DebuggerHandler.h"


DWORD NTAPI ConsoleMoniterThread(PVOID Param)
{
	BOOL   Success;
	DWORD  nRead;
	static WCHAR CommandLine[4096];
	
	LOOP_FOREVER
	{
		RtlZeroMemory(CommandLine, sizeof(CommandLine));
		Success = ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), CommandLine, sizeof(CommandLine) - 1, &nRead, NULL);
		if (!Success)
			goto NEXT_READ;

		if (nRead >= sizeof(CommandLine))
			PrintConsoleA("Debugger : Command line is too long\n");
		else if (nRead)
			ParseCommand(CommandLine);
			
	NEXT_READ:
		Sleep(10);
	}
	return 0;
}

BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		//AllocConsole();
		LdrDisableThreadCalloutsForDll(hModule);
		InitRand(hModule);

		GlobalData::GetGlobalData();

		//should be the last one to be handled
		//AddVectoredExceptionHandler(FALSE, KrkrUnhandledExceptionFilter);

		Nt_CreateThread(ConsoleMoniterThread);
		
		if (!InitKrkrExtract(hModule))
		{
			ExceptionBox(L"Internal exception!");
			ExitProcess(-1);
		}

	break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

