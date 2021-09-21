#pragma once

#include <Windows.h>
#include "RpcUIDefine.h"
#include "RpcDefine.h"
#include <vector>

class ServerStub
{
public:

	virtual BOOL NotifyServerProgressBar(
		_In_ PCWSTR    TaskName,
		_In_ ULONGLONG Current, 
		_In_ ULONGLONG Total
	) = 0;

	virtual BOOL NotifyServerLogOutput(
		_In_ LogLevel Level, 
		_In_ PCWSTR Command
	) = 0;

	virtual BOOL NotifyServerCommandResultOutput(
		_In_  CommandStatus Status,
		_In_  PCWSTR Reply
	) = 0;

	virtual BOOL NotifyServerUIReady(
		ULONG ClientPort,
		PCSTR SessionKey,
		ULONG Extra
	) = 0;

	virtual BOOL NotifyServerMessageBox(
		_In_ PCWSTR Description, 
		_In_ ULONG Flags, 
		_In_ BOOL Locked
	) = 0;

	virtual BOOL NotifyServerTaskStartAndDisableUI(
	) = 0;

	virtual BOOL NotifyServerTaskEndAndEnableUI(
		_In_ BOOL TaskCompleteStatus, 
		_In_ PCWSTR Description
	) = 0;

	virtual BOOL NotifyServerUIHeartbeatPackage(
	) = 0;

	virtual BOOL NotifyServerExitFromRemoteProcess(
	) = 0;
};


class ClientStub
{
public:

	virtual BOOL NotifyClientUniversalDumperModeChecked(
		_In_ KrkrPsbMode PsbMode, 
		_In_ KrkrTextMode TextMode,
		_In_ KrkrPngMode  PngMode,
		_In_ KrkrTjs2Mode Tjs2Mode,
		_In_ KrkrTlgMode  TlgMode,
		_In_ KrkrAmvMode  AmvMode,
		_In_ KrkrPbdMode  PdbMode
	) = 0;

	virtual BOOL NotifyClientUniversalPatchMakeChecked(
		_In_ BOOL Protect, 
		_In_ BOOL Icon
	) = 0;

	virtual BOOL NotifyClientPackerChecked(
		_In_ PCWSTR BaseDir,
		_In_ PCWSTR OriginalArchiveName,
		_In_ PCWSTR OutputArchiveName
	) = 0;

	virtual BOOL NotifyClientCommandEmitted(
		_In_ PCWSTR Command
	) = 0;

	virtual BOOL NotifyClientTaskDumpStart(
		_In_ KrkrPsbMode PsbMode,
		_In_ KrkrTextMode TextMode,
		_In_ KrkrPngMode  PngMode,
		_In_ KrkrTjs2Mode Tjs2Mode,
		_In_ KrkrTlgMode  TlgMode,
		_In_ KrkrAmvMode  AmvMode,
		_In_ KrkrPbdMode  PdbMode,
		_In_ PCWSTR File
	) = 0;

	virtual BOOL NotifyClientCancelTask(
	) = 0;

	virtual BOOL NotifyClientTaskCloseWindow(
	) = 0;
};

