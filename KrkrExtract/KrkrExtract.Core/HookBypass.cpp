#include "KrkrHook.h"
#include "SafeMemory.h"

PVOID KrkrHook::GetProcAddressBypass(
	PVOID Module,
	PCSTR RoutineName
)
{
	return Nt_GetProcAddress(Module, RoutineName);
}



//
// hooking the same memory page?
//

BOOL KrkrHook::CreateProcessInternalWBypass(
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
	API_POINTER(CreateProcessInternalW) CreateProcessInternalWPtr;

	if (m_IsCreateProcessInternalWHooked)
	{
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

	*(PVOID*)&CreateProcessInternalWPtr = Nt_GetProcAddress(GetKernel32Handle(), "CreateProcessInternalW");
	if (!CreateProcessInternalWPtr)
		return FALSE;

	return CreateProcessInternalWPtr(
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


INT KrkrHook::MultiByteToWideCharBypass(
	UINT    CodePage,
	DWORD   dwFlags,
	LPCCH   lpMultiByteStr,
	int     cbMultiByte,
	LPWSTR  lpWideCharStr,
	int     cchWideChar
)
{
	if (m_IsMultiByteToWideCharHooked)
	{
		return m_MultiByteToWideChar(
			CodePage,
			dwFlags,
			lpMultiByteStr,
			cbMultiByte,
			lpWideCharStr,
			cchWideChar
		);
	}

	return MultiByteToWideChar(
		CodePage,
		dwFlags,
		lpMultiByteStr,
		cbMultiByte,
		lpWideCharStr,
		cchWideChar
	);
}



HANDLE KrkrHook::CreateFileWBypass(
	LPCWSTR               lpFileName,
	DWORD                 dwDesiredAccess,
	DWORD                 dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD                 dwCreationDisposition,
	DWORD                 dwFlagsAndAttributes,
	HANDLE                hTemplateFile
)
{
	if (m_IsCreateFileWHooked)
	{
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

BOOL KrkrHook::ReadFileBypass(
	HANDLE       hFile,
	LPVOID       lpBuffer,
	DWORD        nNumberOfBytesToRead,
	LPDWORD      lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped
)
{
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;
	PVOID           ApcContext;
	LARGE_INTEGER   Offset;
	
	if (lpNumberOfBytesRead) {
		*lpNumberOfBytesRead = 0;
	}

	if ((ULONG)hFile >= 0xFFFFFFF4)
	{
		switch ((ULONG)hFile)
		{
		case STD_INPUT_HANDLE:
			hFile = Nt_CurrentPeb()->ProcessParameters->StandardInput;
			break;

		case STD_OUTPUT_HANDLE:
			hFile = Nt_CurrentPeb()->ProcessParameters->StandardOutput;
			break;

		case STD_ERROR_HANDLE:
			hFile = Nt_CurrentPeb()->ProcessParameters->StandardInput;
			break;
		}
	}

	if (lpOverlapped) 
	{
		lpOverlapped->Internal = STATUS_PENDING;
		Offset.LowPart  = lpOverlapped->Offset;
		Offset.HighPart = lpOverlapped->OffsetHigh;
		ApcContext      = (((ULONG_PTR)lpOverlapped->hEvent & 0x1) ? NULL : lpOverlapped);
		Status = NtReadFile(
			hFile, 
			lpOverlapped->hEvent, 
			nullptr, 
			ApcContext, 
			(PIO_STATUS_BLOCK)lpOverlapped, 
			lpBuffer, 
			nNumberOfBytesToRead, 
			&Offset, 
			nullptr
		);
		
		if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
		{
			if (Status == STATUS_END_OF_FILE && lpNumberOfBytesRead != NULL) {
				*lpNumberOfBytesRead = 0;
			}

			BaseSetLastNTError(Status);
			return FALSE;

		}

		if (lpNumberOfBytesRead != NULL) {
			*lpNumberOfBytesRead = lpOverlapped->InternalHigh;
		}
	}
	else
	{
		Status = NtReadFile(
			hFile,
			nullptr,
			nullptr,
			nullptr,
			&IoStatus,
			lpBuffer,
			nNumberOfBytesToRead,
			nullptr,
			nullptr
		);

		if (Status == STATUS_PENDING)
		{
			Status = NtWaitForSingleObject(hFile, FALSE, NULL);
			if (NT_SUCCESS(Status)) {
				Status = IoStatus.Status;
			}
		}

		if (Status == STATUS_END_OF_FILE)
		{
			if (lpNumberOfBytesRead) {
				*lpNumberOfBytesRead = 0;
			}

			return TRUE;
		}

		if (NT_SUCCESS(Status))
		{
			if (lpNumberOfBytesRead) {
				*lpNumberOfBytesRead = IoStatus.Information;
			}
		}
		else
		{
			BaseSetLastNTError(Status);
			return FALSE;
		}
	}

	return TRUE;
}

BOOL KrkrHook::IsDebuggerPresentBypass()
{
	return Nt_CurrentPeb()->BeingDebugged;
}

PVOID KrkrHook::LoadLibraryABypass(PCSTR LibFileName)
{
	NTSTATUS Status;
	ULONG    OutLength, Length;

	Length = (StrLengthA(LibFileName) + 1) * 2;

	auto UnicodeName = AllocateMemorySafeP<WCHAR>(Length);
	if (!UnicodeName)
		return nullptr;
	
	//
	// this routine always returns STATUS_SUCCESS
	//

	Status = RtlMultiByteToUnicodeN(UnicodeName.get(), Length, &OutLength, (PSTR)LibFileName, Length / 2 - 1);
	if (NT_FAILED(Status))
		return nullptr;

	return Nt_LoadLibrary(UnicodeName.get());
}

PVOID KrkrHook::LoadLibraryWBypass(PCWSTR LibFileName)
{
	return Nt_LoadLibrary((PWSTR)LibFileName);
}

BOOL KrkrHook::IsDBCSLeadByteBypass(BYTE TestChar)
{
	return IsDBCSLeadByteEx(GetACP(), TestChar);
}

