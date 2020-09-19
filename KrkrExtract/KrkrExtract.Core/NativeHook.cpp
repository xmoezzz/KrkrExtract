#include "KrkrHook.h"
#include "KrkrExtract.h"
#include "tp_stub.h"
#include "Decoder.h"
#include "Prototype.h"
#include "Helper.h"
#include <Psapi.h>
#include "magic_enum.hpp"

#define KrContext(Handle) Handle->GetKrkrInstance()->GetInstanceForHook()

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
	KrkrHook* Handle = KrkrHook::GetInstance();

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

	return Handle->m_MultiByteToWideChar(
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
	KrkrHook*        Handle;

	Length      = (StrLengthA(FileName) + 1) * 2;
	UnicodeName = (PWSTR)AllocStack(Length);
	Handle      = KrkrHook::GetInstance();

	RtlZeroMemory(UnicodeName, Length);
	RtlMultiByteToUnicodeN(UnicodeName, Length, &OutLength, (PSTR)FileName, Length / 2 - 1);

	Module = Nt_LoadLibrary(UnicodeName);
	if (!Module)
		return Module;

	if (LookupImportTable(Module, "KERNEL32.dll", "FlushInstructionCache") != IMAGE_INVALID_VA)
		return Module;

	KrContext(Handle)->InitHookWithDll(UnicodeName, Module);
	return Module;
}


PVOID
NTAPI
XeLoadLibraryW(LPWSTR FileName)
{
	PVOID            Module;
	KrkrHook*        Handle;

	Handle = KrkrHook::GetInstance();
	Module = Nt_LoadLibrary(FileName);

	if (!Module)
		return NULL;

	if (LookupImportTable(Module, "KERNEL32.dll", "FlushInstructionCache") != IMAGE_INVALID_VA)
		return Module;

	KrContext(Handle)->InitHookWithDll(FileName, Module);
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
	KrkrHook*        Handle;
	HANDLE           FileHandle;
	MODULEINFO       ModuleInfo;

	Handle   = KrkrHook::GetInstance();
	auto&& FileName = GetPackageName(lpFileName);

	RtlZeroMemory(&ModuleInfo, sizeof(MODULEINFO));
	GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(NULL), &ModuleInfo, sizeof(MODULEINFO));

	if (IN_RANGE(
		(ULONG_PTR)ModuleInfo.lpBaseOfDll,
		(ULONG_PTR)_ReturnAddress(),
		(ULONG_PTR)ModuleInfo.lpBaseOfDll + ModuleInfo.SizeOfImage)) {
		//PrintConsoleW(L"CreateFile : %s\n", lpFileName);
	}

	FileHandle = Handle->m_CreateFileW(
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
		KrContext(Handle)->GetCurrentTempFileName()) && FileHandle != INVALID_HANDLE_VALUE) {
		KrContext(Handle)->SetCurrentTempHandle(FileHandle);
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
	KrkrHook*        Handle;

	Handle = KrkrHook::GetInstance();
	RtlInitUnicodeString(&FullDllPath, KrContext(Handle)->m_DllPath.c_str());

	IsSuspended = !!(dwCreationFlags & CREATE_SUSPENDED);
	dwCreationFlags |= CREATE_SUSPENDED;
	Success = Handle->m_CreateProcessInternalW(
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
	KrkrHook*             Handle;
	Prototype::V2LinkFunc V2LinkStub;


	Handle = KrkrHook::GetInstance();

	Status = KrContext(Handle)->InitExporterWithDll(Exporter);
	if (NT_FAILED(Status)) 
	{
		PrintConsoleW(L"XeV2Link : InitExporterWithDll failed %08x", Status);

		V2LinkStub = KrContext(Handle)->GetV2Link();
		return V2LinkStub(Exporter);
	}

	ImageWorkerV2Link(Exporter);
	KrContext(Handle)->SetTVPFunctionExporter(Exporter);
	InitLayer(KrContext(Handle));

	V2LinkStub = KrContext(Handle)->GetV2Link();
	return V2LinkStub(Exporter);
}


BOOL WINAPI XeIsDBCSLeadByte(BYTE TestChar)
{
	return IsDBCSLeadByteEx(932, TestChar);
}


LONG NTAPI FindPrivateProcHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	KrkrHook*        Handle;
	PVOID            ModuleBase;
	ULONG            iPos, RealRetAddr;
	BOOL             FindRet;
	DWORD            PreviousProtect;

	Handle = KrkrHook::GetInstance();

	PrintConsoleW(
		L"FindPrivateProcHandler : %08x -> %08x\n", 
		ExceptionInfo->ExceptionRecord->ExceptionCode,
		ExceptionInfo->ContextRecord->Eip
	);

	if (Handle->m_BreakOnce == FALSE && ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
	{
		Handle->RemoveAllHwBreakPointAt(ExceptionInfo->ContextRecord);
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
				Handle->m_RetAddr = iPos;

				BYTE Break = 0xCC;

				Nt_WriteProtectMemory(NtCurrentProcess(), (PVOID)Handle->m_RetAddr, &Break, sizeof(Break));
				break;
			}

			iPos += OpSize;
		}

		Handle->m_BreakOnce = TRUE;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	else if (
		Handle->m_RetAddr != 0 && 
		ExceptionInfo->ContextRecord->Eip == Handle->m_RetAddr && 
		ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT
		)
	{
		BYTE Break = 0xC3;

		Nt_WriteProtectMemory(NtCurrentProcess(), (PVOID)Handle->m_RetAddr, &Break, sizeof(Break));

		RealRetAddr = ((PDWORD)(ExceptionInfo->ContextRecord->Esp))[0];
		PrintConsoleW(L"ret addr : %08x\n", RealRetAddr);

		//
		// validate this call first
		//

		KrContext(Handle)->SetSpecialChunkDecoder((Prototype::SpecialChunkDecoderFunc)GetCallDestination(RealRetAddr - 5));
		PrintConsoleW(L"ok, I GOT YOUR ROUTINE -> %p\n", KrContext(Handle)->GetSpecialChunkDecoder());

		Handle->m_RetAddr = 0;
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
	KrkrHook*        Handle;

	Handle = KrkrHook::GetInstance();

	SectionProtector AutoLocker(&Handle->m_ReadFileLocker);

	Success = Handle->m_ReadFile(
		hFile, 
		lpBuffer, 
		nNumberOfBytesToRead, 
		lpNumberOfBytesRead, 
		lpOverlapped
	);

	if (KrContext(Handle)->GetSpecialChunkDecoder() && Handle->m_ExceptionHandler)
	{
		RemoveStatus = RtlRemoveVectoredExceptionHandler(Handle->m_ExceptionHandler);
		if (RemoveStatus) 
		{
			PrintConsoleW(L"XeReadFile : RtlRemoveVectoredExceptionHandler ok\n");
			Handle->m_ExceptionHandler = nullptr;
		}
		else {
			PrintConsoleW(L"XeReadFile : RtlRemoveVectoredExceptionHandler failed, LastError = %d\n", GetLastError());
		}

		return Success;
	}

	auto Entry = KrContext(Handle)->m_SpecialChunkMapBySize.find(nNumberOfBytesToRead);

	if (Success && Entry != KrContext(Handle)->m_SpecialChunkMapBySize.end() && Handle->m_SetBpOnce == FALSE)
	{
		Hash = MurmurHash64B(lpBuffer, nNumberOfBytesToRead);

		//
		// also check name
		//

		if (Hash == Entry->second.Hash) 
		{
			Handle->m_SetBpOnce = TRUE;

			//
			// add handler to vectored exception list
			//

			Handle->m_ExceptionHandler = RtlAddVectoredExceptionHandler(1, FindPrivateProcHandler);

			if (!Handle->m_ExceptionHandler) {
				PrintConsoleW(L"Try to added handler, reason = %u\n", GetLastError());
			}
			else {
				PrintConsoleW(L"Exception was added.\n");
			}

			//
			// set hardware breakpoint
			//

			Index = -1;
			Status = Handle->SetHwBreakPointAt(lpBuffer, 1, HardwareBreakpoint::Condition::Write, Index);
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
	KrkrHook*    Handle;

	enum { FOUND_NONE, FOUND_C2, FOUND_04, FOUND_00 };

	Handle = KrkrHook::GetInstance();
	Buffer = (PBYTE)KrContext(Handle)->GetFirstSectionAddress();

	State = FOUND_NONE;
	for (ULONG SizeOfImage = KrContext(Handle)->GetSizeOfImage(); SizeOfImage; ++Buffer, --SizeOfImage)
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
	KrkrHook*               Handle;

	Handle = KrkrHook::GetInstance();
	g_FakeReturnAddress = FindReturnAddress();
	if (!g_FakeExtractionFilter)
	{
		*Result = FALSE;
		return NULL;
	}

	DosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
	NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);
	SectionHeader = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeader + NtHeader->FileHeader.SizeOfOptionalHeader + sizeof(*NtHeader) - sizeof(NtHeader->OptionalHeader));

	KrContext(Handle)->SetFirstSectionAddress(SectionHeader->VirtualAddress + (ULONG_PTR)DosHeader);
	KrContext(Handle)->SetSizeOfImage(NtHeader->OptionalHeader.SizeOfImage);
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
	KrkrHook*          Handle;

	Handle = KrkrHook::GetInstance();
	Success      = FALSE;
	g_RealFilter = Filter;

	tTVPXP3ArchiveExtractionFilter FakeFilter = InitFakeFilter(&Success);
	
	if (Success && FakeFilter) {
		KrContext(Handle)->SetXP3Filter(FakeFilter);
	}
	else {
		KrContext(Handle)->SetXP3Filter(Filter);
	}

	PrintConsoleW(L"Filter ProcAddr : %p\n", Filter);
	Handle->m_TVPSetXP3ArchiveExtractionFilter(Filter);
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
	KrkrHook*             Handle;

	static char funcname[] = "void ::TVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter)";

	Handle   = KrkrHook::GetInstance();
	Exporter = Handle->m_TVPGetFunctionExporter();
	Handle->GetKrkrInstance()->GetInstanceForHook()->SetTVPFunctionExporter(Exporter);

	if (KrContext(Handle)->GetInitializationType() == KrkrExtractCore::ModuleInitializationType::NotInited)
	{
		KrContext(Handle)->InitExporterWithExe(Exporter);
		ImageWorkerV2Link(Exporter);
		InitLayer(KrContext(Handle));

		Status = Handle->HookTVPSetXP3ArchiveExtractionFilter();
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"Failed to hook TVPSetXP3ArchiveExtractionFilter\n");
		}

		KrContext(Handle)->SetInitializationType(KrkrExtractCore::ModuleInitializationType::ByExeModule);
		KrContext(Handle)->InitHookWithExe();
	}
	return Exporter;
}


NTSTATUS KrkrHook::HookGetProcAddressNative()
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

NTSTATUS KrkrHook::UnHookGetProcAddressNative()
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

NTSTATUS KrkrHook::HookCreateProcessInternalWNative()
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

NTSTATUS KrkrHook::UnHookCreateProcessInternalWNative()
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

NTSTATUS KrkrHook::HookMultiByteToWideCharNative()
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

NTSTATUS KrkrHook::UnHookMultiByteToWideCharNative()
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

NTSTATUS KrkrHook::HookCreateFileWNative()
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

NTSTATUS KrkrHook::UnHookCreateFileWNative()
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

NTSTATUS KrkrHook::HookReadFileNative()
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

NTSTATUS KrkrHook::UnHookReadFileNative()
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

NTSTATUS KrkrHook::HookIsDebuggerPresentNative()
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

NTSTATUS KrkrHook::UnHookIsDebuggerPresentNative()
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

NTSTATUS KrkrHook::HookLoadLibraryANative()
{
	NTSTATUS Status;

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(LoadLibraryA, XeLoadLibraryA, &m_IsLoadLibraryAHooked)
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

NTSTATUS KrkrHook::UnHookLoadLibraryANative()
{
	if (!m_IsLoadLibraryAHooked)
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS;
}

NTSTATUS KrkrHook::HookLoadLibraryWNative()
{
	NTSTATUS Status;

	Mp::PATCH_MEMORY_DATA f[] =
	{
		Mp::FunctionJumpVa(LoadLibraryW, XeLoadLibraryW, &m_IsLoadLibraryWHooked)
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

NTSTATUS KrkrHook::UnHookLoadLibraryWNative()
{
	if (!m_IsLoadLibraryWHooked)
		return STATUS_UNSUCCESSFUL;


	return STATUS_SUCCESS;
}

NTSTATUS KrkrHook::HookTVPGetFunctionExporterNative()
{
	NTSTATUS         Status;
	KrkrHook*        Handle;

	Handle = KrkrHook::GetInstance();

	m_TVPGetFunctionExporter = (Prototype::TVPGetFunctionExporterFunc)Nt_GetProcAddress(
		KrContext(this)->GetHostModule(),
		"TVPGetFunctionExporter"
	);

	if (m_TVPGetFunctionExporter == nullptr) {
		m_TVPGetFunctionExporter = (Prototype::TVPGetFunctionExporterFunc)KrContext(Handle)->GetTVPGetFunctionExporter();
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

NTSTATUS KrkrHook::UnHookTVPGetFunctionExporterNative()
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

NTSTATUS KrkrHook::HookTVPSetXP3ArchiveExtractionFilterNative()
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

NTSTATUS KrkrHook::UnHookTVPSetXP3ArchiveExtractionFilterNative()
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

NTSTATUS KrkrHook::HookV2LinkNative(PVOID Module)
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

NTSTATUS KrkrHook::UnHookV2LinkNative()
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

NTSTATUS KrkrHook::HookIsDBCSLeadByteNative()
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


NTSTATUS KrkrHook::UnHookIsDBCSLeadByteNative()
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


NTSTATUS KrkrHook::SetHwBreakPointNative(PVOID Address, SIZE_T Size, HardwareBreakpoint::Condition AccessStatus, INT& Index)
{
	NTSTATUS Status;

	Index = -1;
	Status = HardwareBreakpoint::FindAvailableRegister(Index);
	if (NT_FAILED(Status)) {
		PrintConsoleW(L"KrkrHook::SetHwBreakPointNative : HardwareBreakpoint::FindAvailableRegister failed %08x\n", Status);
		return Status;
	}

	if (Index < 0 || Index > 3) {
		PrintConsoleW(L"KrkrHook::SetHwBreakPointNative : Invalid index %d\n", Index);
		return STATUS_NOT_SUPPORTED;
	}

	if (m_BreakPoint[Index].IsBusy()) {
		PrintConsoleW(L"KrkrHook::SetHwBreakPointNative : Index = %d is already set\n", Index);
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
		PrintConsoleW(L"KrkrHook::SetHwBreakPointNative : Invalid size = %d\n", Size);
		return STATUS_NOT_SUPPORTED;
	}

	if (NT_FAILED(Status)) {
		PrintConsoleW(L"KrkrHook::SetHwBreakPointNative : m_BreakPoint[%d].Set failed %08x\n", Index, Status);
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

NTSTATUS KrkrHook::RemoveHwBreakPointNative(INT Index)
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
		PrintConsoleW(L"KrkrHook::RemoveHwBreakPointNative : Unsupported index = %d\n", Index);
		return STATUS_NOT_SUPPORTED;
	}

	if (NT_FAILED(Status)) {
		PrintConsoleW(L"KrkrHook::RemoveHwBreakPointNative : m_BreakPoint[%d].Clear() failed, %08x\n", Index, Status);
		return Status;
	}


	return Status;
}

NTSTATUS KrkrHook::RemoveAllHwBreakPointNative(PCONTEXT Context)
{
	NTSTATUS Status;

	Status = HardwareBreakpoint::ClearAll(Context, m_BreakPointIndexSet);
	if (NT_FAILED(Status)) {
		PrintConsoleW(L"KrkrHook::RemoveAllHwBreakPointNative : HardwareBreakpoint::ClearAll failed %08x\n", Status);
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

NTSTATUS KrkrHook::GetBusyHwBreakPointNative(_Out_ HwBreakPointStatus& BpStatus)
{
	BpStatus = m_BreakPointIndexSet;
	
	return STATUS_SUCCESS;
}


