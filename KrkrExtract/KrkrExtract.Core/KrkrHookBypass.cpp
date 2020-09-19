#include "KrkrExtract.h"


PVOID NTAPI KrkrExtractCore::GetProcAddressBypass(
	PVOID Module,
	PCSTR RoutineName
)
{
	return m_HookEngine->GetProcAddressBypass(
		Module,
		RoutineName
	);
}

BOOL NTAPI KrkrExtractCore::CreateProcessBypass(
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
	return m_HookEngine->CreateProcessInternalWBypass(
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
		phNewToken
	);
}


INT NTAPI KrkrExtractCore::MultiByteToWideCharBypass(
	UINT    CodePage,
	DWORD   dwFlags,
	LPCCH   lpMultiByteStr,
	int     cbMultiByte,
	LPWSTR  lpWideCharStr,
	int     cchWideChar
)
{
	return m_HookEngine->MultiByteToWideCharBypass(
		CodePage,
		dwFlags,
		lpMultiByteStr,
		cbMultiByte,
		lpWideCharStr,
		cchWideChar
	);
}

HANDLE NTAPI KrkrExtractCore::CreateFileWBypass(
	LPCWSTR               lpFileName,
	DWORD                 dwDesiredAccess,
	DWORD                 dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD                 dwCreationDisposition,
	DWORD                 dwFlagsAndAttributes,
	HANDLE                hTemplateFile
)
{
	return m_HookEngine->CreateFileWBypass(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile
	);
}

BOOL NTAPI KrkrExtractCore::ReadFileBypass(
	HANDLE       hFile,
	LPVOID       lpBuffer,
	DWORD        nNumberOfBytesToRead,
	LPDWORD      lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped
)
{
	return m_HookEngine->ReadFileBypass(
		hFile,
		lpBuffer,
		nNumberOfBytesToRead,
		lpNumberOfBytesRead,
		lpOverlapped
	);
}

BOOL NTAPI KrkrExtractCore::IsDebuggerPresentBypass(
)
{
	return m_HookEngine->IsDebuggerPresentBypass();
}

PVOID NTAPI KrkrExtractCore::LoadLibraryABypass(
	PCSTR LibFileName
)
{
	return m_HookEngine->LoadLibraryABypass(
		LibFileName
	);
}

PVOID NTAPI KrkrExtractCore::LoadLibraryWBypass(
	PCWSTR LibFileName
)
{
	return m_HookEngine->LoadLibraryWBypass(
		LibFileName
	);
}

BOOL NTAPI KrkrExtractCore::IsDBCSLeadByteBypass(
	BYTE TestChar
)
{
	return m_HookEngine->IsDBCSLeadByteBypass(
		TestChar
	);
}

