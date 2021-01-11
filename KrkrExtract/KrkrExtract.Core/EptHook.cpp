#include "KrkrHook.h"


//===================================================
// 
// Defined in NativeHook.cpp
//

INT
NTAPI
XeMultiByteToWideChar(
	UINT   CodePage,
	DWORD  dwFlags,
	LPCSTR lpMultiByteStr,
	int    cbMultiByte,
	LPWSTR lpWideCharStr,
	int    cchWideChar
);

PVOID
NTAPI
XeLoadLibraryA(LPCSTR FileName);

PVOID
NTAPI
XeLoadLibraryW(LPWSTR FileName);

FARPROC
WINAPI
XeGetProcAddress(HMODULE Module, LPCSTR ProcName);

HANDLE
WINAPI
XeCreateFileW(
	LPCWSTR               lpFileName,
	DWORD                 dwDesiredAccess,
	DWORD                 dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD                 dwCreationDisposition,
	DWORD                 dwFlagsAndAttributes,
	HANDLE                hTemplateFile
);


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
);

BOOL WINAPI XeIsDBCSLeadByte(BYTE TestChar);

BOOL
WINAPI
XeReadFile
(
	HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped
);

BOOL
NTAPI
XeIsDebuggerPresent();


//============================================
//
// hooks
//


NTSTATUS KrkrHook::HookGetProcAddressEpt()
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


NTSTATUS KrkrHook::UnHookGetProcAddressEpt()
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


NTSTATUS KrkrHook::HookCreateProcessInternalWEpt()
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

NTSTATUS KrkrHook::UnHookCreateProcessInternalWEpt()
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

NTSTATUS KrkrHook::HookMultiByteToWideCharEpt()
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

NTSTATUS KrkrHook::UnHookMultiByteToWideCharEpt()
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

NTSTATUS KrkrHook::HookCreateFileWEpt()
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

NTSTATUS KrkrHook::UnHookCreateFileWEpt()
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

NTSTATUS KrkrHook::HookReadFileEpt()
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

NTSTATUS KrkrHook::UnHookReadFileEpt()
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

NTSTATUS KrkrHook::HookIsDebuggerPresentEpt()
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

NTSTATUS KrkrHook::UnHookIsDebuggerPresentEpt()
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

NTSTATUS KrkrHook::HookLoadLibraryAEpt()
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

NTSTATUS KrkrHook::UnHookLoadLibraryAEpt()
{
	if (!m_IsLoadLibraryAHooked)
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS;
}

NTSTATUS KrkrHook::HookLoadLibraryWEpt()
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

NTSTATUS KrkrHook::UnHookLoadLibraryWEpt()
{
	if (!m_IsLoadLibraryWHooked)
		return STATUS_UNSUCCESSFUL;


	return STATUS_SUCCESS;
}

NTSTATUS KrkrHook::HookTVPGetFunctionExporterEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::UnHookTVPGetFunctionExporterEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::HookTVPSetXP3ArchiveExtractionFilterEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::UnHookTVPSetXP3ArchiveExtractionFilterEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::HookV2LinkEpt(PVOID Module)
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::UnHookV2LinkEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS KrkrHook::HookIsDBCSLeadByteEpt()
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::UnHookIsDBCSLeadByteEpt()
{
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS KrkrHook::SetHwBreakPointEpt(PVOID Address, SIZE_T Size, HardwareBreakpoint::Condition AccessStatus, INT& Index)
{
	SectionProtector Section(m_Lock.Get());

	Index = -1;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::RemoveHwBreakPointEpt(INT Index)
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::RemoveAllHwBreakPointEpt(PCONTEXT Context)
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::GetBusyHwBreakPointEpt(_Out_ HwBreakPointStatus& Status)
{
	SectionProtector Section(m_Lock.Get());

	Status = HwBreakPointStatus::SlotEmpty;
	return STATUS_NOT_IMPLEMENTED;
}

