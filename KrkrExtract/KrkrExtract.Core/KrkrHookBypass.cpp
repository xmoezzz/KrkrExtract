#include "KrkrExtract.h"


PVOID NTAPI KrkrExtractCore::GetProcAddressBypass(
	PVOID Module,
	PCSTR RoutineName
)
{
	if (m_IsGetProcAddressHooked && m_GetProcAddress) {
		return m_GetProcAddress(
			static_cast<HMODULE>(Module),
			RoutineName
		);
	}

	return Nt_GetProcAddress(
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
	if (m_IsCreateProcessInternalWHooked && m_CreateProcessInternalW) {
		return m_CreateProcessInternalW(
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

	return CreateProcessInternalW(
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
	if (m_IsMultiByteToWideCharHooked && m_MultiByteToWideChar) {
		return m_MultiByteToWideChar(
			CodePage,
			dwFlags,
			lpMultiByteStr,
			cbMultiByte,
			lpWideCharStr,
			cchWideChar
		);
	}

	return MultiByteToWideCharBypass(
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
	if (m_IsCreateFileWHooked && m_CreateFileW) {
		return m_CreateFileW(
			lpFileName,
			dwDesiredAccess,
			dwShareMode,
			lpSecurityAttributes,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile
		);
	}

	return CreateFileW(
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
	if (m_IsReadFileHooked && m_ReadFile) {
		return m_ReadFile(
			hFile,
			lpBuffer,
			nNumberOfBytesToRead,
			lpNumberOfBytesRead,
			lpOverlapped
		);
	}

	return ReadFile(
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
	if (m_IsIsDebuggerPresentHooked && m_IsDebuggerPresent) {
		return m_IsDebuggerPresent();
	}

	return Nt_CurrentPeb()->BeingDebugged;
}

PVOID NTAPI KrkrExtractCore::LoadLibraryABypass(
	PCSTR LibFileName
)
{
	PWSTR            UnicodeName;
	ULONG            Length, OutLength;

	Length = (StrLengthA(LibFileName) + 1) * 2;
	UnicodeName = (PWSTR)AllocStack(Length);

	RtlZeroMemory(UnicodeName, Length);
	RtlMultiByteToUnicodeN(UnicodeName, Length, &OutLength, (PSTR)LibFileName, Length / 2 - 1);

	return Nt_LoadLibrary(UnicodeName);
}

PVOID NTAPI KrkrExtractCore::LoadLibraryWBypass(
	PCWSTR LibFileName
)
{
	return Nt_LoadLibrary(LibFileName);
}

BOOL NTAPI KrkrExtractCore::IsDBCSLeadByteBypass(
	BYTE TestChar
)
{
	if (m_IsDBCSLeadByteHooked && m_IsDBCSLeadByte) {
		return m_IsDBCSLeadByte(TestChar);
	}

	return IsDBCSLeadByteEx(GetACP(), TestChar);
}

