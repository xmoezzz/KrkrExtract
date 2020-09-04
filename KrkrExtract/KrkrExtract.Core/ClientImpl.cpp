#include "KrkrExtract.h"
#include "CoreTask.h"

#include <RpcSequence.h>
#include <Utf.Convert.h>
#include <ClientUniversalDumperModeChecked.pb.h>
#include <ClientCancelTask.pb.h>
#include <ClientCommandEmitted.pb.h>
#include <ClientTaskDumpStart.pb.h>
#include <ClientTaskCloseWindow.pb.h>
#include <ClientUniversalPatchMakeChecked.pb.h>
#include <ClientPackerChecked.pb.h>


BOOL KrkrExtractCore::AlpcProcessClientPackage(PVOID Message, SIZE_T MessageSize)
{
	BOOL                             Success;
	ClientSequence                   MessageType;
	ClientCommandEmitted             ClientCommandEmittedMessage;
	ClientTaskDumpStart              ClientTaskDumpStartMessage;
	ClientCancelTask                 ClientCancelTaskMessage;
	ClientTaskCloseWindow            ClientTaskCloseWindowMessage;
	ClientUniversalDumperModeChecked ClientUniversalDumperModeCheckedMessage;
	ClientUniversalPatchMakeChecked  ClientUniversalPatchMakeCheckedMessage;
	ClientPackerChecked              ClientPackerCheckedMessage;

	//
	// sizeof(UINT32) MessageType
	//
	if (!Message || MessageSize <= sizeof(UINT32))
		return FALSE;
	
	MessageType = (ClientSequence)*(PUINT32)Message;
	switch (MessageType)
	{
	case ClientSequence::SID_ClientUniversalDumperModeChecked:
		Success = ClientUniversalDumperModeCheckedMessage.ParseFromArray((PBYTE)Message + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		if (ClientUniversalDumperModeCheckedMessage.sequenceid() != (UINT64)ClientSequence::SID_ClientUniversalDumperModeChecked)
			return FALSE;
		
		return NotifyClientUniversalDumperModeChecked(
			(KrkrPsbMode)ClientUniversalDumperModeCheckedMessage.psbmode(),
			(KrkrTextMode)ClientUniversalDumperModeCheckedMessage.textmode(),
			(KrkrPngMode)ClientUniversalDumperModeCheckedMessage.pngmode(),
			(KrkrTjs2Mode)ClientUniversalDumperModeCheckedMessage.tjs2mode(),
			(KrkrTlgMode)ClientUniversalDumperModeCheckedMessage.tlgmode(),
			(KrkrAmvMode)ClientUniversalDumperModeCheckedMessage.amvmode(),
			(KrkrPbdMode)ClientUniversalDumperModeCheckedMessage.pdbmode()
		);
	
	case ClientSequence::SID_ClientUniversalPatchMakeChecked:
		Success = ClientUniversalPatchMakeCheckedMessage.ParseFromArray((PBYTE)Message + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		if (ClientUniversalPatchMakeCheckedMessage.sequenceid() != (UINT64)ClientSequence::SID_ClientUniversalPatchMakeChecked)
			return FALSE;

		return NotifyClientUniversalPatchMakeChecked(
			ClientUniversalPatchMakeCheckedMessage.protect(),
			ClientUniversalPatchMakeCheckedMessage.icon()
		);

	case ClientSequence::SID_ClientPackerChecked:
		Success = ClientPackerCheckedMessage.ParseFromArray((PBYTE)Message + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		if (ClientPackerCheckedMessage.sequenceid() != (UINT64)ClientSequence::SID_ClientPackerChecked)
			return FALSE;

		return NotifyClientPackerChecked(
			Utf8ToUtf16(ClientPackerCheckedMessage.basedir()).c_str(),
			Utf8ToUtf16(ClientPackerCheckedMessage.originalarchivename()).c_str(),
			Utf8ToUtf16(ClientPackerCheckedMessage.outputarchivename()).c_str()
		);
	
	case ClientSequence::SID_ClientCommandEmitted:
		Success = ClientCommandEmittedMessage.ParseFromArray((PBYTE)Message + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		if (ClientCommandEmittedMessage.sequenceid() != (UINT64)ClientSequence::SID_ClientCommandEmitted)
			return FALSE;
		
		return NotifyClientCommandEmitted(Utf8ToUtf16(ClientCommandEmittedMessage.command()).c_str());

	case ClientSequence::SID_ClientTaskDumpStart:
		Success = ClientTaskDumpStartMessage.ParseFromArray((PBYTE)Message + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		if (ClientTaskDumpStartMessage.sequenceid() != (UINT64)ClientSequence::SID_ClientTaskDumpStart)
			return FALSE;
		
		return NotifyClientTaskDumpStart(
			(KrkrPsbMode)ClientTaskDumpStartMessage.psbmode(),
			(KrkrTextMode)ClientTaskDumpStartMessage.textmode(),
			(KrkrPngMode)ClientTaskDumpStartMessage.pngmode(),
			(KrkrTjs2Mode)ClientTaskDumpStartMessage.tjs2mode(),
			(KrkrTlgMode)ClientTaskDumpStartMessage.tlgmode(),
			(KrkrAmvMode)ClientTaskDumpStartMessage.amvmode(),
			(KrkrPbdMode)ClientTaskDumpStartMessage.pdbmode(),
			Utf8ToUtf16(ClientTaskDumpStartMessage.file()).c_str()
		);
	
	case ClientSequence::SID_ClientCancelTask:
		Success = ClientCancelTaskMessage.ParseFromArray((PBYTE)Message + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		if (ClientCancelTaskMessage.sequenceid() != (UINT64)ClientSequence::SID_ClientCancelTask)
			return FALSE;

		return NotifyClientCancelTask();
	
	case ClientSequence::SID_ClientTaskCloseWindow:
		Success = ClientTaskCloseWindowMessage.ParseFromArray((PBYTE)Message + sizeof(UINT32), MessageSize - sizeof(UINT32));
		if (!Success)
			return Success;

		if (ClientTaskCloseWindowMessage.sequenceid() != (UINT64)ClientSequence::SID_ClientTaskCloseWindow)
			return FALSE;

		return NotifyClientTaskCloseWindow();
	}
	
	PrintConsoleW(L"Unknown message type : %08x\n", (ULONG_PTR)MessageType);
	return FALSE;
}

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
	Ps::ExitProcess(0);
	return TRUE;
}

