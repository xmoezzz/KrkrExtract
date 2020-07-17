#pragma comment(linker, "/SECTION:.text,ERW /MERGE:.rdata=.text /MERGE:.data=.text")
#pragma comment(linker, "/SECTION:.Xmoe,ERW /MERGE:.text=.Xmoe")

#include "my.h"
#include "KrkrExtract.h"
#include "MyHook.h"
#include "FakePNG.h"
#include "tp_stub.h"
#include "mt64.h"

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "ntdll.lib")

EXTERN_C MY_DLL_EXPORT HRESULT NTAPI V2Link(iTVPFunctionExporter *exporter)
{
	TVPInitImportStub(exporter);
	return S_OK;
}

EXTERN_C MY_DLL_EXPORT HRESULT NTAPI V2Unlink()
{
	return S_OK;
}


ULONG_PTR g_FakeExtractionFilter = NULL;
TVPXP3ArchiveExtractionFilterFunc   g_RealFilter = NULL;
ULONG_PTR                           g_FakeReturnAddress = NULL;

ASM Void WINAPI FakeExtractionFilterAsm(tTVPXP3ExtractionFilterInfo * /* info */)
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

Void WINAPI FakeExtractionFilterWithException(tTVPXP3ExtractionFilterInfo *info)
{
	__try
	{
		FakeExtractionFilterAsm(info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

ASM Void WINAPI FakeExtractionFilter(tTVPXP3ExtractionFilterInfo* /* info */)
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

	ULONG     OldProtect;
	ULONG_PTR ReturnAddress = (ULONG_PTR)Nt_GetModuleHandle(NULL) + 0xFE0;

	ProtectVirtualMemory((PVOID)ReturnAddress, 1, PAGE_READONLY, &OldProtect);
	g_FakeExtractionFilter = (ULONG_PTR)FakeExtractionFilterWithException;

	return ReturnAddress;
}


TVPXP3ArchiveExtractionFilterFunc WINAPI InitFakeFilter()
{
	g_FakeReturnAddress = FindReturnAddress();
	return FakeExtractionFilter;
}


PVOID WINAPI HookLoadLibraryA(LPCSTR lpFileName)
{
	PVOID   Result;
	PWSTR   UnicodeName;
	ULONG   Length, OutLength;
	
	Length      = (StrLengthA(lpFileName) + 1) * 2;
	UnicodeName = (PWSTR)AllocStack(Length);

	RtlZeroMemory(UnicodeName, Length);
	RtlMultiByteToUnicodeN(UnicodeName, Length, &OutLength, lpFileName, Length / 2 - 1);

	Result = Nt_LoadLibrary(UnicodeName);
	if (LookupImportTable(Result, "KERNEL32.dll", "FlushInstructionCache") != IMAGE_INVALID_VA)
		return Result;
	
	GlobalData::GetGlobalData()->InitHook(UnicodeName, Result);
	return Result;
}


PVOID WINAPI HookLoadLibraryW(LPCWSTR lpFileName)
{
	PVOID   Result;

	Result = Nt_LoadLibrary((PWSTR)lpFileName);

	if (LookupImportTable(Result, "KERNEL32.dll", "FlushInstructionCache") != IMAGE_INVALID_VA)
		return (HMODULE)Result;

	GlobalData::GetGlobalData()->InitHook(lpFileName, Result);

	return Result;
}

XP3Filter pfTVPSetXP3ArchiveExtractionFilter = nullptr;

void WINAPI HookTVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter filter)
{
	GlobalData*   Handle;

	Handle = GlobalData::GetGlobalData();
	g_RealFilter = filter;

	tTVPXP3ArchiveExtractionFilter FakeFilter = InitFakeFilter();
	_InterlockedExchangePointer((void* volatile *)&(GlobalData::GetGlobalData()->pfGlobalXP3Filter), (void*)FakeFilter);

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
		
		if (pfTVPSetXP3ArchiveExtractionFilter != nullptr)
		{
			Target = pfTVPSetXP3ArchiveExtractionFilter;
			INLINE_PATCH_DATA p[] = 
			{
				{ Target, HookTVPSetXP3ArchiveExtractionFilter, (PVOID*)&pfTVPSetXP3ArchiveExtractionFilter }
			};
			
			InlinePatchMemory(p, countof(p));
		}
		Handle->InitHookNull();
		Handle->Inited = TRUE;
	}
	return Result;
}


FARPROC WINAPI HookGetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	if (!StrCompareA(lpProcName, "GetSystemWow64DirectoryA"))
		return NULL;
	
	return GlobalData::GetGlobalData()->StubGetProcAddress(hModule, lpProcName);
}



HANDLE WINAPI HookCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	wstring     FileName(lpFileName), InternalFileName;
	ULONG_PTR   Index;
	GlobalData* Handle;
	HANDLE      Result;

	Handle = GlobalData::GetGlobalData();

	Index = FileName.find_last_of(L'/');
	if (Index != std::wstring::npos)
		FileName = FileName.substr(Index + 1, std::wstring::npos);

	Index = FileName.find_last_of(L"\\");
	if (Index != std::wstring::npos)
		FileName = FileName.substr(Index + 1, std::wstring::npos);

	Result = CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
		lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (Handle->CurrentTempFileName.find_last_of(L".xp3") != std::wstring::npos)
		InternalFileName = Handle->CurrentTempFileName;
	else
		InternalFileName = L"KrkrzTempWorker.xp3";

	if (!StrICompareW(FileName.c_str(), InternalFileName.c_str(), StrCmp_ToLower) && Result != INVALID_HANDLE_VALUE)
		InterlockedExchangePointer(&(Handle->CurrentTempHandle), Result);

	return Result;
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

NTSTATUS WINAPI InitHook()
{
	NTSTATUS     Status;
	PVOID        Kernel32Handle, Target;
	GlobalData*  Handle;
	
	Handle = GlobalData::GetGlobalData();
	Kernel32Handle = Nt_LoadLibrary(L"KERNEL32.dll");

	PVOID hModule = Nt_GetExeModuleHandle();
	*(FARPROC *)&pfTVPGetFunctionExporter = (FARPROC)Nt_GetProcAddress(hModule, "TVPGetFunctionExporter");

	//Krkr2 & normal Krkrz Module
	if (pfTVPGetFunctionExporter != nullptr)
	{
		Target = pfTVPGetFunctionExporter;
		INLINE_PATCH_DATA p[] = 
		{
			{ Target, HookTVPGetFunctionExporter, (PVOID*)&pfTVPGetFunctionExporter }
		};

		if (Handle->DebugOn)
			PrintConsoleW(L"Normal init...\n");

		InlinePatchMemory(p, countof(p));
	}
	else
	{
		IAT_PATCH_DATA f[] =
		{
			{ hModule, LoadLibraryA, HookLoadLibraryA, "KERNEL32.dll" },
			{ hModule, LoadLibraryW, HookLoadLibraryW, "KERNEL32.dll" },
			{ hModule, CreateFileW,  HookCreateFileW,  "KERNEL32.dll" }
		};

		if (Handle->DebugOn)
			PrintConsoleW(L"Special init...\n");

		IATPatchMemory(f, countof(f));
	}
	
	Target = Nt_LoadLibrary(L"KERNELBASE.DLL");
	if (Target)
	{
		Handle->StubCreateProcessInternalW =
			(API_POINTER(CreateProcessInternalW))Nt_GetProcAddress(Target, "CreateProcessInternalW");
		Target = Handle->StubCreateProcessInternalW;
	}
	else
	{
		Handle->StubCreateProcessInternalW =
			(API_POINTER(CreateProcessInternalW))Nt_GetProcAddress(Kernel32Handle, "CreateProcessInternalW");
		Target = Handle->StubCreateProcessInternalW;
	}
	
	INLINE_PATCH_DATA f[] = 
	{
		{ Target, HookCreateProcessInternalW, (PVOID*)&(Handle->StubCreateProcessInternalW) }
	};
	
	InlinePatchMemory(f, countof(f));

	if (Nt_CurrentPeb()->OSMajorVersion > 6 || (Nt_CurrentPeb()->OSMajorVersion == 6 && Nt_CurrentPeb()->OSMinorVersion >= 2))
	{
		INLINE_PATCH_DATA ooxx[] =
		{
			{ GetProcAddress, HookGetProcAddress, (PVOID*)&(Handle->StubGetProcAddress) }
		};
		
		InlinePatchMemory(ooxx, countof(ooxx));
	}
	
	return STATUS_SUCCESS;
}



INT WINAPI HookMultiByteToWideChar(
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


//TVP(KIRIKIRI) Z core / Scripting Platform for Win32
//TVP(KIRIKIRI) 2 core / Scripting Platform for Win32

BOOL NTAPI InitKrkrExtract(HMODULE hModule)
{
	BOOL          Success;
	GlobalData*   Handle;

	static WCHAR Pattern[] = L"TVP(KIRIKIRI) Z core / Scripting Platform for Win32";

	auto CheckKrkrZ = [&]()->NTSTATUS
	{
		NTSTATUS   Status;
		NtFileDisk File;
		ULONG_PTR  FileSize;
		PBYTE      FileBuffer;
		WCHAR      FileName[MAX_PATH];

		Handle = GlobalData::GetGlobalData();

		RtlZeroMemory(FileName,   countof(FileName) * sizeof(FileName[0]));
		Nt_GetModuleFileName(Nt_GetExeModuleHandle(), FileName,  MAX_PATH * 2);

		LOOP_ONCE
		{
			Status = File.Open(FileName);
			if (NT_FAILED(Status))
				break;

			Status     = STATUS_INSUFFICIENT_RESOURCES;
			FileSize   = File.GetSize32();
			FileBuffer = (PBYTE)AllocateMemoryP(FileSize);
			
			if (!FileBuffer)
				break;

			File.Read(FileBuffer, FileSize);
			if (Handle->FindCodeSlow((PCChar)FileBuffer, FileSize, (PCHAR)Pattern, StrLengthW(Pattern) * 2))
			{

				IAT_PATCH_DATA f[] = 
				{
					{ Nt_GetExeModuleHandle(), MultiByteToWideChar, HookMultiByteToWideChar, "KERNEL32.dll" }
				};

				IATPatchMemory(f, countof(f));

				Handle->ModuleType = ModuleVersion::Krkrz;
			}
			FreeMemoryP(FileBuffer);
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
		Io::DeleteFileW(L"KrkrExtractFailed.txt");
		
		Handle->SetDllModule(hModule);
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

Void InitRand(HMODULE hModule)
{
	ULONG64  Seeds[4];
	WCHAR    Path[MAX_PATH];

	RtlZeroMemory(Path, countof(Path) * 2);
	Nt_GetExeDirectory(Path, MAX_PATH);
	
	Seeds[0] = MakeQword(Nt_GetCurrentProcessId(),     Nt_GetCurrentThreadId());
	Seeds[1] = MakeQword(Nt_CurrentPeb()->ProcessHeap, Nt_CurrentTeb()->EnvironmentPointer);
	Seeds[2] = MurmurHash64B(Path, StrLengthW(Path) * 2);
	Seeds[3] = MakeQword(hModule, Nt_GetExeModuleHandle());

	init_by_array64(Seeds, countof(Seeds));
}



BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);
	
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		//AllocConsole();
		ml::MlInitialize();
		LdrDisableThreadCalloutsForDll(hModule);
		InitRand(hModule);
		GlobalData::GetGlobalData();
		//MessageBox(NULL, 0,0,0);
		//__asm int 3;
		if (!InitKrkrExtract(hModule))
		{
			MessageBoxW(NULL, L"Internal exception!", L"FATAL", MB_OK);
			Ps::ExitProcess(-1);
		}
	break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

