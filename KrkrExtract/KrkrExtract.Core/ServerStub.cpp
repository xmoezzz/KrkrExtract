#include "KrkrExtract.h"
#include <RpcSequence.h>
#include <Utf.Convert.h>


BOOL NTAPI KrkrExtractCore::TellServerProgressBar(PCWSTR TaskName, ULONGLONG Current, ULONGLONG Total)
{
	BOOL LocalState, RemoteState;
	BOOL Success = FALSE;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerProgressBar(TaskName, Current, Total);
		break;

	case KrkrRunMode::REMOTE_MODE:
		if (m_ConnectionApi) {
			Success = m_ConnectionApi->NotifyServerProgressBar(TaskName, Current, Total);
		}
		break;

	case KrkrRunMode::MIXED_MODE:
		if (m_ConnectionApi) {
			RemoteState = m_ConnectionApi->NotifyServerProgressBar(TaskName, Current, Total);
		}
		LocalState = m_LocalServer->NotifyServerProgressBar(TaskName, Current, Total);
		Success = RemoteState && LocalState;
		break;

	default:
		PrintConsoleW(L"TellServerProgressBar: Unknown mode %d\n", (INT)m_RunMode.load());
		break;
	}

	return Success;
}


BOOL NTAPI KrkrExtractCore::TellServerLogOutput(LogLevel Level, PCWSTR FormatString, ...)
{
	BOOL        LocalState = FALSE;
	BOOL        RemoteState = FALSE;
	BOOL        Success = FALSE;
	WCHAR       Message[0x200];
	va_list     Args;

	va_start(Args, FormatString);
	FormatStringvnW(Message, countof(Message) - 1, FormatString, Args);

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerLogOutput(Level, Message);
		break;

	case KrkrRunMode::REMOTE_MODE:
		if (m_ConnectionApi) {
			Success = m_ConnectionApi->NotifyServerLogOutput(Level, Message);
		}
		break;

	case KrkrRunMode::MIXED_MODE:
		if (m_ConnectionApi) {
			RemoteState = m_ConnectionApi->NotifyServerLogOutput(Level, Message);
		}
		LocalState  = m_LocalServer->NotifyServerLogOutput(Level, Message);
		Success = RemoteState && LocalState;
		break;

	default:
		PrintConsoleW(L"TellServerLogOutput: Unknown mode %d\n", (INT)m_RunMode.load());
		break;
	}

	return Success;
}


BOOL NTAPI KrkrExtractCore::TellServerCommandResultOutput(CommandStatus Status, PCWSTR FormatString, ...)
{
	BOOL        LocalState = FALSE;
	BOOL        RemoteState = FALSE;
	BOOL        Success = FALSE;
	WCHAR       Message[0x200];
	va_list     Args;

	va_start(Args, FormatString);
	FormatStringvnW(Message, countof(Message) - 1, FormatString, Args);

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerCommandResultOutput(Status, Message);
		break;

	case KrkrRunMode::REMOTE_MODE:
		if (m_ConnectionApi) {
			Success = m_ConnectionApi->NotifyServerCommandResultOutput(Status, Message);
		}
		break;

	case KrkrRunMode::MIXED_MODE:
		if (m_ConnectionApi) {
			RemoteState = m_ConnectionApi->NotifyServerCommandResultOutput(Status, Message);
		}
		LocalState  = m_LocalServer->NotifyServerCommandResultOutput(Status, Message);
		Success = RemoteState && LocalState;
		break;

	default:
		PrintConsoleW(L"TellServerCommandResultOutput: Unknown mode %d\n", (INT)m_RunMode.load());
		break;
	}

	return Success;
}


BOOL NTAPI KrkrExtractCore::TellServerUIReady(
	ULONG ClientPort,
	PCSTR SessionKey,
	ULONG Extra
)
{
	BOOL  LocalState = FALSE;
	BOOL  RemoteState = FALSE;
	BOOL  Success = FALSE;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerUIReady(ClientPort, SessionKey, Extra);
		break;

	case KrkrRunMode::REMOTE_MODE:
		if (m_ConnectionApi) {
			Success = m_ConnectionApi->NotifyServerUIReady(ClientPort, SessionKey, Extra);
		}
		break;

	case KrkrRunMode::MIXED_MODE:
		if (m_ConnectionApi) {
			LocalState = m_ConnectionApi->NotifyServerUIReady(ClientPort, SessionKey, Extra);
		}
		RemoteState = m_LocalServer->NotifyServerUIReady(ClientPort, SessionKey, Extra);
		Success = LocalState && RemoteState;
		break;

	default:
		PrintConsoleW(L"TellServerUIReady: Unknown mode %d\n", (INT)m_RunMode.load());
		break;
	}

	return Success;
}


BOOL NTAPI KrkrExtractCore::TellServerMessageBox(PCWSTR Description, ULONG Flags, BOOL Locked)
{
	BOOL  LocalState = FALSE;
	BOOL  RemoteState = FALSE;
	BOOL  Success = FALSE;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerMessageBox(Description, Flags, Locked);
		break;

	case KrkrRunMode::REMOTE_MODE:
		if (m_ConnectionApi) {
			Success = m_ConnectionApi->NotifyServerMessageBox(Description, Flags, Locked);
		}
		break;

	case KrkrRunMode::MIXED_MODE:
		if (m_ConnectionApi) {
			RemoteState = m_ConnectionApi->NotifyServerMessageBox(Description, Flags, Locked);
		}
		LocalState  = m_LocalServer->NotifyServerMessageBox(Description, Flags, Locked);
		Success = RemoteState && LocalState;
		break;

	default:
		PrintConsoleW(L"TellServerMessageBox: Unknown mode %d\n", (INT)m_RunMode.load());
		break;
	}

	return Success;
}


BOOL NTAPI KrkrExtractCore::TellServerTaskStartAndDisableUI()
{
	BOOL  LocalState = FALSE;
	BOOL  RemoteState = FALSE;
	BOOL  Success = FALSE;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerTaskStartAndDisableUI();
		break;

	case KrkrRunMode::REMOTE_MODE:
		if (m_ConnectionApi) {
			Success = m_ConnectionApi->NotifyServerTaskStartAndDisableUI();
		}
		break;

	case KrkrRunMode::MIXED_MODE:
		if (m_ConnectionApi) {
			RemoteState = m_ConnectionApi->NotifyServerTaskStartAndDisableUI();
		}
		LocalState = m_LocalServer->NotifyServerTaskStartAndDisableUI();
		Success = LocalState && RemoteState;
		break;

	default:
		PrintConsoleW(L"TellServerTaskStartAndDisableUI: Unknown mode %d\n", (INT)m_RunMode.load());
		break;
	}

	return Success;
}


BOOL NTAPI KrkrExtractCore::TellServerTaskEndAndEnableUI(BOOL TaskCompleteStatus, PCWSTR Description)
{
	BOOL  LocalState = FALSE;
	BOOL  RemoteState = FALSE;
	BOOL  Success = FALSE;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerTaskEndAndEnableUI(TaskCompleteStatus, Description);
		break;

	case KrkrRunMode::REMOTE_MODE:
		if (m_ConnectionApi) {
			Success = m_ConnectionApi->NotifyServerTaskEndAndEnableUI(TaskCompleteStatus, Description);
		}
		break;

	case KrkrRunMode::MIXED_MODE:
		if (m_ConnectionApi) {
			RemoteState = m_ConnectionApi->NotifyServerTaskEndAndEnableUI(TaskCompleteStatus, Description);
		}
		LocalState  = m_LocalServer->NotifyServerTaskEndAndEnableUI(TaskCompleteStatus, Description);
		Success = RemoteState && LocalState;
		break;

	default:
		PrintConsoleW(L"TellServerTaskEndAndEnableUI: Unknown mode %d\n", (INT)m_RunMode.load());
		break;
	}

	return Success;
}


BOOL NTAPI KrkrExtractCore::TellServerUIHeartbeatPackage()
{
	BOOL  LocalState = FALSE;
	BOOL  RemoteState = FALSE;
	BOOL  Success = FALSE;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerUIHeartbeatPackage();
		break;

	case KrkrRunMode::REMOTE_MODE:
		if (m_ConnectionApi) {
			Success = m_ConnectionApi->NotifyServerUIHeartbeatPackage();
		}
		break;

	case KrkrRunMode::MIXED_MODE:
		if (m_ConnectionApi) {
			RemoteState = m_ConnectionApi->NotifyServerUIHeartbeatPackage();
		}
		LocalState = m_LocalServer->NotifyServerUIHeartbeatPackage();
		Success = RemoteState && LocalState;
		break;

	default:
		PrintConsoleW(L"TellServerUIHeartbeatPackage: Unknown mode %d\n", (INT)m_RunMode.load());
		break;
	}

	return Success;
}


BOOL NTAPI KrkrExtractCore::TellServerExitFromRemoteProcess()
{
	BOOL  LocalState = FALSE;
	BOOL  RemoteState = FALSE;
	BOOL  Success = FALSE;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerExitFromRemoteProcess();
		break;

	case KrkrRunMode::REMOTE_MODE:
		if (m_ConnectionApi) {
			Success = m_ConnectionApi->NotifyServerExitFromRemoteProcess();
		}
		break;

	case KrkrRunMode::MIXED_MODE:
		if (m_ConnectionApi) {
			RemoteState = m_ConnectionApi->NotifyServerExitFromRemoteProcess();
		}
		LocalState  = m_LocalServer->NotifyServerExitFromRemoteProcess();
		Success = RemoteState && LocalState;
		break;

	default:
		PrintConsoleW(L"TellServerExitFromRemoteProcess: Unknown mode %d\n", (INT)m_RunMode.load());
		break;
	}

	return Success;
}


