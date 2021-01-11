#pragma once

#include <my.h>


BOOL RemoteProcessIs64Bits(PCWSTR FilePath, BOOL& Is64Bit);
BOOL PrepareShellCode(HANDLE Process, HANDLE Thread, PCWSTR DllPath);
BOOL PrepareShellCode_AMD64(HANDLE Process, HANDLE Thread, PCWSTR DllPath);



BOOL WINAPI CreateProcessWithDllW_ANY_TO_I386(
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	LPCWSTR lpDllFullPath
);


BOOL WINAPI CreateProcessWithDllW_ANYX86_TO_AMD64(
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	LPCWSTR lpDllFullPath
);


