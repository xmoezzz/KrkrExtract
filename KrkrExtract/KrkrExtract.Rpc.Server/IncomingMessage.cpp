#include "ServerImpl.h"
#include <RpcSequence.h>
#include <ServerProgressBar.pb.h>
#include <ServerLogOutput.pb.h>
#include <ServerExitFromRemoteProcess.pb.h>
#include <ServerCommandResultOutput.pb.h>
#include <ServerUIReady.pb.h>
#include <ServerMessageBox.pb.h>
#include <ServerTaskStartAndDisableUI.pb.h>
#include <ServerTaskEndAndEnableUI.pb.h>
#include <ServerUIHeartbeatPackage.pb.h>
#include <ServerExitFromRemoteProcess.pb.h>
#include <Utf.Convert.h>

BOOL ServerImpl::ParseMessage(PVOID MessageBuffer, SIZE_T MessageSize)
{
	BOOL                        Success;
	UINT32                      MessageType;
	ServerProgressBar           ServerProgressBarMessage;
	ServerLogOutput             ServerLogOutputMessage;
	ServerCommandResultOutput   ServerCommandResultOutputMessage;
	ServerUIReady               ServerUIReadyMessage;
	ServerMessageBox            ServerMessageBoxMessage;
	ServerTaskStartAndDisableUI ServerTaskStartAndDisableUIMessage;
	ServerTaskEndAndEnableUI    ServerTaskEndAndEnableUIMessage;
	ServerUIHeartbeatPackage    ServerUIHeartbeatPackageMessage;
	ServerExitFromRemoteProcess ServerExitFromRemoteProcessMessage;

	if (MessageSize < 4)
		return FALSE;

	MessageType = *(PUINT32)(MessageBuffer);

	switch ((ServerSequence)MessageType)
	{
	case ServerSequence::SID_ServerProgressBar:
		Success = ServerProgressBarMessage.ParseFromArray((PBYTE)MessageBuffer + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return FALSE;

		return NotifyServerProgressBar(
			Utf8ToUtf16(ServerProgressBarMessage.taskname()).c_str(),
			ServerProgressBarMessage.current(), 
			ServerProgressBarMessage.total()
		);

	case ServerSequence::SID_ServerLogOutput:
		Success = ServerLogOutputMessage.ParseFromArray((PBYTE)MessageBuffer + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return FALSE;

		return NotifyServerLogOutput(
			(LogLevel)ServerLogOutputMessage.level(),
			Utf8ToUtf16(ServerLogOutputMessage.command()).c_str()
		);
	
	case ServerSequence::SID_ServerCommandResultOutput:
		Success = ServerCommandResultOutputMessage.ParseFromArray((PBYTE)MessageBuffer + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		return NotifyServerCommandResultOutput(
			(CommandStatus)ServerCommandResultOutputMessage.status(),
			Utf8ToUtf16(ServerCommandResultOutputMessage.reply()).c_str()
		);
	
	case ServerSequence::SID_ServerUIReady:
		Success = ServerUIReadyMessage.ParseFromArray((PBYTE)MessageBuffer + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		return NotifyServerUIReady();
	
	case ServerSequence::SID_ServerMessageBox:
		Success = ServerMessageBoxMessage.ParseFromArray((PBYTE)MessageBuffer + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		return NotifyServerMessageBox(
			Utf8ToUtf16(ServerMessageBoxMessage.description()).c_str(),
			ServerMessageBoxMessage.flags(),
			ServerMessageBoxMessage.locked()
		);
	
	case ServerSequence::SID_ServerTaskStartAndDisableUI:
		Success = ServerTaskStartAndDisableUIMessage.ParseFromArray((PBYTE)MessageBuffer + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		return NotifyServerTaskStartAndDisableUI();
	
	case ServerSequence::SID_ServerTaskEndAndEnableUI:
		Success = ServerTaskEndAndEnableUIMessage.ParseFromArray((PBYTE)MessageBuffer + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		return NotifyServerTaskEndAndEnableUI(
			ServerTaskEndAndEnableUIMessage.taskcompletestatus(),
			Utf8ToUtf16(ServerTaskEndAndEnableUIMessage.description()).c_str()
		);
	
	case ServerSequence::SID_ServerUIHeartbeatPackage:
		Success = ServerUIHeartbeatPackageMessage.ParseFromArray((PBYTE)MessageBuffer + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		return NotifyServerUIHeartbeatPackage();

	case ServerSequence::SID_ServerExitFromRemoteProcess:
		Success = ServerExitFromRemoteProcessMessage.ParseFromArray((PBYTE)MessageBuffer + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		return NotifyServerExitFromRemoteProcess();
	}

	return FALSE;
}

