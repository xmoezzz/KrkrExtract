#include "ServerImpl.h"
#include "Console.h"
#include "AlpcRev.h"
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
	if (!m_ServerThread)
		return NULL;
	
	return m_RemoteProcess;
}

ULONG ServerImpl::GetRemoteProcessId()
{
	if (!m_ServerThread)
		return NULL;

	return m_RemoteProcessPid;
}

BOOL ServerImpl::RunServer()
{
	NTSTATUS Status;

	if (m_ServerThread)
		return TRUE;

	m_ServerThread = new RevThread(
		this,
		m_HandshakeTimeoutThreshold,
		m_CurrentSecret
	);

	return m_ServerThread->Run();
}

BOOL ServerImpl::ShutdownServer()
{
	EventMsg* Item;

	if (m_ServerThread)
	{
		m_ServerThread->SendKill();
		Ps::Sleep(50);
		m_ServerThread->Stop();

		delete m_ServerThread;
	}

	if (m_AlpcServerPort) {
		NtClose(m_AlpcServerPort);
	}

	m_ServerThread = nullptr;

	while (m_EventQueue.try_dequeue(Item)) {
		delete Item;
	}

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

	return m_EventQueue.enqueue(
		new (std::nothrow) NotifyServerProgressChangedMsg(
			Current,
			Total,
			TaskName
		)
	);
}


BOOL ServerImpl::NotifyServerLogOutput(LogLevel Level, PCWSTR Command)
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (!m_UIIsReady)
		return FALSE;

	return m_EventQueue.enqueue(
		new NotifyServerLogOutputMsg(
			Level,
			Command,
			FALSE
		)
	);

	return TRUE;
}

BOOL ServerImpl::NotifyServerCommandResultOutput(CommandStatus Status, PCWSTR Reply)
{
	BOOL     IsCmd;
	LogLevel Level;

	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (!m_UIIsReady)
		return FALSE;

	switch (Status)
	{
	case CommandStatus::COMMAND_OK:
		Level = LogLevel::LOG_OK;
		break;

	case CommandStatus::COMMAND_WARN:
		Level = LogLevel::LOG_WARN;
		break;

	case CommandStatus::COMMAND_ERROR:
		Level = LogLevel::LOG_ERROR;
		break;

	default:
		Level = LogLevel::LOG_OK;
		break;
	}

	return m_EventQueue.enqueue(
		new NotifyServerLogOutputMsg(
			Level,
			Reply,
			TRUE
		)
	);
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

	return m_EventQueue.enqueue(
		new NotifyServerUIReadyMsg(
		)
	);
}

BOOL ServerImpl::NotifyServerMessageBox(PCWSTR Description, ULONG Flags, BOOL Locked)
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (!m_UIIsReady)
		return FALSE;
	
	return m_EventQueue.enqueue(
		new NotifyServerMessageBoxMsg(
			Description,
			Flags,
			Locked
		)
	);
}

BOOL ServerImpl::NotifyServerTaskStartAndDisableUI()
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (!m_UIIsReady)
		return FALSE;

	return m_EventQueue.enqueue(
		new NotifyServerTaskStartAndDisableUIMsg(
		)
	);
}

BOOL ServerImpl::NotifyServerTaskEndAndEnableUI(BOOL TaskCompleteStatus, PCWSTR Description)
{
	if (!m_ServerPortInitialized)
		return FALSE;

	if (!m_GetHandShakePackage)
		return FALSE;

	if (m_UIIsReady)
		return TRUE;

	return m_EventQueue.enqueue(
		new NotifyServerTaskEndAndEnableUIMsg(
		)
	);
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
		m_EventQueue.enqueue(
			new NotifyServerRaiseErrorMsg(
				RaiseErrorType::RAISE_ERROR_HEARTBEAT_TIMEOUT,
				L"Heartbeat timeout"
			)
		);
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

	return m_EventQueue.enqueue(
		new NotifyServerExitFromRemoteProcessMsg(
		)
	);
}


EventMsg* ServerImpl::PopEventMessage()
{
	EventMsg* Msg = nullptr;
	if (m_EventQueue.try_dequeue(Msg)) {
		return Msg;
	}
	
	return nullptr;
}

