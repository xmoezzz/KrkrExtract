#include <my.h>

#pragma pack(push, 1)
typedef struct SHELL_DATA
{
	ULONG64      Maigc1;
	ULONG64      Magic2;
	WCHAR        FileName[MAX_PATH];
}SHELL_DATA, *PSHELL_DATA;
#pragma pack(pop)

SHELL_DATA LinkerData
{
	(ULONG64)TAG8('Krkr', 'Info'),
	(ULONG64)TAG8('Xmoe', 'Anzu')
};


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

	StubCreateProcessInternalW = (FuncCreateProcessInternalW)EATLookupRoutineByHashPNoFix(GetKernel32Handle(), KERNEL32_CreateProcessInternalW);


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

BOOL FASTCALL CheckAndCreateProcess()
{
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	return VMeCreateProcess(NULL, NULL, LinkerData.FileName, L"KrkrUniversalPatch.dll", NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi, NULL);
}


int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR    lpCmdLine,
	int       nShowCmd)
{
	PWSTR*              Argv;
	INT                 Argc;
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;
	ULONG               Length;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	Argv = CommandLineToArgvW(lpCmdLine, &Argc);
	if (Argv == NULL || Argc < 1)
	{
		LocalFree(Argv);
		return 0;
	}

	if (!CheckAndCreateProcess())
		MessageBoxW(NULL, L"Couldn't Launch Game", L"Krkr Universal Patch", MB_OK | MB_ICONERROR);

	LocalFree(Argv);
	return 0;
}

