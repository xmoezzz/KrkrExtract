#include <my.h>


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

	RtlInitUnicodeString(&FullDllPath, (PWSTR)lpDllPath);

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


int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR    lpCmdLine,
	int       nShowCmd
)
{
	PWSTR*              Argv;
	INT                 Argc;
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;
	BOOL                CreateResult;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	Argv = CommandLineToArgvW(lpCmdLine, &Argc);
	if (Argv == NULL || Argc < 1)
	{
		LocalFree(Argv);
		return 0;
	}

	StubCreateProcessInternalW = (FuncCreateProcessInternalW)EATLookupRoutineByHashPNoFix(GetKernel32Handle(), KERNEL32_CreateProcessInternalW);

	CreateResult = CreateProcessInternalWithDll(Argv[0]);
	if (!CreateResult)
	{
		MessageBoxW(NULL, L"Couldn't Launch KrkrExtract", L"KrkrExtract", MB_OK | MB_ICONERROR);
	}

	LocalFree(Argv);
	return 0;
}




