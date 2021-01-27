#include "KrkrHook.h"
#include "KrkrExtract.h"
#include "Helper.h"
#include "tp_stub.h"
#include "Decoder.h"
#include "Prototype.h"
#include <Psapi.h>

static KrkrHook* g_HookEngine = nullptr;

KrkrHook* KrkrHook::GetInstance()
{
	return g_HookEngine;
}

KrkrHook::KrkrHook(KrkrHookExporter* MasterInstance, HookMode Mode) :
	m_Mode(Mode)
{
	RtlZeroMemory(m_BreakPointAddresses, sizeof(m_BreakPointAddresses));

	InterlockedExchange((volatile PSIZE_T)&m_MasterInstance, (SIZE_T)MasterInstance);
	InterlockedExchange((volatile PSIZE_T)&g_HookEngine,     (SIZE_T)this);
	
	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:

		//
		// ensure current process is under hypervisor mode
		//
		break;

	case HookMode::HOOK_NATIVE:

		//
		// no nothing here
		//
		
		break;

	default:
		PrintConsoleW(L"Unknown hook mode : %s\n", (ULONG)m_Mode);
		break;
	}
}

KrkrHook::~KrkrHook()
{
	for (ULONG i = 0; i < countof(m_BreakPoint); i++) {
		m_BreakPoint[i].Clear();
	}

	InterlockedExchange((volatile PSIZE_T)&m_MasterInstance, (SIZE_T)0);
	InterlockedExchange((volatile PSIZE_T)&g_HookEngine,     (SIZE_T)0);
}


KrkrHookExporter* KrkrHook::GetKrkrInstance()
{
	return m_MasterInstance;
}


//
// Hook helper
//

NTSTATUS KrkrHook::HookGetProcAddress()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookGetProcAddressEpt();
		break;
		
	case HookMode::HOOK_NATIVE:
		Status = HookGetProcAddressNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::UnHookGetProcAddress()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookGetProcAddressEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookGetProcAddressNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::HookCreateProcessInternalW()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookCreateProcessInternalWEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = HookCreateProcessInternalWNative();
		break;
	}

	return Status;
}

NTSTATUS KrkrHook::UnHookCreateProcessInternalW()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookCreateProcessInternalWEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookCreateProcessInternalWNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::HookMultiByteToWideChar()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookMultiByteToWideCharEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = HookMultiByteToWideCharNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::UnHookMultiByteToWideChar()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookMultiByteToWideCharEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookMultiByteToWideCharNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::HookCreateFileW()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookCreateFileWEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = HookCreateFileWNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::UnHookCreateFileW()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookCreateFileWEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookCreateFileWNative();
		break;
	}

	return Status;
}

NTSTATUS KrkrHook::HookReadFile()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookReadFileEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = HookReadFileNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::UnHookReadFile()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookReadFileEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookReadFileNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::HookIsDebuggerPresent()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookIsDebuggerPresentEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = HookIsDebuggerPresentNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::UnHookIsDebuggerPresent()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookIsDebuggerPresentEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookIsDebuggerPresentNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::HookLoadLibraryA()
{
	NTSTATUS Status = STATUS_SUCCESS;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookLoadLibraryAEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = HookLoadLibraryANative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::UnHookLoadLibraryA()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookLoadLibraryAEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookLoadLibraryANative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::HookLoadLibraryW()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookLoadLibraryWEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = HookLoadLibraryWNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::UnHookLoadLibraryW()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookLoadLibraryWEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookLoadLibraryWNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::HookTVPGetFunctionExporter()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookTVPGetFunctionExporterEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = HookTVPGetFunctionExporterNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::UnHookTVPGetFunctionExporter()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookTVPGetFunctionExporterEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookTVPGetFunctionExporterNative();
		break;
	}

	return Status;
}

NTSTATUS KrkrHook::HookTVPSetXP3ArchiveExtractionFilter()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookTVPSetXP3ArchiveExtractionFilterEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = HookTVPSetXP3ArchiveExtractionFilterNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::UnHookTVPSetXP3ArchiveExtractionFilter()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookTVPSetXP3ArchiveExtractionFilterEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookTVPSetXP3ArchiveExtractionFilterNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::HookV2Link(PVOID Module)
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookV2LinkEpt(Module);
		break;

	case HookMode::HOOK_NATIVE:
		Status = HookV2LinkNative(Module);
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::UnHookV2Link()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookV2LinkEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookV2LinkNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::HookIsDBCSLeadByte()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = HookIsDBCSLeadByteEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = HookIsDBCSLeadByteNative();
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::UnHookIsDBCSLeadByte()
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = UnHookIsDBCSLeadByteEpt();
		break;

	case HookMode::HOOK_NATIVE:
		Status = UnHookIsDBCSLeadByteNative();
		break;
	}

	return Status;
}



NTSTATUS KrkrHook::SetHwBreakPointAt(PVOID Address, SIZE_T Size, HardwareBreakpoint::Condition AccessStatus, INT& Index)
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	Index = (ULONG)-1;

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = SetHwBreakPointEpt(Address, Size, AccessStatus, Index);
		break;

	case HookMode::HOOK_NATIVE:
		Status = SetHwBreakPointNative(Address, Size, AccessStatus, Index);
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::RemoveHwBreakPointAt(INT Index)
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = RemoveHwBreakPointEpt(Index);
		break;

	case HookMode::HOOK_NATIVE:
		Status = RemoveHwBreakPointNative(Index);
		break;
	}

	return Status;
}

NTSTATUS KrkrHook::RemoveAllHwBreakPointAt(PCONTEXT Context)
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = RemoveAllHwBreakPointEpt(Context);
		break;

	case HookMode::HOOK_NATIVE:
		Status = RemoveAllHwBreakPointNative(Context);
		break;
	}

	return Status;
}


NTSTATUS KrkrHook::GetBusyHwBreakPoint(_Out_ HwBreakPointStatus& BpStatus)
{
	NTSTATUS         Status;
	SectionProtector Section(m_Lock.Get());

	switch (m_Mode)
	{
	case HookMode::HOOK_EPT:
		Status = GetBusyHwBreakPointEpt(BpStatus);
		break;

	case HookMode::HOOK_NATIVE:
		Status = GetBusyHwBreakPointNative(BpStatus);
		break;
	}

	return Status;
}



