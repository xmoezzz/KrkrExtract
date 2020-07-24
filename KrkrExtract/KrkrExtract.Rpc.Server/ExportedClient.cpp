#include "ServerImpl.h"
#include <ClientUniversalDumperModeChecked.pb.h>
#include <ClientUniversalPatchMakeChecked.pb.h>
#include <ClientPackerChecked.pb.h>
#include <ClientCommandEmitted.pb.h>
#include <ClientTaskDumpStart.pb.h>
#include <ClientCancelTask.pb.h>
#include <ClientTaskCloseWindow.pb.h>
#include <Utf.Convert.h>


BOOL ServerImpl::TellClientUniversalDumperModeChecked(
	_In_ KrkrPsbMode PsbMode,
	_In_ KrkrTextMode TextMode,
	_In_ KrkrPngMode  PngMode,
	_In_ KrkrTjs2Mode Tjs2Mode,
	_In_ KrkrTlgMode  TlgMode,
	_In_ KrkrAmvMode  AmvMode,
	_In_ KrkrPbdMode  PbdMode
)
{
	ClientUniversalDumperModeChecked Message;

	if (!m_ServerThread || !m_ServerThread->IsAlpcServerReady())
		return FALSE;

	Message.set_psbmode((UINT64)PsbMode);
	Message.set_textmode((UINT64)TextMode);
	Message.set_pngmode((UINT64)PngMode);
	Message.set_tjs2mode((UINT64)Tjs2Mode);
	Message.set_tlgmode((UINT64)TlgMode);
	Message.set_pdbmode((UINT64)PbdMode);

	return m_ServerThread->SendMessageToRemote(
		(ULONG)ClientSequence::SID_ClientUniversalDumperModeChecked, 
		Message.SerializeAsString()
	);
}

BOOL ServerImpl::TellClientUniversalPatchMakeChecked(
	_In_ BOOL Protect,
	_In_ BOOL Icon
)
{
	ClientUniversalPatchMakeChecked Message;
	
	Message.set_protect(Protect);
	Message.set_icon(Icon);

	return m_ServerThread->SendMessageToRemote(
		(ULONG)ClientSequence::SID_ClientUniversalPatchMakeChecked, 
		Message.SerializeAsString()
	);
}

BOOL ServerImpl::TellClientPackerChecked(
	_In_ PCWSTR BaseDir,
	_In_ PCWSTR OriginalArchiveName,
	_In_ PCWSTR OutputArchiveName
)
{
	ClientPackerChecked Message;

	Message.set_basedir(Utf16ToUtf8(BaseDir));
	Message.set_originalarchivename(Utf16ToUtf8(OriginalArchiveName));
	Message.set_outputarchivename(Utf16ToUtf8(OutputArchiveName));

	return m_ServerThread->SendMessageToRemote(
		(ULONG)ClientSequence::SID_ClientPackerChecked, 
		Message.SerializeAsString()
	);
}

BOOL ServerImpl::TellClientCommandEmitted(
	PCWSTR Command
)
{
	ClientCommandEmitted Message;
	
	Message.set_command(Utf16ToUtf8(Command));
	return m_ServerThread->SendMessageToRemote(
		(ULONG)ClientSequence::SID_ClientCommandEmitted, 
		Message.SerializeAsString()
	);
}

BOOL ServerImpl::TellClientTaskDumpStart(
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
	ClientTaskDumpStart Message;

	Message.set_psbmode((UINT64)PsbMode);
	Message.set_textmode((UINT64)TextMode);
	Message.set_pngmode((UINT64)PngMode);
	Message.set_tjs2mode((UINT64)Tjs2Mode);
	Message.set_tlgmode((UINT64)TlgMode);
	Message.set_pdbmode((UINT64)PbdMode);
	Message.set_file(Utf16ToUtf8(File));

	return m_ServerThread->SendMessageToRemote(
		(ULONG)ClientSequence::SID_ClientTaskDumpStart, 
		Message.SerializeAsString()
	);
}

BOOL ServerImpl::TellClientCancelTask()
{
	ClientCancelTask Message;

	return m_ServerThread->SendMessageToRemote(
		(ULONG)ClientSequence::SID_ClientCancelTask, 
		Message.SerializeAsString()
	);
}

BOOL ServerImpl::TellClientTaskCloseWindow()
{
	ClientTaskCloseWindow Message;

	return m_ServerThread->SendMessageToRemote(
		(ULONG)ClientSequence::SID_ClientTaskCloseWindow, 
		Message.SerializeAsString()
	);
}

