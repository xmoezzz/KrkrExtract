#include <my.h>

#pragma comment(lib, "MyLibrary_x64.lib")

Void InjectDllToRemoteProcess64(HANDLE ProcessHandle, HANDLE ThreadHandle, LPCWSTR DllPath)
{
	CONTEXT context;
	DWORD   len;
	LPVOID  mem;
	DWORD64 LLW;

	union
	{
		PBYTE pB;
		PDWORD64 pL;
	} ip;

#define CODESIZE 92
	static BYTE code[CODESIZE + 2 * (MAX_PATH)] =
	{
		0, 0, 0, 0, 0, 0, 0, 0,	// original rip
		0, 0, 0, 0, 0, 0, 0, 0,	// LoadLibraryW
		0x9C,				// pushfq
		0x50,				// push  rax
		0x51,				// push  rcx
		0x52,				// push  rdx
		0x53,				// push  rbx
		0x55,				// push  rbp
		0x56,				// push  rsi
		0x57,				// push  rdi
		0x41, 0x50,			// push  r8
		0x41, 0x51,			// push  r9
		0x41, 0x52,			// push  r10
		0x41, 0x53,			// push  r11
		0x41, 0x54,			// push  r12
		0x41, 0x55,			// push  r13
		0x41, 0x56,			// push  r14
		0x41, 0x57,			// push  r15
		0x48, 0x83, 0xEC, 0x28,		// sub   rsp, 40
		0x48, 0x8D, 0x0D, 41, 0, 0, 0,	// lea   ecx, L"path\to\procrewriter.x64.dll"
		0xFF, 0x15, -49, -1, -1, -1,		// call  LoadLibraryW
		0x48, 0x83, 0xC4, 0x28,		// add   rsp, 40
		0x41, 0x5F,			// pop r15
		0x41, 0x5E,			// pop r14
		0x41, 0x5D,			// pop r13
		0x41, 0x5C,			// pop r12
		0x41, 0x5B,			// pop r11
		0x41, 0x5A,			// pop r10
		0x41, 0x59,			// pop r9
		0x41, 0x58,			// pop r8
		0x5F,				// pop rdi
		0x5E,				// pop rsi
		0x5D,				// pop rbp
		0x5B,				// pop rbx
		0x5A,				// pop rdx
		0x59,				// pop rcx
		0x58,				// pop rax
		0x9D,				// popfq
		0xFF, 0x25, -91, -1, -1, -1,    // jmp   original Rip
		0x0,			   // DWORD alignment for LLW, fwiw
	};

	len = 2 * (StrLengthW(DllPath) + 1);
	if (len > 2 * (MAX_PATH))
		return;

	RtlCopyMemory(code + CODESIZE, DllPath, len);
	len += CODESIZE;

	context.ContextFlags = CONTEXT_CONTROL;
	NtGetContextThread(ThreadHandle, &context);
	mem = VirtualAllocEx(ProcessHandle, NULL, len, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	LLW = (DWORD64)LoadLibraryW;

	ip.pB = code;
	*ip.pL++ = context.Rip;
	*ip.pL++ = LLW;

	WriteProcessMemory(ProcessHandle, mem, code, len, NULL);
	FlushInstructionCache(ProcessHandle, mem, len);
	context.Rip = (DWORD64)mem + 16;
	NtSetContextThread(ThreadHandle, &context);
}

API_POINTER(CreateProcessInternalW) StubCreateProcessInternalW = NULL;

BOOL
WINAPI
CreateProcessInternalWithDll_X64(
HANDLE                  hToken,
LPCWSTR                 lpApplicationName,
LPWSTR                  lpCommandLine,
LPCWSTR                 lpDllPath,
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

	IsSuspended = !!(dwCreationFlags & CREATE_SUSPENDED);
	dwCreationFlags |= CREATE_SUSPENDED;
	Result = StubCreateProcessInternalW(
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

	InjectDllToRemoteProcess64(
		lpProcessInformation->hProcess,
		lpProcessInformation->hThread,
		lpDllPath);

	NtResumeThread(lpProcessInformation->hThread, NULL);

	return TRUE;
}



BOOL
WINAPI
HookCreateProcessInternalW(
HANDLE                  Token,
PCWSTR                  ApplicationName,
PWSTR                   CommandLine,
LPSECURITY_ATTRIBUTES   ProcessAttributes,
LPSECURITY_ATTRIBUTES   ThreadAttributes,
BOOL                    InheritHandles,
DWORD                   CreationFlags,
LPVOID                  Environment,
PCWSTR                  CurrentDirectory,
LPSTARTUPINFOW          StartupInfo,
LPPROCESS_INFORMATION   ProcessInformation,
PHANDLE                 NewToken
)
{
	NTSTATUS            Status;
	PBYTE               Buffer;
	ULONG_PTR           Size;
	NtFileDisk          File;
	LPWSTR*             Argv;
	LONG_PTR            Argc;
	LPCWSTR             Application;
	PIMAGE_NT_HEADERS32 NtHeaders32;
	PIMAGE_NT_HEADERS64 NtHeaders64;
	ULONG_PTR           NtHeadersVersion;
	BOOL                IsKrkrModule;
	PVOID               Ptr;
	BOOL                IsWow64Process;
	BOOL                IsProcessCreated, CreateResult;
	PWSTR               NewCommandLine;

	LOOP_ONCE
	{
		if (ApplicationName == NULL || CommandLine == NULL)
			break;

		NewCommandLine = (PWSTR)AllocStack(0x2000);
		RtlZeroMemory(NewCommandLine, 0x2000);

		Argv             = NULL;
		IsProcessCreated = FALSE;

		if (ApplicationName != NULL)
		{
			Application = ApplicationName;
		}
		else
		{
			Argv = CmdLineToArgvW(CommandLine, &Argc);
			if (Argv) Application = Argv[0];
		}

		Status = File.Open(Application);
		if (NT_FAILED(Status))
			goto CREATE_CLEANUP;

		Size = File.GetSize32();
		Buffer = (PBYTE)AllocateMemoryP(Size);
		if (!Buffer)
			goto CREATE_CLEANUP;

		File.Read(Buffer, Size);
		File.Close();

		static WCHAR Krkr2Magic[] = L"TVP(KIRIKIRI) 2 core / Scripting Platform for Win32";
		static WCHAR KrkrzMagic[] = L"TVP(KIRIKIRI) Z core / Scripting Platform for Win32";

		Ptr = KMP(Buffer, Size, KrkrzMagic, sizeof(KrkrzMagic) - sizeof(KrkrzMagic[0]));
		if (!Ptr)
			Ptr = KMP(Buffer, Size, KrkrzMagic, sizeof(Krkr2Magic) - sizeof(Krkr2Magic[0]));

		NtHeaders32 = (PIMAGE_NT_HEADERS32)ImageNtHeaders(Buffer, &NtHeadersVersion);
		NtHeaders64 = (PIMAGE_NT_HEADERS64)NtHeaders32;

		switch (NtHeadersVersion)
		{
		case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
			IsWow64Process = TRUE;

			if (Argv)
				FormatStringW(NewCommandLine, L"KrkrExtract.exe >c %s", CommandLine);
			else
				FormatStringW(NewCommandLine, L"KrkrExtract.exe >a %s", ApplicationName);

			CreateResult = StubCreateProcessInternalW(
				Token,
				NULL,
				NewCommandLine,
				ProcessAttributes,
				ThreadAttributes,
				InheritHandles,
				CreationFlags,
				Environment,
				CurrentDirectory,
				StartupInfo,
				ProcessInformation,
				NewToken);

			IsProcessCreated = TRUE;
			break;

		case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
			IsWow64Process = FALSE;
			if (Ptr)
				CreateResult = CreateProcessInternalWithDll_X64(
				Token,
				ApplicationName,
				CommandLine,
				L"KrkrExtract64.dll",
				ProcessAttributes,
				ThreadAttributes,
				InheritHandles,
				CreationFlags,
				Environment,
				CurrentDirectory,
				StartupInfo,
				ProcessInformation,
				NewToken);
			else
				CreateResult = CreateProcessInternalWithDll_X64(
				Token,
				ApplicationName,
				CommandLine,
				L"KrkrHelper.dll",
				ProcessAttributes,
				ThreadAttributes,
				InheritHandles,
				CreationFlags,
				Environment,
				CurrentDirectory,
				StartupInfo,
				ProcessInformation,
				NewToken);

			IsProcessCreated = TRUE;
			break;

		default:
			goto CREATE_CLEANUP;
		}

		FreeMemoryP(Buffer);

	CREATE_CLEANUP:
		File.Close();
		if (Argv)
			ReleaseArgv(Argv);

		if (IsProcessCreated)
			return CreateResult;
	}

	return StubCreateProcessInternalW(
		Token,
		ApplicationName,
		CommandLine,
		ProcessAttributes,
		ThreadAttributes,
		InheritHandles,
		CreationFlags,
		Environment,
		CurrentDirectory,
		StartupInfo,
		ProcessInformation,
		NewToken);
}


BOOL InitKrkrExtract = FALSE;

PVOID WINAPI HookLoadLibraryA(LPCSTR lpFileName)
{
	PVOID   Result;
	PWSTR   UnicodeName;
	ULONG   Length, OutLength;

	Length = (StrLengthA(lpFileName) + 1) * 2;
	UnicodeName = (PWSTR)AllocStack(Length);

	RtlZeroMemory(UnicodeName, Length);
	RtlMultiByteToUnicodeN(UnicodeName, Length, &OutLength, lpFileName, Length / 2 - 1);

	Result = Nt_LoadLibrary(UnicodeName);
	if (LookupExportTable(Result, "V2Link") == IMAGE_INVALID_VA)
		return Result;

	if (!InitKrkrExtract)
	{
		InterlockedExchange((volatile LONG*)&InitKrkrExtract, TRUE);
	}

	return Result;
}


PVOID WINAPI HookLoadLibraryW(LPCWSTR lpFileName)
{
	PVOID   Result;

	Result = Nt_LoadLibrary((PWSTR)lpFileName);

	if (LookupExportTable(Result, "V2Link") == IMAGE_INVALID_VA)
		return Result;

	if (!InitKrkrExtract)
	{
		InterlockedExchange((volatile LONG*)&InitKrkrExtract, TRUE);
	}

	return Result;
}



BOOL FASTCALL Init(HMODULE hModule)
{
	NTSTATUS Status;

	Status = ml::MlInitialize();
	if (NT_FAILED(Status))
		return NT_SUCCESS(Status);

	
	Mp::PATCH_MEMORY_DATA p[]=
	{
		Mp::FunctionJumpVa(CreateProcessInternalW, HookCreateProcessInternalW, &StubCreateProcessInternalW),
		Mp::FunctionJumpVa(LoadLibraryA,           HookLoadLibraryA,           NULL),
		Mp::FunctionJumpVa(LoadLibraryW,           HookLoadLibraryW,           NULL)
	};
	
	Status = Mp::PatchMemory(p, countof(p));
	return NT_SUCCESS(Status);
}



BOOL FASTCALL UnInit(HMODULE hModule)
{
	NTSTATUS Status;
	
	Mp::PATCH_MEMORY_DATA p[] =
	{
		Mp::FunctionJumpVa(CreateProcessInternalW, HookCreateProcessInternalW, &StubCreateProcessInternalW),
		Mp::FunctionJumpVa(LoadLibraryA,           HookLoadLibraryA,           NULL),
		Mp::FunctionJumpVa(LoadLibraryW,           HookLoadLibraryW,           NULL)
	};

	Mp::RestoreMemory(p, countof(p));

	Status = ml::MlUnInitialize();
	return NT_SUCCESS(Status);
}


BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		return Init(hModule);
	case DLL_PROCESS_DETACH:
		return UnInit(hModule);
	}
	return TRUE;
}

