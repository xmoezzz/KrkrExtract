#include <my.h>

#pragma comment(linker, "/ENTRY:MainEntry")
#pragma comment(linker, "/SECTION:.text,ERW /MERGE:.rdata=.text /MERGE:.data=.text")
#pragma comment(linker, "/SECTION:.Xmoe,ERW /MERGE:.text=.Xmoe")

#pragma comment(lib, "MyLibrary_x86.lib")


typedef
BOOL
(WINAPI
*FuncCreateProcessInternalW)(
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
);

BOOL
(WINAPI
*StubCreateProcessInternalW)(
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
);

BOOL
WINAPI
VMeCreateProcess(
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
	UNICODE_STRING   FullDllPath;

	RtlInitUnicodeString(&FullDllPath, lpDllPath);

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

	InjectDllToRemoteProcess(
		lpProcessInformation->hProcess,
		lpProcessInformation->hThread,
		&FullDllPath,
		IsSuspended
		);

	NtResumeThread(lpProcessInformation->hThread, NULL);

	return TRUE;
}


BOOL CheckDll()
{
	DWORD  Attribute;

	Attribute = GetFileAttributesW(L"KrkrExtract.dll");
	return Attribute != 0xFFFFFFFF;
}


BOOL FASTCALL CreateProcessInternalWithDll(LPCWSTR ProcessName)
{
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	return VMeCreateProcess(NULL, ProcessName, NULL, L"KrkrExtract.dll", NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi, NULL);
}

VOID CDECL MainEntry()
{
	NTSTATUS            Status;
	PWSTR*              Argv;
	LONG_PTR            Argc;
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;
	PWSTR               NewCommandLine, OldCommandLine;
	BOOL                CreateResult;
	ULONG               Length;

	ml::MlInitialize();
	Nt_SetExeDirectoryAsCurrent();
	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	StubCreateProcessInternalW = (FuncCreateProcessInternalW)EATLookupRoutineByHashPNoFix(GetKernel32Handle(), KERNEL32_CreateProcessInternalW);

	Argv = CmdLineToArgvW(Nt_GetCommandLine(), &Argc);
	if (Argc < 2)
	{
		ReleaseArgv(Argv);
		Ps::ExitProcess(0);
	}

	if (Argc == 3 && Argv[1][0] == L'>')
	{
		if (Argv[1][1] == L'a')
		{
			CreateResult = VMeCreateProcess(
				NULL, 
				Argv[2], 
				NULL,
				L"KrkrExtract.dll", 
				NULL, 
				NULL, 
				FALSE,
				NULL, 
				NULL, 
				NULL, 
				&si,
				&pi, 
				NULL);
		}
		else if (Argv[1][1] == L'c')
		{
			OldCommandLine = Nt_GetCommandLine();
			Length = StrLengthW(OldCommandLine);
			Length = ROUND_UP(Length, 4);
			NewCommandLine = (PWSTR)AllocStack(Length * 2);
			RtlZeroMemory(NewCommandLine, Length * 2);
			StrCopyW(NewCommandLine, &OldCommandLine[StrLengthW(Argv[0]) + 4]);

			CreateResult = VMeCreateProcess(
				NULL, 
				NULL, 
				NewCommandLine, 
				L"KrkrExtract.dll", 
				NULL, 
				NULL, 
				FALSE,
				NULL, 
				NULL, 
				NULL, 
				&si, 
				&pi, 
				NULL);
		}
	}
	else if (Argc == 2)
	{
		CreateResult = CreateProcessInternalWithDll(Argv[1]);
	}

	if (!CreateResult)
		MessageBoxW(NULL, L"Couldn't Launch KrkrExtract", L"KrkrExtract", MB_OK | MB_ICONERROR);

	ReleaseArgv(Argv);
	Ps::ExitProcess(0);
}




