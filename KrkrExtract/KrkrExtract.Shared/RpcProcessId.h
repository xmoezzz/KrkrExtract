#pragma once

#include <stdint.h>

enum class RequestFromServer : uint32_t 
{
	//
	// plugins
	//

	mmNotifyPsbModeChanged  = 0,
	mmNotifyTextModeChanged = 1,
	mmNotifyPngModeChanged  = 2,
	mmNotifyTjs2ModeChanged = 3,
	mmNotifyTlgModeChanged  = 4,
	mmNotifyAlphaMovieModeChanged = 5,
	mmNotifyPbdModeChanged  = 6,

	//
	// tasks & other
	//

	mmNotifyUniversalDumperModeChecked = 50,
	mmNotifyUniversalPatchIconChecked  = 51,
	mmNotifyUniversalPatchProtectionChecked = 52,
	mmNotifyUniversalPatchMakeChecked  = 53,
	mmNotifyPackerChecked              = 54,
	mmNotifyPackerBaseDirChecked       = 55,
	mmNotifyPackerOriginalArchiveNameChecked = 56,
	mmNotifyPackerOutputArchiveNameChecked   = 57,
	mmNotifyCommandEmitted  = 58,
	mmNotifyTaskDumpStart   = 59,
	mmNotifyTaskCancelled   = 60,
	mmNotifyTaskCloseWindow = 61,
	mmNotifyOpenConsole     = 62,
	mmNotifyCloseConsole    = 63
};


enum class RequestFromClient : uint32_t 
{
	mmServerProgressBar = 1000,
	mmServerLogOutput   = 1001,
	mmServerUIReady     = 1002,
	mmServerTaskError   = 1003,
	mmServerTaskErrorWithDescription = 1004,
	mmServerUIHeartbeatPackage    = 1005,
	mmServerHandShakeFromRemote   = 1006,
	mmServerExitFromRemoteProcess = 1007
};




