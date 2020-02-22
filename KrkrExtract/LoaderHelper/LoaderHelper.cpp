#include <my.h>

#pragma comment(lib, "Shlwapi.lib")

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

enum KrkrMode
{
	NORMAL = 0,
	BASIC_LOCK = 1,
	ADV_LOCK = 2,
	HYPERVISOR = 3
};

#define KrkrModeVar L"KrkrMode"


#pragma comment(linker, "/EXPORT:CreateProcessInternalWithDll=_CreateProcessInternalWithDll@8")
EXTERN_C __declspec(dllexport)
BOOL NTAPI CreateProcessInternalWithDll(LPCWSTR ProcessName, KrkrMode Mode)
{
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;
	WCHAR               CurrentPath[MAX_PATH];
	WCHAR               Path[MAX_PATH];
	DWORD               Length;

	static WCHAR Postfix[] = L"\\KrkrExtract.dll";

	StubCreateProcessInternalW = (FuncCreateProcessInternalW)EATLookupRoutineByHashPNoFix(GetKernel32Handle(), KERNEL32_CreateProcessInternalW);

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	switch (Mode)
	{
	case NORMAL:
		SetEnvironmentVariableW(KrkrModeVar, L"NORMAL");
		break;
	case BASIC_LOCK:
		SetEnvironmentVariableW(KrkrModeVar, L"BASIC_LOCK");
		break;
	case ADV_LOCK:
		SetEnvironmentVariableW(KrkrModeVar, L"ADV_LOCK");
		break;
	case HYPERVISOR:
		SetEnvironmentVariableW(KrkrModeVar, L"HYPERVISOR");
		break;
	default:
		SetEnvironmentVariableW(KrkrModeVar, L"NORMAL");
		break;
	}

	RtlZeroMemory(CurrentPath, sizeof(CurrentPath));
	Length = GetCurrentDirectoryW(countof(CurrentPath), CurrentPath);

	RtlZeroMemory(Path, sizeof(Path));
	wnsprintfW(Path, countof(Path), L"%s%s", CurrentPath, Postfix);

	return VMeCreateProcess(NULL, ProcessName, NULL, Path, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi, NULL);
}




BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
    return TRUE;
}

