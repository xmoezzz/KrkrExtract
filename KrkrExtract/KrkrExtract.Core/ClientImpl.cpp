#include "KrkrExtract.h"
#include "CoreTask.h"

#include <RpcSequence.h>
#include <Utf.Convert.h>


BOOL KrkrExtractCore::NotifyClientUniversalDumperModeChecked(
	_In_ KrkrPsbMode  PsbMode,
	_In_ KrkrTextMode TextMode,
	_In_ KrkrPngMode  PngMode,
	_In_ KrkrTjs2Mode Tjs2Mode,
	_In_ KrkrTlgMode  TlgMode,
	_In_ KrkrAmvMode  AmvMode,
	_In_ KrkrPbdMode  PbdMode
)
{
	TellServerTaskStartAndDisableUI();

	//
	// Lasy deleting
	//

	if (m_CurrentTask && m_CurrentTask.load()->IsTerminated() == FALSE) {
		TellServerMessageBox(L"Task aborted : Waiting for another to complete", MB_OK | MB_ICONERROR, TRUE);
		TellServerTaskEndAndEnableUI(TRUE, nullptr);
		return FALSE;
	}

	m_PsbFlag  = PsbMode;
	m_TextFlag = TextMode;
	m_PngFlag  = PngMode;
	m_TjsFlag  = Tjs2Mode;
	m_AmvFlag  = AmvMode;
	m_PbdFlag  = PbdMode;

	if (m_CurrentTask) {
		delete m_CurrentTask;
	}

	m_CurrentTask = new (std::nothrow) CoUniversalDumperTask(this);
	if (!m_CurrentTask)
	{
		TellServerMessageBox(L"Task aborted : Insufficient memory for the new task", MB_OK | MB_ICONERROR, TRUE);
		TellServerTaskEndAndEnableUI(TRUE, nullptr);
		return FALSE;
	}
	
	return m_CurrentTask.load()->Run();
}

BOOL KrkrExtractCore::NotifyClientUniversalPatchMakeChecked(
	_In_ BOOL Protect,
	_In_ BOOL Icon
)
{
	TellServerTaskStartAndDisableUI();

	//
	// Lasy deleting
	//

	if (m_CurrentTask && m_CurrentTask.load()->IsTerminated() == FALSE) {
		TellServerMessageBox(L"Task aborted : Waiting for another to complete", MB_OK | MB_ICONERROR, TRUE);
		TellServerTaskEndAndEnableUI(TRUE, nullptr);
		return FALSE;
	}

	if (m_CurrentTask) {
		delete m_CurrentTask;
	}

	m_CurrentTask = new (std::nothrow) CoUniversalPatchTask(this, Icon, Protect);
	if (!m_CurrentTask)
	{
		TellServerMessageBox(L"Task aborted : Insufficient memory for the new task", MB_OK | MB_ICONERROR, TRUE);
		TellServerTaskEndAndEnableUI(TRUE, nullptr);
		return FALSE;
	}
	
	return m_CurrentTask.load()->Run();
}

BOOL KrkrExtractCore::NotifyClientPackerChecked(
	_In_ PCWSTR BaseDir,
	_In_ PCWSTR OriginalArchiveName,
	_In_ PCWSTR OutputArchiveName
)
{
	TellServerTaskStartAndDisableUI();

	//
	// Lasy deleting
	//

	if (m_CurrentTask && m_CurrentTask.load()->IsTerminated() == FALSE) {
		TellServerMessageBox(L"Task aborted : Waiting for another to complete", MB_OK | MB_ICONERROR, TRUE);
		TellServerTaskEndAndEnableUI(TRUE, nullptr);
		return FALSE;
	}

	if (m_CurrentTask) {
		delete m_CurrentTask;
	}
	
	m_CurrentTask = new (std::nothrow) CoPackerTask(this, BaseDir, OriginalArchiveName, OutputArchiveName);
	if (!m_CurrentTask)
	{
		TellServerMessageBox(L"Task aborted : Insufficient memory for the new task", MB_OK | MB_ICONERROR, TRUE);
		TellServerTaskEndAndEnableUI(TRUE, nullptr);
		return FALSE;
	}

	return m_CurrentTask.load()->Run();
}


BOOL KrkrExtractCore::NotifyClientCommandEmitted(
	_In_ PCWSTR Command
)
{
	if (!m_CommandExecutor) {
		m_CommandExecutor = new (std::nothrow) AsyncCommandExecutor(this, 16);
	}

	if (!m_CommandExecutor || m_CommandExecutor->IsInitialized() == FALSE) {
		TellServerLogOutput(LogLevel::LOG_ERROR, L"Command executor is not initialized");
		return FALSE;
	}

	return m_CommandExecutor->Execute(Command);
}

BOOL KrkrExtractCore::NotifyClientTaskDumpStart(
	_In_ KrkrPsbMode PsbMode,
	_In_ KrkrTextMode TextMode,
	_In_ KrkrPngMode  PngMode,
	_In_ KrkrTjs2Mode Tjs2Mode,
	_In_ KrkrTlgMode  TlgMode,
	_In_ KrkrAmvMode  AmvMode,
	_In_ KrkrPbdMode  PbdMode,
	_In_ PCWSTR File
)
{
	TellServerTaskStartAndDisableUI();

	//
	// Lasy deleting
	//

	if (m_CurrentTask && m_CurrentTask.load()->IsTerminated() == FALSE) {
		TellServerMessageBox(L"Task aborted : Waiting for another to complete", MB_OK | MB_ICONERROR, TRUE);
		TellServerTaskEndAndEnableUI(TRUE, nullptr);
		return FALSE;
	}

	m_PsbFlag  = PsbMode;
	m_TextFlag = TextMode;
	m_PngFlag  = PngMode;
	m_TjsFlag  = Tjs2Mode;
	m_AmvFlag  = AmvMode;
	m_PbdFlag  = PbdMode;

	if (m_CurrentTask) {
		delete m_CurrentTask;
	}

	m_CurrentTask = new (std::nothrow) CoDumperTask(this, File);
	if (!m_CurrentTask)
	{
		TellServerMessageBox(L"Task aborted : Insufficient memory for the new task", MB_OK | MB_ICONERROR, TRUE);
		TellServerTaskEndAndEnableUI(TRUE, nullptr);
		return FALSE;
	}

	return m_CurrentTask.load()->Run();

}

BOOL KrkrExtractCore::NotifyClientCancelTask(
)
{
	if (!m_CurrentTask && m_CurrentTask.load()->IsTerminated() == FALSE)
		return FALSE;

	m_CurrentTask.load()->SendKill();
	return TRUE;
}

BOOL KrkrExtractCore::NotifyClientTaskCloseWindow(
)
{
	switch (m_RunMode)
	{
	case KrkrRunMode::REMOTE_MODE:
	case KrkrRunMode::MIXED_MODE:
		if (m_ConnectionApi) {
			m_ConnectionApi->NotifyServerExitFromRemoteProcess();
		}
		break;
	}

	Ps::ExitProcess(0);
	return TRUE;
}

