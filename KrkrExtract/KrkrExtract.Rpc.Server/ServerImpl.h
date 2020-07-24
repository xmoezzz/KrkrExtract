#pragma once

#include <Stubs.h>
#include <RpcSequence.h>
#include <my.h>
#include <atomic>
#include "RevThread.h"
#include "AlpcRev.h"
#include "PrivateStub.h"


class ServerImpl final : 
	public ServerStub, 
	public AlpcRev
{
public:

	ServerImpl() = delete;
	ServerImpl(const ServerImpl&) = delete;
	ServerImpl(BOOL IsOfficialServer, ULONG Secret, ULONG HeartbeatTimeoutThreshold = 2000, ULONG HandshakeTimeoutThreshold = 20000);
	~ServerImpl();

	BOOL RunServer(
		NotifyServerProgressChangedCallback       NotifyServerProgressChangedStub = nullptr,
		NotifyServerLogOutputCallback             NotifyServerLogOutputStub       = nullptr,
		NotifyServerUIReadyCallback               NotifyServerUIReadyStub         = nullptr,
		NotifyServerMessageBoxCallback            NotifyServerMessageBoxStub      = nullptr,
		NotifyServerTaskStartAndDisableUICallback NotifyServerTaskStartAndDisableUIStub = nullptr,
		NotifyServerTaskEndAndEnableUICallback    NotifyServerTaskEndAndEnableUIStub    = nullptr,
		NotifyServerRaiseErrorCallback            NotifyServerRaiseErrorStub            = nullptr
		);

	BOOL ShutdownServer();

private:

	BOOL ParseMessage(PVOID MessageBuffer, SIZE_T MessageSize);


	//
	// Server Stubs
	//
	BOOL NotifyServerProgressBar(PCWSTR TaskName, ULONGLONG Current, ULONGLONG Total);
	BOOL NotifyServerLogOutput(LogLevel Level, PCWSTR Command);
	BOOL NotifyServerCommandResultOutput(CommandStatus Status, PCWSTR Reply);
	BOOL NotifyServerUIReady();
	BOOL NotifyServerMessageBox(PCWSTR Description, ULONG Flags, BOOL Locked);
	BOOL NotifyServerTaskStartAndDisableUI();
	BOOL NotifyServerTaskEndAndEnableUI(BOOL TaskCompleteStatus, PCWSTR Description);

	//
	// Call this function every 1~2 second(s)
	// If last time - now >= Threshold
	// Server process with exit with STATUS_TIMEOUT
	//
	BOOL NotifyServerUIHeartbeatPackage();

	//
	// Tell me I should exit now
	//
	BOOL NotifyServerExitFromRemoteProcess();

public:
	//
	// Client, Exported functions
	//

	BOOL TellClientUniversalDumperModeChecked(
		_In_ KrkrPsbMode PsbMode,
		_In_ KrkrTextMode TextMode,
		_In_ KrkrPngMode  PngMode,
		_In_ KrkrTjs2Mode Tjs2Mode,
		_In_ KrkrTlgMode  TlgMode,
		_In_ KrkrAmvMode  AmvMode,
		_In_ KrkrPbdMode  PdbMode
	);

	BOOL TellClientUniversalPatchMakeChecked(
		_In_ BOOL Protect,
		_In_ BOOL Icon
	);

	BOOL TellClientPackerChecked(
		_In_ PCWSTR BaseDir,
		_In_ PCWSTR OriginalArchiveName,
		_In_ PCWSTR OutputArchiveName
	);

	BOOL TellClientCommandEmitted(
		PCWSTR Command
	);

	BOOL TellClientTaskDumpStart(
		_In_ KrkrPsbMode PsbMode,
		_In_ KrkrTextMode TextMode,
		_In_ KrkrPngMode  PngMode,
		_In_ KrkrTjs2Mode Tjs2Mode,
		_In_ KrkrTlgMode  TlgMode,
		_In_ KrkrAmvMode  AmvMode,
		_In_ KrkrPbdMode  PdbMode,
		_In_ PCWSTR File
	);

	BOOL TellClientCancelTask();
	BOOL TellClientTaskCloseWindow();

private:
	BOOL                                      m_GetHandShakePackage    = FALSE;
	BOOL                                      m_ServerPortInitialized  = FALSE;
	BOOL                                      m_IsOfficialServer       = FALSE;
	std::atomic<BOOL>                         m_UIIsReady              = FALSE;
	std::atomic<BOOL>                         m_EnterExitRoutine       = FALSE;
	ULONG                                     m_CurrentSecret          = TAG4('Xmoe');
	ULONG                                     m_RemoteProcessPid       = 0;
	RevThread*                                m_ServerThread           = nullptr;
	ULONG64                                   m_ServerInitializedTime  = 0;
	std::atomic<ULONG64>                      m_HeartbeatTimeoutThreshold = 2000;
	ULONG64                                   m_HandshakeTimeoutThreshold = 20000;
	ULONG64                                   m_LastHeartbeat = 0;
	NotifyServerProgressChangedCallback       m_NotifyServerProgressChangedStub       = nullptr;
	NotifyServerLogOutputCallback             m_NotifyServerLogOutputStub             = nullptr;
	NotifyServerTaskEndAndEnableUICallback    m_NotifyServerTaskEndAndEnableUIStub    = nullptr;
	NotifyServerUIReadyCallback               m_NotifyServerUIReadyStub               = nullptr;
	NotifyServerMessageBoxCallback            m_NotifyServerMessageBoxStub            = nullptr;
	NotifyServerTaskStartAndDisableUICallback m_NotifyServerTaskStartAndDisableUIStub = nullptr;
	NotifyServerExitFromRemoteProcessCallback m_NotifyServerExitFromRemoteProcessStub = nullptr;
	NotifyServerRaiseErrorCallback            m_NotifyServerRaiseErrorStub            = nullptr;
};

