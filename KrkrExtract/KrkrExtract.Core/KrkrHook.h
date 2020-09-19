#pragma once

#include "KrkrHookExporter.h"
#include "HardwareBreakpoint.h"
#include "SectionProtector.h"
#include <Prototype.h>
#include <atomic>
#include <set>

class KrkrHook
{
public:
	KrkrHook() = delete;
	KrkrHook(const KrkrHook&) = delete;
	KrkrHook(KrkrHookExporter* MasterInstance, HookMode Mode);
	~KrkrHook();

	static KrkrHook* GetInstance();

	//
	// Hook Routines
	//

	NTSTATUS HookGetProcAddress();
	NTSTATUS UnHookGetProcAddress();
	NTSTATUS HookCreateProcessInternalW();
	NTSTATUS UnHookCreateProcessInternalW();
	NTSTATUS HookMultiByteToWideChar();
	NTSTATUS UnHookMultiByteToWideChar();
	NTSTATUS HookCreateFileW();
	NTSTATUS UnHookCreateFileW();
	NTSTATUS HookReadFile();
	NTSTATUS UnHookReadFile();
	NTSTATUS HookIsDebuggerPresent();
	NTSTATUS UnHookIsDebuggerPresent();
	NTSTATUS HookLoadLibraryA();
	NTSTATUS UnHookLoadLibraryA();
	NTSTATUS HookLoadLibraryW();
	NTSTATUS UnHookLoadLibraryW();
	NTSTATUS HookTVPGetFunctionExporter();
	NTSTATUS UnHookTVPGetFunctionExporter();
	NTSTATUS HookTVPSetXP3ArchiveExtractionFilter();
	NTSTATUS UnHookTVPSetXP3ArchiveExtractionFilter();
	NTSTATUS HookV2Link(PVOID Module);
	NTSTATUS UnHookV2Link();
	NTSTATUS HookIsDBCSLeadByte();
	NTSTATUS UnHookIsDBCSLeadByte();

	//
	// Bypass
	//

	PVOID GetProcAddressBypass(
		PVOID Module, 
		PCSTR RoutineName
	);

	BOOL CreateProcessInternalWBypass(
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

	INT MultiByteToWideCharBypass(
		UINT    CodePage,
		DWORD   dwFlags,
		LPCCH   lpMultiByteStr,
		int     cbMultiByte,
		LPWSTR  lpWideCharStr,
		int     cchWideChar
	);

	HANDLE CreateFileWBypass(
		LPCWSTR               lpFileName,
		DWORD                 dwDesiredAccess,
		DWORD                 dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		DWORD                 dwCreationDisposition,
		DWORD                 dwFlagsAndAttributes,
		HANDLE                hTemplateFile
	);

	BOOL ReadFileBypass(
		HANDLE       hFile,
		LPVOID       lpBuffer,
		DWORD        nNumberOfBytesToRead,
		LPDWORD      lpNumberOfBytesRead,
		LPOVERLAPPED lpOverlapped
	);

	BOOL IsDebuggerPresentBypass(
	);

	PVOID LoadLibraryABypass(
		PCSTR LibFileName
	);

	PVOID LoadLibraryWBypass(
		PCWSTR LibFileName
	);

	BOOL IsDBCSLeadByteBypass(
		BYTE TestChar
	);

	//
	// Status
	//

	ForceInline BOOL IsMultiByteToWideCharHooked()              { return m_IsMultiByteToWideCharHooked.load(); }
	ForceInline BOOL IsGetProcAddressHooked()                   { return m_IsGetProcAddressHooked.load(); }
	ForceInline BOOL IsCreateProcessInternalWHooked()           { return m_IsCreateProcessInternalWHooked.load(); }
	ForceInline BOOL IsCreateFileWHooked()                      { return m_IsCreateFileWHooked.load(); }
	ForceInline BOOL IsReadFileHooked()                         { return m_IsReadFileHooked.load(); }
	ForceInline BOOL IsIsDebuggerPresentHooked()                { return m_IsIsDebuggerPresentHooked.load(); }
	ForceInline BOOL IsTVPGetFunctionExporterHooked()           { return m_IsTVPGetFunctionExporterHooked.load(); }
	ForceInline BOOL IsLoadLibraryAHooked()                     { return m_IsLoadLibraryAHooked.load(); }
	ForceInline BOOL IsLoadLibraryWHooked()                     { return m_IsLoadLibraryWHooked.load(); }
	ForceInline BOOL IsV2LinkHooked()                           { return m_IsV2LinkHooked.load(); }
	ForceInline BOOL IsTVPSetXP3ArchiveExtractionFilterHooked() { return m_IsTVPSetXP3ArchiveExtractionFilterHooked.load(); }
	ForceInline BOOL IsDBCSLeadByteHooked()                     { return m_IsDBCSLeadByteHooked.load(); }

	//
	// Addresses for hooking
	//

	API_POINTER(GetProcAddress)            m_GetProcAddress = nullptr;
	API_POINTER(CreateProcessInternalW)    m_CreateProcessInternalW = nullptr;
	API_POINTER(MultiByteToWideChar)       m_MultiByteToWideChar = nullptr;
	API_POINTER(CreateFileW)               m_CreateFileW = nullptr;
	API_POINTER(ReadFile)                  m_ReadFile = nullptr;
	API_POINTER(IsDebuggerPresent)         m_IsDebuggerPresent = nullptr;
	Prototype::TVPGetFunctionExporterFunc  m_TVPGetFunctionExporter = nullptr;
	Prototype::SetXP3FilterFunc            m_TVPSetXP3ArchiveExtractionFilter = nullptr;
	Prototype::V2LinkFunc                  m_V2Link = nullptr;
	API_POINTER(IsDBCSLeadByte)            m_IsDBCSLeadByte = nullptr;

	KrkrHookExporter* GetKrkrInstance();

	NTSTATUS SetHwBreakPointAt(PVOID Address, SIZE_T Size, HardwareBreakpoint::Condition AccessStatus, INT& Index);
	NTSTATUS RemoveHwBreakPointAt(INT Index);
	NTSTATUS RemoveAllHwBreakPointAt(PCONTEXT Context);
	NTSTATUS GetBusyHwBreakPoint(_Out_ HwBreakPointStatus& Status);
	

private:
	//
	// Native
	//
	NTSTATUS HookGetProcAddressNative();
	NTSTATUS UnHookGetProcAddressNative();
	NTSTATUS HookCreateProcessInternalWNative();
	NTSTATUS UnHookCreateProcessInternalWNative();
	NTSTATUS HookMultiByteToWideCharNative();
	NTSTATUS UnHookMultiByteToWideCharNative();
	NTSTATUS HookCreateFileWNative();
	NTSTATUS UnHookCreateFileWNative();
	NTSTATUS HookReadFileNative();
	NTSTATUS UnHookReadFileNative();
	NTSTATUS HookIsDebuggerPresentNative();
	NTSTATUS UnHookIsDebuggerPresentNative();
	NTSTATUS HookLoadLibraryANative();
	NTSTATUS UnHookLoadLibraryANative();
	NTSTATUS HookLoadLibraryWNative();
	NTSTATUS UnHookLoadLibraryWNative();
	NTSTATUS HookTVPGetFunctionExporterNative();
	NTSTATUS UnHookTVPGetFunctionExporterNative();
	NTSTATUS HookTVPSetXP3ArchiveExtractionFilterNative();
	NTSTATUS UnHookTVPSetXP3ArchiveExtractionFilterNative();
	NTSTATUS HookV2LinkNative(PVOID Module);
	NTSTATUS UnHookV2LinkNative();
	NTSTATUS HookIsDBCSLeadByteNative();
	NTSTATUS UnHookIsDBCSLeadByteNative();

	//
	// EPT (Hypervisor)
	//
	
	NTSTATUS HookGetProcAddressEpt();
	NTSTATUS UnHookGetProcAddressEpt();
	NTSTATUS HookCreateProcessInternalWEpt();
	NTSTATUS UnHookCreateProcessInternalWEpt();
	NTSTATUS HookMultiByteToWideCharEpt();
	NTSTATUS UnHookMultiByteToWideCharEpt();
	NTSTATUS HookCreateFileWEpt();
	NTSTATUS UnHookCreateFileWEpt();
	NTSTATUS HookReadFileEpt();
	NTSTATUS UnHookReadFileEpt();
	NTSTATUS HookIsDebuggerPresentEpt();
	NTSTATUS UnHookIsDebuggerPresentEpt();
	NTSTATUS HookLoadLibraryAEpt();
	NTSTATUS UnHookLoadLibraryAEpt();
	NTSTATUS HookLoadLibraryWEpt();
	NTSTATUS UnHookLoadLibraryWEpt();
	NTSTATUS HookTVPGetFunctionExporterEpt();
	NTSTATUS UnHookTVPGetFunctionExporterEpt();
	NTSTATUS HookTVPSetXP3ArchiveExtractionFilterEpt();
	NTSTATUS UnHookTVPSetXP3ArchiveExtractionFilterEpt();
	NTSTATUS HookV2LinkEpt(PVOID Module);
	NTSTATUS UnHookV2LinkEpt();
	NTSTATUS HookIsDBCSLeadByteEpt();
	NTSTATUS UnHookIsDBCSLeadByteEpt();

	//
	// Hardware breakpoints
	// Native
	//

	NTSTATUS SetHwBreakPointNative(PVOID Address, SIZE_T Size, HardwareBreakpoint::Condition AccessStatus, INT& Index);
	NTSTATUS RemoveHwBreakPointNative(INT Index);
	NTSTATUS RemoveAllHwBreakPointNative(PCONTEXT Context);
	NTSTATUS GetBusyHwBreakPointNative(_Out_ HwBreakPointStatus& Status);

	//
	// hypervisor
	//

	NTSTATUS SetHwBreakPointEpt(PVOID Address, SIZE_T Size, HardwareBreakpoint::Condition AccessStatus, INT& Index);
	NTSTATUS RemoveHwBreakPointEpt(INT Index);
	NTSTATUS RemoveAllHwBreakPointEpt(PCONTEXT Context);
	NTSTATUS GetBusyHwBreakPointEpt(_Out_ HwBreakPointStatus& Status);

private:
	KrkrHookExporter* m_MasterInstance = nullptr;
	HookMode          m_Mode           = HookMode::HOOK_NATIVE;

	//
	// Hook Status
	//

	std::atomic<BOOL> m_IsMultiByteToWideCharHooked = FALSE;
	std::atomic<BOOL> m_IsGetProcAddressHooked = FALSE;
	std::atomic<BOOL> m_IsCreateProcessInternalWHooked = FALSE;
	std::atomic<BOOL> m_IsCreateFileWHooked = FALSE;
	std::atomic<BOOL> m_IsReadFileHooked = FALSE;
	std::atomic<BOOL> m_IsIsDebuggerPresentHooked = FALSE;
	std::atomic<BOOL> m_IsTVPGetFunctionExporterHooked = FALSE;
	std::atomic<BOOL> m_IsLoadLibraryAHooked = FALSE;
	std::atomic<BOOL> m_IsLoadLibraryWHooked = FALSE;
	std::atomic<BOOL> m_IsV2LinkHooked = FALSE;
	std::atomic<BOOL> m_IsTVPSetXP3ArchiveExtractionFilterHooked = FALSE;
	std::atomic<BOOL> m_IsDBCSLeadByteHooked = FALSE;

	SectionLock       m_Lock;

public:
	//
	// Native
	//

	PVOID                                 m_ExceptionHandler = NULL;
	SectionLock                           m_ReadFileLocker;
	ULONG                                 m_RetAddr = 0;
	BOOL                                  m_SetBpOnce = FALSE;
	std::atomic<BOOL>                     m_BreakOnce = FALSE;

private:
	//
	// hardware breakpoints
	//

	HardwareBreakpoint                    m_BreakPoint[4];
	HwBreakPointStatus                    m_BreakPointIndexSet;
	ULONG_PTR                             m_BreakPointAddresses[4];
};


