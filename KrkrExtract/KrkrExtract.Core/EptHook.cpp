#include "KrkrHook.h"

NTSTATUS KrkrHook::HookGetProcAddressEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS KrkrHook::UnHookGetProcAddressEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS KrkrHook::HookCreateProcessInternalWEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::UnHookCreateProcessInternalWEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::HookMultiByteToWideCharEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::UnHookMultiByteToWideCharEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::HookCreateFileWEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::UnHookCreateFileWEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::HookReadFileEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::UnHookReadFileEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::HookIsDebuggerPresentEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::UnHookIsDebuggerPresentEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::HookLoadLibraryAEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::UnHookLoadLibraryAEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::HookLoadLibraryWEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KrkrHook::UnHookLoadLibraryWEpt()
{
	SectionProtector Section(m_Lock.Get());

	return STATUS_NOT_IMPLEMENTED;
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

