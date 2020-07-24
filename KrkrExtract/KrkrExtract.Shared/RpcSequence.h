#pragma once

#include <stdint.h>

enum class ServerSequence
{
	SID_ServerProgressBar           = 100,
	SID_ServerLogOutput             = 101,
	SID_ServerCommandResultOutput   = 102,
	SID_ServerUIReady               = 103,
	SID_ServerMessageBox            = 104,
	SID_ServerTaskStartAndDisableUI = 105,
	SID_ServerTaskEndAndEnableUI    = 106,
	SID_ServerUIHeartbeatPackage    = 107,
	SID_ServerExitFromRemoteProcess = 108
};


enum class ClientSequence
{
	SID_ClientUniversalDumperModeChecked = 200,
	SID_ClientUniversalPatchMakeChecked  = 201,
	SID_ClientPackerChecked              = 202,
	SID_ClientCommandEmitted             = 203,
	SID_ClientTaskDumpStart              = 204,
	SID_ClientCancelTask                 = 205,
	SID_ClientTaskCloseWindow            = 206
};

