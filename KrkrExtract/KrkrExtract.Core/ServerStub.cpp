#include "KrkrExtract.h"

#include <RpcSequence.h>
#include <Utf.Convert.h>
#include <ServerProgressBar.pb.h>
#include <ServerLogOutput.pb.h>
#include <ServerCommandResultOutput.pb.h>
#include <ServerUIReady.pb.h>
#include <ServerMessageBox.pb.h>
#include <ServerTaskStartAndDisableUI.pb.h>
#include <ServerTaskEndAndEnableUI.pb.h>
#include <ServerUIHeartbeatPackage.pb.h>
#include <ServerExitFromRemoteProcess.pb.h>

std::string RpcTellServerProgressBar(PCWSTR TaskName, ULONGLONG Current, ULONGLONG Total)
{
	ServerProgressBar Data;
	
	Data.set_sequenceid((uint64_t)ServerSequence::SID_ServerProgressBar);
	Data.set_taskname(Utf16ToUtf8(TaskName));
	Data.set_current(Current);
	Data.set_total(Total);
	
	return Data.SerializeAsString();
}

BOOL NTAPI KrkrExtractCore::TellServerProgressBar(PCWSTR TaskName, ULONGLONG Current, ULONGLONG Total)
{
	BOOL Success;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerProgressBar(TaskName, Current, Total);
		break;

	default:
		Success = RpcSendToServer((ULONG)ServerSequence::SID_ServerProgressBar, RpcTellServerProgressBar(TaskName, Current, Total));
		break;
	}

	return Success;
}


std::string RpcServerLogOutput(LogLevel Level, PCWSTR Command)
{
	ServerLogOutput Data;

	Data.set_sequenceid((uint64_t)ServerSequence::SID_ServerLogOutput);
	Data.set_level((uint64_t)Level);
	Data.set_command(Utf16ToUtf8(Command).c_str());
	return Data.SerializeAsString();
}

BOOL NTAPI KrkrExtractCore::TellServerLogOutput(LogLevel Level, PCWSTR FormatString, ...)
{
	BOOL        Success;
	WCHAR       Message[0x200];
	va_list     Args;

	va_start(Args, FormatString);
	FormatStringvnW(Message, countof(Message) - 1, FormatString, Args);

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerLogOutput(Level, Message);
		break;

	default:
		Success = RpcSendToServer((ULONG)ServerSequence::SID_ServerLogOutput, RpcServerLogOutput(Level, Message));
		break;
	}

	return Success;
}


std::string RpcTellServerCommandResultOutput(CommandStatus Status, PCWSTR Reply)
{
	ServerCommandResultOutput Data;

	Data.set_sequenceid((uint64_t)ServerSequence::SID_ServerCommandResultOutput);
	Data.set_status((uint64_t)Status);
	Data.set_reply(Utf16ToUtf8(Reply).c_str());
	return Data.SerializeAsString();
}

BOOL NTAPI KrkrExtractCore::TellServerCommandResultOutput(CommandStatus Status, PCWSTR FormatString, ...)
{
	BOOL        Success;
	WCHAR       Message[0x200];
	va_list     Args;

	va_start(Args, FormatString);
	FormatStringvnW(Message, countof(Message) - 1, FormatString, Args);

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerCommandResultOutput(Status, Message);
		break;

	default:
		Success = RpcSendToServer((ULONG)ServerSequence::SID_ServerCommandResultOutput, RpcTellServerCommandResultOutput(Status, Message));
		break;
	}

	return Success;
}


std::string RpcServerUIReady()
{
	ServerUIReady Data;
	
	Data.set_sequenceid((uint64_t)ServerSequence::SID_ServerUIReady);
	return Data.SerializeAsString();
}

BOOL NTAPI KrkrExtractCore::TellServerUIReady()
{
	BOOL Success;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerUIReady();
		break;

	default:
		Success = RpcSendToServer((ULONG)ServerSequence::SID_ServerUIReady, RpcServerUIReady());
		break;
	}

	return Success;
}

std::string RpcServerMessageBox(PCWSTR Description, ULONG Flags, BOOL Locked)
{
	ServerMessageBox Data;

	Data.set_sequenceid((uint64_t)ServerSequence::SID_ServerMessageBox);
	Data.set_flags(Flags);
	Data.set_locked(Locked);

	return Data.SerializeAsString();
}

BOOL NTAPI KrkrExtractCore::TellServerMessageBox(PCWSTR Description, ULONG Flags, BOOL Locked)
{
	BOOL Success;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerMessageBox(Description, Flags, Locked);
		break;

	default:
		Success = RpcSendToServer((ULONG)ServerSequence::SID_ServerMessageBox, RpcServerMessageBox(Description, Flags, Locked));
		break;
	}

	return Success;
}


std::string RpcServerTaskStartAndDisableUI()
{
	ServerTaskStartAndDisableUI Data;

	Data.set_sequenceid((uint64_t)ServerSequence::SID_ServerTaskStartAndDisableUI);
	return Data.SerializeAsString();
}

BOOL NTAPI KrkrExtractCore::TellServerTaskStartAndDisableUI()
{
	BOOL Success;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerTaskStartAndDisableUI();
		break;

	default:
		Success = RpcSendToServer((ULONG)ServerSequence::SID_ServerTaskStartAndDisableUI, RpcServerTaskStartAndDisableUI());
		break;
	}

	return Success;
}

std::string RpcServerTaskEndAndEnableUI(BOOL TaskCompleteStatus, PCWSTR Description)
{
	ServerTaskEndAndEnableUI Data;
	
	Data.set_sequenceid((uint64_t)ServerSequence::SID_ServerTaskEndAndEnableUI);
	return Data.SerializeAsString();
}

BOOL NTAPI KrkrExtractCore::TellServerTaskEndAndEnableUI(BOOL TaskCompleteStatus, PCWSTR Description)
{
	BOOL Success;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerTaskEndAndEnableUI(TaskCompleteStatus, Description);
		break;

	default:
		Success = RpcSendToServer((ULONG)ServerSequence::SID_ServerTaskEndAndEnableUI, RpcServerTaskEndAndEnableUI(TaskCompleteStatus, Description));
		break;
	}

	return Success;
}

std::string RpcServerUIHeartbeatPackage()
{
	ServerUIHeartbeatPackage Data;
	
	Data.set_sequenceid((uint64_t)ServerSequence::SID_ServerUIHeartbeatPackage);
	return Data.SerializeAsString();
}

BOOL NTAPI KrkrExtractCore::TellServerUIHeartbeatPackage()
{
	BOOL Success;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerUIHeartbeatPackage();
		break;

	default:
		Success = RpcSendToServer((ULONG)ServerSequence::SID_ServerUIHeartbeatPackage, RpcServerUIHeartbeatPackage());
		break;
	}

	return Success;
}


std::string RpcServerExitFromRemoteProcess()
{
	ServerExitFromRemoteProcess Data;
	
	Data.set_sequenceid((uint64_t)ServerSequence::SID_ServerExitFromRemoteProcess);
	return Data.SerializeAsString();
}

BOOL NTAPI KrkrExtractCore::TellServerExitFromRemoteProcess()
{
	BOOL Success;

	switch (m_RunMode.load())
	{
	case KrkrRunMode::LOCAL_MODE:
		Success = m_LocalServer->NotifyServerExitFromRemoteProcess();
		break;

	default:
		Success = RpcSendToServer(
			(ULONG)ServerSequence::SID_ServerExitFromRemoteProcess,
			RpcServerExitFromRemoteProcess());
		break;
	}

	return Success;
}


