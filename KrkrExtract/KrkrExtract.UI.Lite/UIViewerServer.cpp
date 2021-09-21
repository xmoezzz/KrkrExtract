#include "UIViewer.h"


BOOL UIViewerServer::NotifyServerProgressBar(PCWSTR TaskName, ULONGLONG Current, ULONGLONG Total)
{
	return m_Viewer->NotifyUIProgressBar(TaskName, Current, Total);
}

BOOL UIViewerServer::NotifyServerLogOutput(LogLevel Level, PCWSTR Command)
{
	return m_Viewer->NotifyUILogOutput(Level, Command, FALSE);
}

BOOL UIViewerServer::NotifyServerUIReady(ULONG ClientPort, PCSTR SessionKey, ULONG Extra)
{
	return m_Viewer->NotifyUIReady();
}

BOOL UIViewerServer::NotifyServerMessageBox(PCWSTR Description, ULONG Flags, BOOL Locked)
{
	return m_Viewer->NotifyUIMessageBox(Description, Flags, Locked);
}

BOOL UIViewerServer::NotifyServerUIHeartbeatPackage()
{
	return TRUE;
}

BOOL UIViewerServer::NotifyServerExitFromRemoteProcess()
{
	Ps::ExitProcess(0);
	return TRUE;
}

BOOL UIViewerServer::NotifyServerCommandResultOutput(CommandStatus Status, PCWSTR Reply)
{
	switch (Status)
	{
	case CommandStatus::COMMAND_OK:
		return m_Viewer->NotifyUILogOutput(LogLevel::LOG_OK, Reply, TRUE);

	case CommandStatus::COMMAND_WARN:
		return m_Viewer->NotifyUILogOutput(LogLevel::LOG_WARN, Reply, TRUE);

	case CommandStatus::COMMAND_ERROR:
		return m_Viewer->NotifyUILogOutput(LogLevel::LOG_ERROR, Reply, TRUE);
	}

	return m_Viewer->NotifyUILogOutput(LogLevel::LOG_DEBUG, Reply, TRUE);
}


BOOL UIViewerServer::NotifyServerTaskStartAndDisableUI()
{
	m_Viewer->EnterTask();
	return TRUE;
}

BOOL UIViewerServer::NotifyServerTaskEndAndEnableUI(BOOL TaskCompleteStatus, PCWSTR Description)
{
	switch (TaskCompleteStatus)
	{
	case FALSE:
		m_Viewer->NotifyUIMessageBox(Description ? Description : L"Task failed : Unknown reason", MB_OK | MB_ICONERROR, TRUE);
		m_Viewer->LeaveTask();
		break;

	default:
		//m_Viewer->NotifyUIMessageBox(Description ? Description : L"Task completed", MB_OK, TRUE);
		m_Viewer->LeaveTask();
		break;
	}

	return TRUE;
}

BOOL UIViewerServer::NotifyServerHandShakeFromRemote(ULONG Secret)
{
	return TRUE;
}

