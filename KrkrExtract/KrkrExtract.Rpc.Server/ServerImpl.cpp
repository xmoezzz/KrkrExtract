#include "ServerImpl.h"
#include "Console.h"
#include <my.h>
#include <time.h>
#include <ctime>
#include <chrono>
#include <vector>
#include <ServerProgressBar.pb.h>
#include <ServerLogOutput.pb.h>
#include <ServerCommandResultOutput.pb.h>
#include <ServerUIReady.pb.h>
#include <ServerMessageBox.pb.h>
#include <ServerTaskStartAndDisableUI.pb.h>
#include <ServerTaskEndAndEnableUI.pb.h>
#include <ServerUIHeartbeatPackage.pb.h>
#include <ServerExitFromRemoteProcess.pb.h>

ServerImpl::ServerImpl(
	BOOL IsOfficialServer, 
	ULONG Secret, 
	ULONG HeartbeatTimeoutThreshold, 
	ULONG HandshakeTimeoutThreshold) :
	m_IsOfficialServer(IsOfficialServer),
	m_CurrentSecret(Secret),
	m_HeartbeatTimeoutThreshold(HeartbeatTimeoutThreshold),
	m_HandshakeTimeoutThreshold(HandshakeTimeoutThreshold)
{
	if (m_HandshakeTimeoutThreshold <= 0) {
		m_HandshakeTimeoutThreshold = 20000;
	}
	
	if (m_HandshakeTimeoutThreshold > 5000000) {
		m_HandshakeTimeoutThreshold = 20000;
	}

	if (m_HeartbeatTimeoutThreshold <= 1000) {
		m_HeartbeatTimeoutThreshold = 2000;
	}

	if (m_HeartbeatTimeoutThreshold > 5000000) {
		m_HeartbeatTimeoutThreshold = 2000;
	}
}

ServerImpl::~ServerImpl()
{
	ShutdownServer();
}

HANDLE ServerImpl::GetRemoteProcessHandle()
{
	if (m_ServerThread)
		return NULL;
	
	return m_ServerThread->GetRemoteProcess();
}

BOOL ServerImpl::RunServer(
	NotifyServerProgressChangedCallback       NotifyServerProgressChangedStub,
	NotifyServerLogOutputCallback             NotifyServerLogOutputStub,
	NotifyServerUIReadyCallback               NotifyServerUIReadyStub,
	NotifyServerMessageBoxCallback            NotifyServerMessageBoxStub,
	NotifyServerTaskStartAndDisableUICallback NotifyServerTaskStartAndDisableUIStub,
	NotifyServerTaskEndAndEnableUICallback    NotifyServerTaskEndAndEnableUIStub,
	NotifyServerExitFromRemoteProcessCallback NotifyServerExitFromRemoteProcessStub,
	NotifyServerRaiseErrorCallback            NotifyServerRaiseErrorStub
)
{
	if (m_ServerThread)
		return TRUE;

	m_NotifyServerProgressChangedStub       = NotifyServerProgressChangedStub;
	m_NotifyServerLogOutputStub             = NotifyServerLogOutputStub;
	m_NotifyServerUIReadyStub               = NotifyServerUIReadyStub;
	m_NotifyServerMessageBoxStub            = NotifyServerMessageBoxStub;
	m_NotifyServerTaskStartAndDisableUIStub = NotifyServerTaskStartAndDisableUIStub;
	m_NotifyServerExitFromRemoteProcessStub = NotifyServerExitFromRemoteProcessStub;
	m_NotifyServerTaskEndAndEnableUIStub    = NotifyServerTaskEndAndEnableUIStub;
	
	m_ServerThread = new RevThread(
		this,
		nullptr,
		m_HandshakeTimeoutThreshold,
		m_CurrentSecret,
		m_NotifyServerRaiseErrorStub
	);

	return m_ServerThread->Run();
}

BOOL ServerImpl::ShutdownServer()
{
	if (m_ServerThread)
	{
		m_ServerThread->SendKill();
		Ps::Sleep(50);
		m_ServerThread->Stop();

		delete m_ServerThread;
	}

	m_ServerThread = nullptr;
	return TRUE;
}

BOOL ServerImpl::NotifyServerProgressBar(PCWSTR TaskName, ULONGLONG Current, ULONGLONG Total)
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (!m_UIIsReady)
		return FALSE;

	if (m_NotifyServerProgressChangedStub) {
		m_NotifyServerProgressChangedStub(TaskName, Current, Total);
	}
	
	return TRUE;
}


BOOL ServerImpl::NotifyServerLogOutput(LogLevel Level, PCWSTR Command)
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (!m_UIIsReady)
		return FALSE;

	if (m_NotifyServerLogOutputStub) {
		m_NotifyServerLogOutputStub(Level, Command, FALSE);
	}
	else {
		NtPrintConsole(L"%s\n", Command);
	}

	return TRUE;
}

BOOL ServerImpl::NotifyServerCommandResultOutput(CommandStatus Status, PCWSTR Reply)
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (!m_UIIsReady)
		return FALSE;

	if (m_NotifyServerLogOutputStub) {
		switch (Status)
		{
		case CommandStatus::COMMAND_OK:
			m_NotifyServerLogOutputStub(LogLevel::LOG_OK, Reply, TRUE);
			break;

		case CommandStatus::COMMAND_WARN:
			m_NotifyServerLogOutputStub(LogLevel::LOG_WARN, Reply, TRUE);
			break;

		case CommandStatus::COMMAND_ERROR:
			m_NotifyServerLogOutputStub(LogLevel::LOG_ERROR, Reply, TRUE);
			break;
		}
	}
	else {
		NtPrintConsole(L"%s\n", Reply);
	}
	
	return TRUE;
}


BOOL ServerImpl::NotifyServerUIReady()
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (m_UIIsReady)
		return TRUE;

	m_UIIsReady = TRUE;

	if (m_NotifyServerUIReadyStub) {
		m_NotifyServerUIReadyStub();
	}

	return TRUE;
}

BOOL ServerImpl::NotifyServerMessageBox(PCWSTR Description, ULONG Flags, BOOL Locked)
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (!m_UIIsReady)
		return FALSE;
	
	if (m_NotifyServerMessageBoxStub) {
		m_NotifyServerMessageBoxStub(Description, Flags, Locked);
	}

	return TRUE;
}

BOOL ServerImpl::NotifyServerTaskStartAndDisableUI()
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (!m_UIIsReady)
		return FALSE;

	if (m_NotifyServerTaskStartAndDisableUIStub) {
		m_NotifyServerTaskStartAndDisableUIStub();
	}

	return TRUE;
}

BOOL ServerImpl::NotifyServerTaskEndAndEnableUI(BOOL TaskCompleteStatus, PCWSTR Description)
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (m_UIIsReady)
		return TRUE;

	if (m_NotifyServerTaskEndAndEnableUIStub) {
		m_NotifyServerTaskEndAndEnableUIStub(TaskCompleteStatus, Description);
	}

	return TRUE;
}

BOOL ServerImpl::NotifyServerUIHeartbeatPackage()
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (!m_UIIsReady)
		return FALSE;

	if (m_EnterExitRoutine)
		return TRUE;
	
	if (GetTickCount64() - m_LastHeartbeat > m_HeartbeatTimeoutThreshold) 
	{
		if (m_NotifyServerRaiseErrorStub)
		{
			if (!m_NotifyServerRaiseErrorStub(
				RaiseErrorType::RAISE_ERROR_HEARTBEAT_TIMEOUT,
				L"Heartbeat timeout"))
			{
				Ps::ExitProcess(0);
			}
		}
	}
	m_LastHeartbeat = GetTickCount64();
	
	return TRUE;
}


BOOL ServerImpl::NotifyServerExitFromRemoteProcess()
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (!m_UIIsReady)
		return FALSE;

	if (m_NotifyServerExitFromRemoteProcessStub) {
		m_EnterExitRoutine = TRUE;
		m_NotifyServerExitFromRemoteProcessStub();
		return TRUE;
	}

	Ps::ExitProcess(0);
	return TRUE;
}




