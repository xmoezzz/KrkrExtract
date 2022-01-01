#include "KrkrExtract.h"
#include "tp_stub.h"
#include "Decoder.h"
#include "Prototype.h"
#include "Helper.h"
#include <Psapi.h>
#include "magic_enum.hpp"

INT
NTAPI 
XeMultiByteToWideChar(
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

	return KrkrExtractCore::GetInstance()->m_MultiByteToWideChar(
		CodePage,
		dwFlags,
		lpMultiByteStr,
		cbMultiByte,
		lpWideCharStr,
		cchWideChar
	);
}



PVOID
NTAPI
XeLoadLibraryA(LPCSTR FileName)
{
	PVOID            Module;
	PWSTR            UnicodeName;
	ULONG            Length, OutLength;

	Length      = (StrLengthA(FileName) + 1) * 2;
	UnicodeName = (PWSTR)AllocStack(Length);

	RtlZeroMemory(UnicodeName, Length);
	RtlMultiByteToUnicodeN(UnicodeName, Length, &OutLength, (PSTR)FileName, Length / 2 - 1);

	Module = Nt_LoadLibrary(UnicodeName);
	if (!Module)
		return Module;

	if (LookupImportTable(Module, "KERNEL32.dll", "FlushInstructionCache") != IMAGE_INVALID_VA)
		return Module;

	KrkrExtractCore::GetInstance()->InitHookWithDll(UnicodeName, Module);
	return Module;
}


PVOID
NTAPI
XeLoadLibraryW(LPWSTR FileName)
{
	PVOID            Module;

	Module = Nt_LoadLibrary(FileName);

	if (!Module)
		return NULL;

	// don't motify the decoding-plugin
	if (LookupImportTable(Module, "KERNEL32.dll", "FlushInstructionCache") != IMAGE_INVALID_VA)
		return Module;

	KrkrExtractCore::GetInstance()->InitHookWithDll(FileName, Module);
	return Module;
}

FARPROC
WINAPI
XeGetProcAddress(HMODULE Module, LPCSTR ProcName)
{
	if (!ProcName)
		return NULL;

	if ((ULONG)ProcName <= 0xFFFF)
		return (FARPROC)Nt_GetProcAddress(Module, ProcName);

	if (!lstrcmpA(ProcName, "GetSystemWow64DirectoryA"))
		return NULL;

	return (FARPROC)Nt_GetProcAddress(Module, ProcName);
}



HANDLE 
WINAPI
XeCreateFileW(
	LPCWSTR               lpFileName,
	DWORD                 dwDesiredAccess,
	DWORD                 dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD                 dwCreationDisposition,
	DWORD                 dwFlagsAndAttributes,
	HANDLE                hTemplateFile)
{
	HANDLE           FileHandle;
	MODULEINFO       ModuleInfo;

	auto&& FileName = GetPackageName(lpFileName);

	RtlZeroMemory(&ModuleInfo, sizeof(MODULEINFO));
	GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(NULL), &ModuleInfo, sizeof(MODULEINFO));

	if (IN_RANGE(
		(ULONG_PTR)ModuleInfo.lpBaseOfDll,
		(ULONG_PTR)_ReturnAddress(),
		(ULONG_PTR)ModuleInfo.lpBaseOfDll + ModuleInfo.SizeOfImage)) {
		//PrintConsoleW(L"CreateFile : %s\n", lpFileName);
	}

	FileHandle = KrkrExtractCore::GetInstance()->m_CreateFileW(
		lpFileName, 
		dwDesiredAccess, 
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition, 
		dwFlagsAndAttributes, 
		hTemplateFile
	);

	//trace this handle??
	if (!lstrcmpiW(
		FileName.c_str(), 
		KrkrExtractCore::GetInstance()->GetCurrentTempFileName()) && FileHandle != INVALID_HANDLE_VALUE) {
		KrkrExtractCore::GetInstance()->SetCurrentTempHandle(FileHandle);
	}

	return FileHandle;
}



BOOL
WINAPI
XeCreateProcessInternalW(
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
	BOOL             Success, IsSuspended;
	NTSTATUS         Status;
	UNICODE_STRING   FullDllPath;

	RtlInitUnicodeString(&FullDllPath, KrkrExtractCore::GetInstance()->m_DllPath.c_str());

	IsSuspended = !!(dwCreationFlags & CREATE_SUSPENDED);
	dwCreationFlags |= CREATE_SUSPENDED;
	Success = KrkrExtractCore::GetInstance()->m_CreateProcessInternalW(
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

	if (!Success)
		return Success;

	Status = InjectDllToRemoteProcess(
		lpProcessInformation->hProcess,
		lpProcessInformation->hThread,
		&FullDllPath,
		IsSuspended
	);

	return NT_SUCCESS(Status);
}


HRESULT WINAPI XeV2Link(iTVPFunctionExporter *Exporter)
{
	NTSTATUS              Status;
	Prototype::V2LinkFunc V2LinkStub;

	Status = KrkrExtractCore::GetInstance()->InitExporterWithDll(Exporter);
	if (NT_FAILED(Status)) 
	{
		PrintConsoleW(L"XeV2Link : InitExporterWithDll failed %08x", Status);

		V2LinkStub = KrkrExtractCore::GetInstance()->GetV2Link();
		return V2LinkStub(Exporter);
	}

	ImageWorkerV2Link(Exporter);
	KrkrExtractCore::GetInstance()->SetTVPFunctionExporter(Exporter);
	InitLayer(KrkrExtractCore::GetInstance());

	V2LinkStub = KrkrExtractCore::GetInstance()->GetV2Link();
	return V2LinkStub(Exporter);
}


BOOL WINAPI XeIsDBCSLeadByte(BYTE TestChar)
{
	return IsDBCSLeadByteEx(932, TestChar);
}


LONG NTAPI FindPrivateProcHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	PVOID             ModuleBase;
	ULONG            iPos, RealRetAddr;
	BOOL               FindRet;
	DWORD           PreviousProtect;

	PrintConsoleW(
		L"FindPrivateProcHandler : %08x -> %08x\n", 
		ExceptionInfo->ExceptionRecord->ExceptionCode,
		ExceptionInfo->ContextRecord->Eip
	);

	if (KrkrExtractCore::GetInstance()->m_BreakOnce == FALSE && ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
	{
		KrkrExtractCore::GetInstance()->RemoveAllHwBreakPoint(ExceptionInfo->ContextRecord);
		ExceptionInfo->ContextRecord->ContextFlags |= CONTEXT_DEBUG_REGISTERS;

		//
		// get call routine
		//

		PrintConsoleW(L"eip %08x\n", ExceptionInfo->ContextRecord->Eip);
		ModuleBase = GetImageBaseAddress((PVOID)ExceptionInfo->ContextRecord->Eip);
		if (ModuleBase == NULL)
		{
			//
			// check current process memory in user land.
			// dump all hidden modules and find the target one.
			//

			PrintConsoleW(L"0ops...failed to get module base?\n");
			PrintConsoleW(L"->launch memory search engine...\n");
		}
		else
		{
			PrintConsoleW(L"ahhh...I FOUND YOUR HIDDEN MODULE -> %08x\n", ModuleBase);
			LdrAddRefDll(GET_MODULE_HANDLE_EX_FLAG_PIN, ModuleBase);
		}

		//
		// found retn
		// because ebp will be overwrote...
		//

		iPos = ExceptionInfo->ContextRecord->Eip;
		FindRet = FALSE;


		//
		// create symbol cache?
		//

		while (iPos < ExceptionInfo->ContextRecord->Eip + 0x300)
		{
			ULONG OpSize = GetOpCodeSize32((PVOID)iPos);
			if (OpSize == 0 || OpSize == -1)
				break;

			if (OpSize == 1 && ((PBYTE)iPos)[0] == 0xC3)
			{
				FindRet = TRUE;
				KrkrExtractCore::GetInstance()->m_RetAddr = iPos;

				BYTE Break = 0xCC;

				Nt_WriteProtectMemory(NtCurrentProcess(), (PVOID)KrkrExtractCore::GetInstance()->m_RetAddr, &Break, sizeof(Break));
				break;
			}

			iPos += OpSize;
		}

		KrkrExtractCore::GetInstance()->m_BreakOnce = TRUE;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	else if (
		KrkrExtractCore::GetInstance()->m_RetAddr != 0 &&
		ExceptionInfo->ContextRecord->Eip == KrkrExtractCore::GetInstance()->m_RetAddr &&
		ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT
		)
	{
		BYTE Break = 0xC3;

		Nt_WriteProtectMemory(NtCurrentProcess(), (PVOID)KrkrExtractCore::GetInstance()->m_RetAddr, &Break, sizeof(Break));

		RealRetAddr = ((PDWORD)(ExceptionInfo->ContextRecord->Esp))[0];
		PrintConsoleW(L"ret addr : %08x\n", RealRetAddr);

		//
		// validate this call first
		//

		KrkrExtractCore::GetInstance()->SetSpecialChunkDecoder((Prototype::SpecialChunkDecoderFunc)GetCallDestination(RealRetAddr - 5));
		PrintConsoleW(L"ok, I GOT YOUR ROUTINE -> %p\n", KrkrExtractCore::GetInstance()->GetSpecialChunkDecoder());

		KrkrExtractCore::GetInstance()->m_RetAddr = 0;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}



BOOL
WINAPI
XeReadFile
(
	HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped
)
{
	NTSTATUS         Status;
	ULONG            RemoveStatus;
	BOOL             Success;
	ULONG64          Hash;
	INT              Index;

	SectionProtector AutoLocker(&KrkrExtractCore::GetInstance()->m_ReadFileLocker);

	Success = KrkrExtractCore::GetInstance()->m_ReadFile(
		hFile, 
		lpBuffer, 
		nNumberOfBytesToRead, 
		lpNumberOfBytesRead, 
		lpOverlapped
	);

	if (KrkrExtractCore::GetInstance()->GetSpecialChunkDecoder() && KrkrExtractCore::GetInstance()->m_ExceptionHandler)
	{
		RemoveStatus = RtlRemoveVectoredExceptionHandler(KrkrExtractCore::GetInstance()->m_ExceptionHandler);
		if (RemoveStatus) 
		{
			PrintConsoleW(L"XeReadFile : RtlRemoveVectoredExceptionHandler ok\n");
			KrkrExtractCore::GetInstance()->m_ExceptionHandler = nullptr;
		}
		else {
			PrintConsoleW(L"XeReadFile : RtlRemoveVectoredExceptionHandler failed, LastError = %d\n", GetLastError());
		}

		return Success;
	}

	auto Entry = KrkrExtractCore::GetInstance()->m_SpecialChunkMapBySize.find(nNumberOfBytesToRead);

	if (Success && Entry != KrkrExtractCore::GetInstance()->m_SpecialChunkMapBySize.end() && KrkrExtractCore::GetInstance()->m_SetBpOnce == FALSE)
	{
		Hash = MurmurHash64B(lpBuffer, nNumberOfBytesToRead);

		//
		// also check name
		//

		if (Hash == Entry->second.Hash) 
		{
			KrkrExtractCore::GetInstance()->m_SetBpOnce = TRUE;

			// add handler to vectored exception list
			KrkrExtractCore::GetInstance()->m_ExceptionHandler = RtlAddVectoredExceptionHandler(1, FindPrivateProcHandler);

			if (!KrkrExtractCore::GetInstance()->m_ExceptionHandler) {
				PrintConsoleW(L"Try to added handler, reason = %u\n", GetLastError());
			}
			else {
				PrintConsoleW(L"Exception was added.\n");
			}

			// set hardware breakpoint
			Index = -1;
			Status = KrkrExtractCore::GetInstance()->SetHwBreakPoint(lpBuffer, 1, HardwareBreakpoint::Condition::Write, Index);
			if (NT_FAILED(Status)) {
				PrintConsoleW(L"XeReadFile : SetHwBreakPointAt failed, %08x\n", Status);
			}
			else {
				PrintConsoleW(L"XeReadFile : SetHwBreakPointAt ok, Index = %d\n", Index);
			}
		}
	}
	return Success;
}



BOOL
NTAPI
XeIsDebuggerPresent()
{
	return FALSE;
}



static ULONG_PTR g_FakeExtractionFilter = NULL;
static ULONG_PTR g_FakeReturnAddress    = NULL;
static Prototype::TVPXP3ArchiveExtractionFilterFunc g_RealFilter = NULL;



ASM VOID WINAPI FakeExtractionFilterAsm(tTVPXP3ExtractionFilterInfo * /* info */)
{
	INLINE_ASM
	{
		mov      ecx, g_RealFilter;
		jecxz    NO_EXT_FILTER;
		mov      eax, g_FakeReturnAddress;
		mov[esp], eax;
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

	enum { FOUND_NONE, FOUND_C2, FOUND_04, FOUND_00 };

	Buffer = (PBYTE)KrkrExtractCore::GetInstance()->GetFirstSectionAddress();

	State = FOUND_NONE;
	for (ULONG SizeOfImage = KrkrExtractCore::GetInstance()->GetSizeOfImage(); SizeOfImage; ++Buffer, --SizeOfImage)
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
	SEH_TRY
	{
		ULONG_PTR ReturnAddress = FindReturnAddressWorker();

		if (ReturnAddress != NULL)
		{
			g_FakeExtractionFilter = (ULONG_PTR)FakeExtractionFilterAsm;
			return ReturnAddress;
		}
	}
	SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
	}
	return NULL;
}


Prototype::TVPXP3ArchiveExtractionFilterFunc WINAPI InitFakeFilter(PBOOL Result)
{
	PIMAGE_DOS_HEADER       DosHeader;
	PIMAGE_NT_HEADERS       NtHeader;
	PIMAGE_SECTION_HEADER   SectionHeader;

	g_FakeReturnAddress = FindReturnAddress();
	if (!g_FakeExtractionFilter)
	{
		*Result = FALSE;
		return NULL;
	}

	DosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
	NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);
	SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeader + NtHeader->FileHeader.SizeOfOptionalHeader + sizeof(*NtHeader) - sizeof(NtHeader->OptionalHeader));

	KrkrExtractCore::GetInstance()->SetFirstSectionAddress(SectionHeader->VirtualAddress + (ULONG_PTR)DosHeader);
	KrkrExtractCore::GetInstance()->SetSizeOfImage(NtHeader->OptionalHeader.SizeOfImage);
	if (Result) {
		*Result = TRUE;
	}

	return FakeExtractionFilter;
}


void
NTAPI
XeTVPSetXP3ArchiveExtractionFilter(
	tTVPXP3ArchiveExtractionFilter Filter
)
{
	BOOL               Success;

	Success      = FALSE;
	g_RealFilter = Filter;

	tTVPXP3ArchiveExtractionFilter FakeFilter = InitFakeFilter(&Success);
	
	if (Success && FakeFilter) {
		KrkrExtractCore::GetInstance()->SetXP3Filter(FakeFilter);
	}
	else {
		KrkrExtractCore::GetInstance()->SetXP3Filter(Filter);
	}

	PrintConsoleW(L"Filter ProcAddr : %p\n", Filter);
	KrkrExtractCore::GetInstance()->m_TVPSetXP3ArchiveExtractionFilter(Filter);
}




//
// multiple called
//
iTVPFunctionExporter*
NTAPI
XeTVPGetFunctionExporter()
{
	NTSTATUS              Status;
	iTVPFunctionExporter* Exporter;
	PVOID                 Target;

	static char funcname[] = "void ::TVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter)";

	Exporter = KrkrExtractCore::GetInstance()->m_TVPGetFunctionExporter();
	KrkrExtractCore::GetInstance()->SetTVPFunctionExporter(Exporter);

	if (KrkrExtractCore::GetInstance()->GetInitializationType() == KrkrExtractCore::ModuleInitializationType::NotInited)
	{
		KrkrExtractCore::GetInstance()->InitExporterWithExe(Exporter);
		ImageWorkerV2Link(Exporter);
		InitLayer(KrkrExtractCore::GetInstance());

		Status = KrkrExtractCore::GetInstance()->HookTVPSetXP3ArchiveExtractionFilter();
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"Failed to hook TVPSetXP3ArchiveExtractionFilter\n");
		}

		KrkrExtractCore::GetInstance()->SetInitializationType(KrkrExtractCore::ModuleInitializationType::ByExeModule);
		KrkrExtractCore::GetInstance()->InitHookWithExe();
	}
	return Exporter;
}


NTSTATUS KrkrExtractCore::HookGetProcAddress()
{
	NTSTATUS Status;

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(GetProcAddress, XeGetProcAddress, &m_GetProcAddress)
	};

	if (m_IsGetProcAddressHooked)
		return STATUS_SUCCESS;

	Status = Mp::PatchMemory(f, countof(f));
	if (NT_SUCCESS(Status))
	{
		m_IsGetProcAddressHooked = TRUE;
		return Status;
	}

	m_GetProcAddress = nullptr;
	return Status;
}

NTSTATUS KrkrExtractCore::UnHookGetProcAddress()
{
	NTSTATUS Status;

	if (!m_IsGetProcAddressHooked)
		return STATUS_UNSUCCESSFUL;

	Status = Mp::RestoreMemory(m_GetProcAddress);
	if (NT_SUCCESS(Status))
	{
		m_GetProcAddress = nullptr;
		m_IsGetProcAddressHooked = FALSE;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::HookCreateProcessInternalW()
{
	NTSTATUS Status;
	PVOID    Kernel32Handle, Routine;

	Kernel32Handle = GetKernel32Handle();
	Routine = Nt_GetProcAddress(Kernel32Handle, "CreateProcessInternalW");

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(Routine, XeCreateProcessInternalW, &m_CreateProcessInternalW)
	};

	if (m_IsCreateProcessInternalWHooked)
		return STATUS_SUCCESS;

	Status = Mp::PatchMemory(f, countof(f));
	if (NT_SUCCESS(Status))
	{
		m_IsCreateProcessInternalWHooked = TRUE;
		return Status;
	}

	m_CreateProcessInternalW = nullptr;
	return Status;
}

NTSTATUS KrkrExtractCore::UnHookCreateProcessInternalW()
{
	NTSTATUS Status;

	if (!m_IsCreateProcessInternalWHooked)
		return STATUS_UNSUCCESSFUL;

	Status = Mp::RestoreMemory(m_CreateProcessInternalW);
	if (NT_SUCCESS(Status))
	{
		m_CreateProcessInternalW = nullptr;
		m_IsCreateProcessInternalWHooked = FALSE;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::HookMultiByteToWideChar()
{
	NTSTATUS Status;

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(MultiByteToWideChar, XeMultiByteToWideChar, &m_MultiByteToWideChar)
	};

	if (m_IsMultiByteToWideCharHooked)
		return STATUS_SUCCESS;

	Status = Mp::PatchMemory(f, countof(f));
	if (NT_SUCCESS(Status))
	{
		m_IsMultiByteToWideCharHooked = TRUE;
		return Status;
	}

	m_MultiByteToWideChar = nullptr;
	return Status;
}

NTSTATUS KrkrExtractCore::UnHookMultiByteToWideChar()
{
	NTSTATUS Status;

	if (!m_IsMultiByteToWideCharHooked)
		return STATUS_UNSUCCESSFUL;

	Status = Mp::RestoreMemory(m_MultiByteToWideChar);
	if (NT_SUCCESS(Status))
	{
		m_MultiByteToWideChar = nullptr;
		m_IsMultiByteToWideCharHooked = FALSE;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::HookCreateFileW()
{
	NTSTATUS Status;

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(CreateFileW, XeCreateFileW, &m_CreateFileW)
	};

	if (m_IsCreateFileWHooked)
		return STATUS_SUCCESS;

	Status = Mp::PatchMemory(f, countof(f));
	if (NT_SUCCESS(Status))
	{
		m_IsCreateFileWHooked = TRUE;
		return Status;
	}

	m_CreateFileW = nullptr;
	return Status;
}

NTSTATUS KrkrExtractCore::UnHookCreateFileW()
{
	NTSTATUS Status;

	if (!m_IsCreateFileWHooked)
		return STATUS_UNSUCCESSFUL;

	Status = Mp::RestoreMemory(m_CreateFileW);
	if (NT_SUCCESS(Status))
	{
		m_CreateFileW = nullptr;
		m_IsCreateFileWHooked = FALSE;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::HookReadFile()
{
	NTSTATUS Status;

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(ReadFile, XeReadFile, &m_ReadFile)
	};

	if (m_IsReadFileHooked)
		return STATUS_SUCCESS;

	Status = Mp::PatchMemory(f, countof(f));
	if (NT_SUCCESS(Status))
	{
		m_IsReadFileHooked = TRUE;
		return Status;
	}

	m_ReadFile = nullptr;
	return Status;
}

NTSTATUS KrkrExtractCore::UnHookReadFile()
{
	NTSTATUS Status;

	if (!m_IsReadFileHooked)
		return STATUS_UNSUCCESSFUL;

	Status = Mp::RestoreMemory(m_ReadFile);
	if (NT_SUCCESS(Status))
	{
		m_ReadFile = nullptr;
		m_IsReadFileHooked = FALSE;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::HookIsDebuggerPresent()
{
	NTSTATUS Status;

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(IsDebuggerPresent, XeIsDebuggerPresent, &m_IsDebuggerPresent)
	};

	if (m_IsIsDebuggerPresentHooked)
		return STATUS_SUCCESS;

	Status = Mp::PatchMemory(f, countof(f));
	if (NT_SUCCESS(Status))
	{
		m_IsIsDebuggerPresentHooked = TRUE;
		return Status;
	}

	m_IsDebuggerPresent = nullptr;
	return Status;
}

NTSTATUS KrkrExtractCore::UnHookIsDebuggerPresent()
{
	NTSTATUS Status;

	if (!m_IsIsDebuggerPresentHooked)
		return STATUS_UNSUCCESSFUL;

	Status = Mp::RestoreMemory(m_IsDebuggerPresent);
	if (NT_SUCCESS(Status))
	{
		m_IsDebuggerPresent = nullptr;
		m_IsIsDebuggerPresentHooked = FALSE;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::HookLoadLibraryA()
{
	NTSTATUS Status;

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(LoadLibraryA, XeLoadLibraryA)
	};

	if (m_IsLoadLibraryAHooked)
		return STATUS_SUCCESS;

	Status = Mp::PatchMemory(f, countof(f));
	if (NT_SUCCESS(Status))
	{
		m_IsLoadLibraryAHooked = TRUE;
		return Status;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::UnHookLoadLibraryA()
{
	if (!m_IsLoadLibraryAHooked)
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS;
}

NTSTATUS KrkrExtractCore::HookLoadLibraryW()
{
	NTSTATUS Status;

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(LoadLibraryW, XeLoadLibraryW)
	};

	if (m_IsLoadLibraryWHooked)
		return STATUS_SUCCESS;

	Status = Mp::PatchMemory(f, countof(f));
	if (NT_SUCCESS(Status))
	{
		m_IsLoadLibraryWHooked = TRUE;
		return Status;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::UnHookLoadLibraryW()
{
	if (!m_IsLoadLibraryWHooked)
		return STATUS_UNSUCCESSFUL;


	return STATUS_SUCCESS;
}

NTSTATUS KrkrExtractCore::HookTVPGetFunctionExporter()
{
	NTSTATUS         Status;

	m_TVPGetFunctionExporter = (Prototype::TVPGetFunctionExporterFunc)Nt_GetProcAddress(
		KrkrExtractCore::GetInstance()->GetHostModule(),
		"TVPGetFunctionExporter"
	);

	if (m_TVPGetFunctionExporter == nullptr) {
		m_TVPGetFunctionExporter = 
			(Prototype::TVPGetFunctionExporterFunc)KrkrExtractCore::GetInstance()->GetTVPGetFunctionExporter();
	}

	if (m_TVPGetFunctionExporter == nullptr) {
		return STATUS_NOT_FOUND;
	}

	if (m_IsTVPGetFunctionExporterHooked)
		return STATUS_SUCCESS;

	Mp::PATCH_MEMORY_DATA p[] =
	{
		Mp::FunctionJumpVa(m_TVPGetFunctionExporter, XeTVPGetFunctionExporter, &m_TVPGetFunctionExporter)
	};

	Status = Mp::PatchMemory(p, countof(p));
	if (NT_SUCCESS(Status))
	{
		m_IsTVPGetFunctionExporterHooked = TRUE;
		return Status;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::UnHookTVPGetFunctionExporter()
{
	NTSTATUS Status;

	if (!m_IsTVPGetFunctionExporterHooked)
		return STATUS_UNSUCCESSFUL;

	Status = Mp::RestoreMemory(m_TVPGetFunctionExporter);
	if (NT_SUCCESS(Status))
	{
		m_TVPGetFunctionExporter = nullptr;
		m_IsTVPGetFunctionExporterHooked = FALSE;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::HookTVPSetXP3ArchiveExtractionFilter()
{
	NTSTATUS Status;

	static char funcname[] = "void ::TVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter)";

	m_TVPSetXP3ArchiveExtractionFilter = (Prototype::SetXP3FilterFunc)TVPGetImportFuncPtr(funcname);
	if (!m_TVPSetXP3ArchiveExtractionFilter)
		return STATUS_NOT_FOUND;

	Mp::PATCH_MEMORY_DATA p[] =
	{
		Mp::FunctionJumpVa(m_TVPSetXP3ArchiveExtractionFilter, XeTVPSetXP3ArchiveExtractionFilter, &m_TVPSetXP3ArchiveExtractionFilter)
	};

	Status = Mp::PatchMemory(p, countof(p));
	if (NT_SUCCESS(Status))
	{
		m_IsTVPSetXP3ArchiveExtractionFilterHooked = TRUE;
		return Status;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::UnHookTVPSetXP3ArchiveExtractionFilter()
{
	NTSTATUS Status;

	if (!m_IsTVPSetXP3ArchiveExtractionFilterHooked)
		return STATUS_UNSUCCESSFUL;

	Status = Mp::RestoreMemory(m_TVPSetXP3ArchiveExtractionFilter);
	if (NT_SUCCESS(Status))
	{
		m_TVPSetXP3ArchiveExtractionFilter = nullptr;
		m_IsTVPSetXP3ArchiveExtractionFilterHooked = FALSE;
	}

	return Status;
}




using ZlibFunc = int (NTAPI*)(unsigned char*, unsigned long*, const unsigned char*, unsigned long);
ZlibFunc Zlib_func = nullptr;
int NTAPI XeZLIB_uncompress(unsigned char* uncompressed, unsigned long* usize, const unsigned char* compressed, unsigned long csize)
{
	int ret = Zlib_func(uncompressed, usize, compressed, csize);
	if (ret == Z_OK && uncompressed && usize && *usize) {
		NTSTATUS   Status;
		NtFileDisk File;
		wstring    HashName;


		GenMD5Code(uncompressed, *usize, HashName);

		HashName += L"_krkrz.index";
		Status = File.Create(HashName.c_str());
		if (NT_FAILED(Status))
			return Status;

		File.Write(uncompressed, *usize);
		return File.Close();
	}
}

NTSTATUS KrkrExtractCore::HookZLIBUncompress()
{
	static char funcname[] = "int ::ZLIB_uncompress(unsigned char *,unsigned long *,const unsigned char *,unsigned long)";
	void* func = TVPGetImportFuncPtr(funcname);

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(func, XeZLIB_uncompress, &Zlib_func)
	};

	return Mp::PatchMemory(f, countof(f));
}


NTSTATUS KrkrExtractCore::HookV2Link(PVOID Module)
{
	NTSTATUS              Status;
	Prototype::V2LinkFunc V2LinkPtr;

	V2LinkPtr = (Prototype::V2LinkFunc)Nt_GetProcAddress(Module, "V2Link");
	if (V2LinkPtr == nullptr)
		return STATUS_UNSUCCESSFUL;

	Mp::PATCH_MEMORY_DATA p[] =
	{
		Mp::FunctionJumpVa(V2LinkPtr, XeV2Link, &m_V2Link)
	};

	Status = Mp::PatchMemory(p, countof(p));
	if (NT_SUCCESS(Status))
	{
		m_IsV2LinkHooked = TRUE;
		return Status;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::UnHookV2Link()
{
	NTSTATUS Status;

	if (!m_IsV2LinkHooked)
		return STATUS_UNSUCCESSFUL;

	Status = Mp::RestoreMemory(m_V2Link);
	if (NT_SUCCESS(Status))
	{
		m_V2Link = nullptr;
		m_IsV2LinkHooked = FALSE;
	}

	return Status;
}

NTSTATUS KrkrExtractCore::HookIsDBCSLeadByte()
{
	NTSTATUS Status;

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(IsDBCSLeadByte, XeIsDBCSLeadByte, &m_IsDBCSLeadByte)
	};

	if (m_IsDBCSLeadByteHooked)
		return STATUS_SUCCESS;

	Status = Mp::PatchMemory(f, countof(f));
	if (NT_SUCCESS(Status))
	{
		m_IsDBCSLeadByte = nullptr;
		m_IsDBCSLeadByteHooked = TRUE;
		return Status;
	}

	return Status;

}


NTSTATUS KrkrExtractCore::UnHookIsDBCSLeadByte()
{
	NTSTATUS Status;

	if (!m_IsDBCSLeadByteHooked)
		return STATUS_UNSUCCESSFUL;

	Status = Mp::RestoreMemory(m_IsDBCSLeadByte);
	if (NT_SUCCESS(Status))
	{
		m_IsDBCSLeadByteHooked = FALSE;
	}

	return Status;
}


NTSTATUS KrkrExtractCore::SetHwBreakPoint(PVOID Address, SIZE_T Size, HardwareBreakpoint::Condition AccessStatus, INT& Index)
{
	NTSTATUS Status;

	Index = -1;
	Status = HardwareBreakpoint::FindAvailableRegister(Index);
	if (NT_FAILED(Status)) {
		PrintConsoleW(L"KrkrExtractCore::SetHwBreakPoint : HardwareBreakpoint::FindAvailableRegister failed %08x\n", Status);
		return Status;
	}

	if (Index < 0 || Index > 3) {
		PrintConsoleW(L"KrkrExtractCore::SetHwBreakPoint : Invalid index %d\n", Index);
		return STATUS_NOT_SUPPORTED;
	}

	if (m_BreakPoint[Index].IsBusy()) {
		PrintConsoleW(L"KrkrExtractCore::SetHwBreakPoint : Index = %d is already set\n", Index);
		return STATUS_NOT_SUPPORTED;
	}

	switch (Size) 
	{
	case 1:
	case 2:
	case 4:
#if defined(ML_AMD64)
	case 8:
#endif
		Status = m_BreakPoint[Index].Set(Address, Size, AccessStatus, Index);
	break;

	default:
		PrintConsoleW(L"KrkrExtractCore::SetHwBreakPoint : Invalid size = %d\n", Size);
		return STATUS_NOT_SUPPORTED;
	}

	if (NT_FAILED(Status)) {
		PrintConsoleW(L"KrkrExtractCore::SetHwBreakPoint : m_BreakPoint[%d].Set failed %08x\n", Index, Status);
		return Status;
	}

	using namespace magic_enum::bitwise_operators;
	
	switch (Index)
	{
	case 0:
		SET_FLAG(m_BreakPointIndexSet, HwBreakPointStatus::Slot0IsBusy);
		break;

	case 1:
		SET_FLAG(m_BreakPointIndexSet, HwBreakPointStatus::Slot1IsBusy);
		break;

	case 2:
		SET_FLAG(m_BreakPointIndexSet, HwBreakPointStatus::Slot2IsBusy);
		break;

	case 3:
		SET_FLAG(m_BreakPointIndexSet, HwBreakPointStatus::Slot3IsBusy);
		break;
	}

	m_BreakPointAddresses[Index] = (ULONG_PTR)Address;
	
	return Status;
}

NTSTATUS KrkrExtractCore::RemoveHwBreakPoint(INT Index)
{
	NTSTATUS Status;

	using namespace magic_enum::bitwise_operators;

	switch (Index)
	{
	case 0:
		if (FLAG_OFF(m_BreakPointIndexSet, HwBreakPointStatus::Slot0IsBusy))
			return STATUS_SUCCESS;

		Status = m_BreakPoint[0].Clear();
		if (NT_FAILED(Status))
			break;

		m_BreakPointAddresses[0] = 0;
		CLEAR_FLAG(m_BreakPointIndexSet, HwBreakPointStatus::Slot0IsBusy);
		break;

	case 1:
		if (FLAG_OFF(m_BreakPointIndexSet, HwBreakPointStatus::Slot1IsBusy))
			return STATUS_SUCCESS;

		Status = m_BreakPoint[1].Clear();
		if (NT_FAILED(Status))
			break;

		m_BreakPointAddresses[1] = 0;
		CLEAR_FLAG(m_BreakPointIndexSet, HwBreakPointStatus::Slot1IsBusy);
		break;

	case 2:
		if (FLAG_OFF(m_BreakPointIndexSet, HwBreakPointStatus::Slot2IsBusy))
			return STATUS_SUCCESS;

		Status = m_BreakPoint[2].Clear();
		if (NT_FAILED(Status))
			break;

		m_BreakPointAddresses[2] = 0;
		CLEAR_FLAG(m_BreakPointIndexSet, HwBreakPointStatus::Slot2IsBusy);
		break;

	case 3:
		if (FLAG_OFF(m_BreakPointIndexSet, HwBreakPointStatus::Slot3IsBusy))
			return STATUS_SUCCESS;

		Status = m_BreakPoint[3].Clear();
		m_BreakPointAddresses[3] = 0;
		CLEAR_FLAG(m_BreakPointIndexSet, HwBreakPointStatus::Slot3IsBusy);
		break;

	default:
		PrintConsoleW(L"KrkrExtractCore::RemoveHwBreakPoint : Unsupported index = %d\n", Index);
		return STATUS_NOT_SUPPORTED;
	}

	if (NT_FAILED(Status)) {
		PrintConsoleW(L"KrkrExtractCore::RemoveHwBreakPoint : m_BreakPoint[%d].Clear() failed, %08x\n", Index, Status);
		return Status;
	}


	return Status;
}

NTSTATUS KrkrExtractCore::RemoveAllHwBreakPoint(PCONTEXT Context)
{
	NTSTATUS Status;

	Status = HardwareBreakpoint::ClearAll(Context, m_BreakPointIndexSet);
	if (NT_FAILED(Status)) {
		PrintConsoleW(L"KrkrExtractCore::RemoveAllHwBreakPoint : HardwareBreakpoint::ClearAll failed %08x\n", Status);
		return Status;
	}

	using namespace magic_enum::bitwise_operators;

	if (FLAG_ON(m_BreakPointIndexSet, HwBreakPointStatus::Slot0IsBusy)) 
	{
		m_BreakPoint[0].Invaild();
		m_BreakPointAddresses[0] = 0;
	}
	
	if (FLAG_ON(m_BreakPointIndexSet, HwBreakPointStatus::Slot1IsBusy)) 
	{
		m_BreakPoint[1].Invaild();
		m_BreakPointAddresses[1] = 0;
	}

	if (FLAG_ON(m_BreakPointIndexSet, HwBreakPointStatus::Slot2IsBusy)) 
	{
		m_BreakPoint[2].Invaild();
		m_BreakPointAddresses[2] = 0;
	}

	if (FLAG_ON(m_BreakPointIndexSet, HwBreakPointStatus::Slot3IsBusy))
	{
		m_BreakPoint[3].Invaild();
		m_BreakPointAddresses[3] = 0;
	}

	m_BreakPointIndexSet = HwBreakPointStatus::SlotEmpty;

	return Status;
}

NTSTATUS KrkrExtractCore::GetBusyHwBreakPoint(_Out_ HwBreakPointStatus& BpStatus)
{
	BpStatus = m_BreakPointIndexSet;
	
	return STATUS_SUCCESS;
}


