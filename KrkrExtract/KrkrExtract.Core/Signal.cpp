#include "Signals.h"
#include "KrkrExtract.h"


//
// Deprecated
//

NTSTATUS NTAPI SignalUnhookLoadLibraryA(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->UnHookLoadLibraryA();
}


NTSTATUS NTAPI SignalUnhookLoadLibraryW(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->UnHookLoadLibraryW();
}


NTSTATUS NTAPI SignalUnhookTVPGetFunctionExporter(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->UnHookTVPGetFunctionExporter();
}

NTSTATUS NTAPI SignalUnhookV2Link(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->UnHookV2Link();
}

NTSTATUS NTAPI SignalUnhookTVPSetXP3ArchiveExtractionFilter(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->UnHookTVPSetXP3ArchiveExtractionFilter();
}

NTSTATUS NTAPI SignalUnhookGetProcAddress(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->HookGetProcAddress();
}

NTSTATUS NTAPI SignalUnhookCreateProcessInternalW(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->UnHookCreateProcessInternalW();
}

NTSTATUS NTAPI SignalUnhookMultiByteToWideChar(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->UnHookMultiByteToWideChar();
}

NTSTATUS NTAPI SignalUnhookCreateFileW(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->UnHookCreateFileW();
}

NTSTATUS NTAPI SignalUnhookReadFile(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->UnHookReadFile();
}

NTSTATUS NTAPI SignalUnhookIsDebuggerPresent(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->UnHookIsDebuggerPresent();
}

NTSTATUS NTAPI SignalRemoveAllHardwareBreakPoint(KrkrExtractCore* Handle, PVOID UserData)
{
	NTSTATUS Status;
	CONTEXT  Context;
	
	Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	Status = NtGetContextThread(NtCurrentThread(), &Context);
	if (NT_FAILED(Status))
		return Status;

	Status = Handle->GetHookEngine()->RemoveAllHwBreakPointAt(&Context);
	if (NT_FAILED(Status))
		return Status;

	Status = NtSetContextThread(NtCurrentThread(), &Context);
	return Status;
}

NTSTATUS NTAPI SignalRemoveHardwareBreakPoint0(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->RemoveHwBreakPointAt(0);
}

NTSTATUS NTAPI SignalRemoveHardwareBreakPoint1(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->RemoveHwBreakPointAt(1);
}

NTSTATUS NTAPI SignalRemoveHardwareBreakPoint2(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->RemoveHwBreakPointAt(2);
}

NTSTATUS NTAPI SignalRemoveHardwareBreakPoint3(KrkrExtractCore* Handle, PVOID UserData)
{
	return Handle->GetHookEngine()->RemoveHwBreakPointAt(3);
}

NTSTATUS NTAPI SignalRemovePrivateProcHandler(KrkrExtractCore* Handle, PVOID UserData)
{
	//
	// TODO
	//

	RtlRemoveVectoredExceptionHandler(UserData) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


