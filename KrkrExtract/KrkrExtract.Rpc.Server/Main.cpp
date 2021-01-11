#include <my.h>
#include <atomic>
#include <memory>
#include <string>
#include <shlwapi.h>
#include <Psapi.h>
#include <vector>
#include <Wincrypt.h>
#include "ServerImpl.h"
#include "SectionProtector.h"
#include "picojson.h"
#include "UniversalShellCode.h"
#include <Utf.Convert.h>

template <class T> inline std::shared_ptr<T> AllocateMemorySafe(SIZE_T Size)
{
	return std::shared_ptr<T>(
		(T*)AllocateMemory(Size),
		[](T* Ptr)
		{
			if (Ptr) {
				FreeMemory(Ptr);
			}
		});
}



class KrkrServer
{
public:
	void NotifyDllLoad()
	{
		RtlInitializeCriticalSection(&m_CriticalSection);
	}

	void NotifyDllUnLoad()
	{
		RtlDeleteCriticalSection(&m_CriticalSection);
		if (m_Server) 
		{
			m_Server->ShutdownServer();
			delete m_Server;
		}

		m_Server = nullptr;
	}

	BOOL XeCreateInstance(
		_In_ BOOL  IsOfficialServer,
		_In_ ULONG Secret,
		_In_ ULONG HeartbeatTimeoutThreshold,
		_In_ ULONG HandshakeTimeoutThreshold
	)
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		m_Server = new (std::nothrow) ServerImpl(
			IsOfficialServer,
			Secret,
			HeartbeatTimeoutThreshold,
			HandshakeTimeoutThreshold
		);

		if (!m_Server)
			return FALSE;

		return TRUE;
	}

	BOOL XeRunServer()
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return FALSE;

		return m_Server->RunServer();
	}

	BOOL XeClientUniversalDumperModeChecked(
		_In_ KrkrPsbMode PsbMode,
		_In_ KrkrTextMode TextMode,
		_In_ KrkrPngMode  PngMode,
		_In_ KrkrTjs2Mode Tjs2Mode,
		_In_ KrkrTlgMode  TlgMode,
		_In_ KrkrAmvMode  AmvMode,
		_In_ KrkrPbdMode  PdbMode
	)
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return FALSE;

		return m_Server->TellClientUniversalDumperModeChecked(
			PsbMode,
			TextMode,
			PngMode,
			Tjs2Mode,
			TlgMode,
			AmvMode,
			PdbMode
		);
	}

	BOOL XeClientUniversalPatchMakeChecked(
		_In_ BOOL Protect,
		_In_ BOOL Icon
	)
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return FALSE;

		return m_Server->TellClientUniversalPatchMakeChecked(
			Protect,
			Icon
		);
	}

	BOOL XeClientPackerChecked(
		_In_ PCWSTR BaseDir,
		_In_ PCWSTR OriginalArchiveName,
		_In_ PCWSTR OutputArchiveName
	)
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return FALSE;

		return m_Server->TellClientPackerChecked(
			BaseDir,
			OriginalArchiveName,
			OutputArchiveName
		);
	}

	BOOL XeClientCommandEmitted(
		_In_ PCWSTR Command
	)
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return FALSE;

		return m_Server->TellClientCommandEmitted(
			Command
		);
	}

	BOOL XeClientTaskDumpStart(
		_In_ KrkrPsbMode PsbMode,
		_In_ KrkrTextMode TextMode,
		_In_ KrkrPngMode  PngMode,
		_In_ KrkrTjs2Mode Tjs2Mode,
		_In_ KrkrTlgMode  TlgMode,
		_In_ KrkrAmvMode  AmvMode,
		_In_ KrkrPbdMode  PdbMode,
		_In_ PCWSTR File
	)
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return FALSE;

		return m_Server->TellClientTaskDumpStart(
			PsbMode,
			TextMode,
			PngMode,
			Tjs2Mode,
			TlgMode,
			AmvMode,
			PdbMode,
			File
		);
	}

	BOOL XeClientCancelTask()
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return FALSE;

		return m_Server->TellClientCancelTask();
	}

	BOOL XeClientTaskCloseWindow()
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return FALSE;

		return m_Server->TellClientTaskCloseWindow();
	}

	HANDLE XeGetRemoteProcessHandle()
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return nullptr;

		return m_Server->GetRemoteProcessHandle();
	}

	ULONG XeGetRemoteProcessId()
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return 0;

		return m_Server->GetRemoteProcessId();
	}

	BOOL XeShutdownServer()
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return FALSE;

		return m_Server->ShutdownServer();
	}

	BOOL XeGetEventMsg(EventMsg** Item)
	{
		//SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server || !Item)
			return FALSE;

		*Item = m_Server->PopEventMessage();
		return *Item ? TRUE : FALSE;
	}


private:
	RTL_CRITICAL_SECTION m_CriticalSection;
	ServerImpl*          m_Server = nullptr;
};

static KrkrServer* g_Server;

//
// Supported arch :
// x86_32 (IA32)
// x86_64 (AMD64)
// arm64  (aarch64)
//

CRITICAL_SECTION EventSection;

BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
		LdrDisableThreadCalloutsForDll(hModule);
		RtlInitializeCriticalSection(&EventSection);
		g_Server = new (std::nothrow) KrkrServer();
		g_Server->NotifyDllLoad();
		break;

    case DLL_PROCESS_DETACH:
		if (g_Server)
		{
			g_Server->NotifyDllUnLoad();
			delete g_Server;
		}
		g_Server = nullptr;
		RtlDeleteCriticalSection(&EventSection);
        break;
    }
    return TRUE;
}


#if ML_X86
#pragma comment(linker, "/EXPORT:XeCreateInstance=_XeCreateInstance@16")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeCreateInstance=XeCreateInstance")
#endif
EXTC_EXPORT
BOOL
NTAPI 
XeCreateInstance(
	_In_ BOOL  IsOfficialServer, 
	_In_ ULONG Secret,
	_In_ ULONG HeartbeatTimeoutThreshold,
	_In_ ULONG HandshakeTimeoutThreshold
)
{
	if (g_Server) 
	{
		return g_Server->XeCreateInstance(
			IsOfficialServer,
			Secret,
			HeartbeatTimeoutThreshold,
			HandshakeTimeoutThreshold
		);
	}

	return FALSE;
}


#if ML_X86
#pragma comment(linker, "/EXPORT:XeRunServer=_XeRunServer@0")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeRunServer=XeRunServer")
#endif
EXTC_EXPORT
BOOL 
NTAPI 
XeRunServer()
{
	return g_Server && g_Server->XeRunServer();
}


#if ML_X86
#pragma comment(linker, "/EXPORT:XeClientUniversalDumperModeChecked=_XeClientUniversalDumperModeChecked@28")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeClientUniversalDumperModeChecked=XeClientUniversalDumperModeChecked")
#endif
EXTC_EXPORT
BOOL 
NTAPI 
XeClientUniversalDumperModeChecked(
	_In_ KrkrPsbMode PsbMode,
	_In_ KrkrTextMode TextMode,
	_In_ KrkrPngMode  PngMode,
	_In_ KrkrTjs2Mode Tjs2Mode,
	_In_ KrkrTlgMode  TlgMode,
	_In_ KrkrAmvMode  AmvMode,
	_In_ KrkrPbdMode  PdbMode
)
{
	return g_Server && g_Server->XeClientUniversalDumperModeChecked(
		PsbMode,
		TextMode,
		PngMode,
		Tjs2Mode,
		TlgMode,
		AmvMode,
		PdbMode
	);
}


#if ML_X86
#pragma comment(linker, "/EXPORT:XeClientUniversalPatchMakeChecked=_XeClientUniversalPatchMakeChecked@8")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeClientUniversalPatchMakeChecked=XeClientUniversalPatchMakeChecked")
#endif
EXTC_EXPORT
BOOL 
NTAPI 
XeClientUniversalPatchMakeChecked(
	_In_ BOOL Protect,
	_In_ BOOL Icon
)
{
	return g_Server && g_Server->XeClientUniversalPatchMakeChecked(
		Protect,
		Icon
	);
}


#if ML_X86
#pragma comment(linker, "/EXPORT:XeClientPackerChecked=_XeClientPackerChecked@12")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeClientPackerChecked=XeClientPackerChecked")
#endif
EXTC_EXPORT
BOOL 
NTAPI 
XeClientPackerChecked(
	_In_ PCWSTR BaseDir,
	_In_ PCWSTR OriginalArchiveName,
	_In_ PCWSTR OutputArchiveName
)
{
	return g_Server && g_Server->XeClientPackerChecked(
		BaseDir,
		OriginalArchiveName,
		OutputArchiveName
	);

}


#if ML_X86
#pragma comment(linker, "/EXPORT:XeClientCommandEmitted=_XeClientCommandEmitted@4")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeClientCommandEmitted=XeClientCommandEmitted")
#endif
EXTC_EXPORT
BOOL
NTAPI 
XeClientCommandEmitted(
	PCWSTR Command
)
{
	return g_Server && g_Server->XeClientCommandEmitted(
		Command
	);
}



#if ML_X86
#pragma comment(linker, "/EXPORT:XeClientTaskDumpStart=_XeClientTaskDumpStart@32")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeClientTaskDumpStart=XeClientTaskDumpStart")
#endif
EXTC_EXPORT
BOOL
NTAPI 
XeClientTaskDumpStart(
	_In_ KrkrPsbMode PsbMode,
	_In_ KrkrTextMode TextMode,
	_In_ KrkrPngMode  PngMode,
	_In_ KrkrTjs2Mode Tjs2Mode,
	_In_ KrkrTlgMode  TlgMode,
	_In_ KrkrAmvMode  AmvMode,
	_In_ KrkrPbdMode  PdbMode,
	_In_ PCWSTR File
)
{
	return g_Server && g_Server->XeClientTaskDumpStart(
		PsbMode,
		TextMode,
		PngMode,
		Tjs2Mode,
		TlgMode,
		AmvMode,
		PdbMode,
		File
	);
}


#if ML_X86
#pragma comment(linker, "/EXPORT:XeClientCancelTask=_XeClientCancelTask@0")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeClientCancelTask=XeClientCancelTask")
#endif
EXTC_EXPORT
BOOL
NTAPI
XeClientCancelTask()
{
	return g_Server && g_Server->XeClientCancelTask();
}


#if ML_X86
#pragma comment(linker, "/EXPORT:XeClientTaskCloseWindow=_XeClientTaskCloseWindow@0")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeClientTaskCloseWindow=XeClientTaskCloseWindow")
#endif
EXTC_EXPORT
BOOL
NTAPI
XeClientTaskCloseWindow()
{
	return g_Server && g_Server->XeClientTaskCloseWindow();
}



#if ML_X86
#pragma comment(linker, "/EXPORT:XeGetRemoteProcessHandle=_XeGetRemoteProcessHandle@0")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeGetRemoteProcessHandle=XeGetRemoteProcessHandle")
#endif
EXTC_EXPORT
HANDLE
NTAPI
XeGetRemoteProcessHandle()
{
	return g_Server ? g_Server->XeGetRemoteProcessHandle() : 0;
}


#if ML_X86
#pragma comment(linker, "/EXPORT:XeShutdownServer=_XeShutdownServer@0")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeShutdownServer=XeShutdownServer")
#endif
EXTC_EXPORT
BOOL
NTAPI
XeShutdownServer()
{
	return g_Server && g_Server->XeShutdownServer();
}


EXTC_EXPORT
PCSTR
NTAPI
XeGetEventMsg()
{
	BOOL                Success;
	EventMsg*           Msg;
	static std::string  JsonMsg;

	SectionProtector<RTL_CRITICAL_SECTION> Protection(&EventSection);

	auto SerializeToString = [](EventMsg::MsgKind Kind, EventMsg* Msg, ULONG Limit = 0)->std::string
	{
		picojson::object Root;

		switch (Kind)
		{
		case EventMsg::MsgKind::M_NotifyServerProgressChanged:
			Root["kind"] = picojson::value(
				static_cast<int64_t>(EventMsg::MsgKind::M_NotifyServerProgressChanged)
			);
			Root["current"] = picojson::value(
				static_cast<int64_t>(static_cast<NotifyServerProgressChangedMsg*>(Msg)->GetCurrent())
			);
			Root["total"] = picojson::value(
				static_cast<int64_t>(static_cast<NotifyServerProgressChangedMsg*>(Msg)->GetTotal())
			);
			Root["taskname"] = picojson::value(
				Utf16ToUtf8(static_cast<NotifyServerProgressChangedMsg*>(Msg)->GetTaskName())
			);
			break;

		case EventMsg::MsgKind::M_NotifyServerLogOutput:
			Root["kind"] = picojson::value(
				static_cast<int64_t>(EventMsg::MsgKind::M_NotifyServerLogOutput)
			);
			Root["level"] = picojson::value(
				static_cast<int64_t>(static_cast<NotifyServerLogOutputMsg*>(Msg)->GetLevel())
			);
			Root["level"] = picojson::value(
				Utf16ToUtf8(static_cast<NotifyServerLogOutputMsg*>(Msg)->GetMessageW())
			);
			Root["iscmd"] = picojson::value(
				static_cast<bool>(static_cast<NotifyServerLogOutputMsg*>(Msg)->GetIsCmd())
			);
			break;

		case EventMsg::MsgKind::M_NotifyServerUIReady:
			Root["kind"] = picojson::value(
				static_cast<int64_t>(EventMsg::MsgKind::M_NotifyServerUIReady)
			);
			break;

		case EventMsg::MsgKind::M_NotifyServerMessageBox:
			Root["kind"] = picojson::value(
				static_cast<int64_t>(EventMsg::MsgKind::M_NotifyServerMessageBox)
			);
			Root["flags"] = picojson::value(
				static_cast<int64_t>(static_cast<NotifyServerMessageBoxMsg*>(Msg)->GetFlags())
			);
			Root["locked"] = picojson::value(
				static_cast<bool>(static_cast<NotifyServerMessageBoxMsg*>(Msg)->GetLocked())
			);
			Root["locked"] = picojson::value(
				Utf16ToUtf8(static_cast<NotifyServerMessageBoxMsg*>(Msg)->GetDescription())
			);
			break;

		case EventMsg::MsgKind::M_NotifyServerTaskStartAndDisableUI:
			Root["kind"] = picojson::value(
				static_cast<int64_t>(EventMsg::MsgKind::M_NotifyServerTaskStartAndDisableUI)
			);
			break;

		case EventMsg::MsgKind::M_NotifyServerTaskEndAndEnableUI:
			Root["kind"] = picojson::value(
				static_cast<int64_t>(EventMsg::MsgKind::M_NotifyServerTaskEndAndEnableUI)
			);
			break;

		case EventMsg::MsgKind::M_NotifyServerExitFromRemoteProcess:
			Root["kind"] = picojson::value(
				static_cast<int64_t>(EventMsg::MsgKind::M_NotifyServerExitFromRemoteProcess)
			);
			break;

		case EventMsg::MsgKind::M_NotifyServerRaiseError:
			Root["kind"] = picojson::value(
				static_cast<int64_t>(EventMsg::MsgKind::M_NotifyServerExitFromRemoteProcess)
			);

			Root["type"] = picojson::value(
				static_cast<int64_t>(static_cast<NotifyServerRaiseErrorMsg*>(Msg)->GetErrorType())
			);

			Root["msg"] = picojson::value(
				Utf16ToUtf8(static_cast<NotifyServerRaiseErrorMsg*>(Msg)->GetError())
			);
			break;

		default:
			PrintConsoleW(L"XeGetEventMsg : Unknown message type : %d\n", static_cast<ULONG>(Kind));
			break;
		}

		return picojson::value(Root).serialize();
	};

	Msg = nullptr;
	Success = g_Server && g_Server->XeGetEventMsg(&Msg);
	if (Success) {
		if (!Msg) {
			PrintConsoleW(L"XeGetEventMsg : Msg is nullptr\n");
			return nullptr;
		}

		const auto Kind = Msg->GetMessageKind();
		JsonMsg = SerializeToString(Kind, Msg);
		delete Msg;
		return JsonMsg.c_str();
	}

	if (Msg) delete Msg;
	return nullptr;
}


#if ML_X86
#pragma comment(linker, "/EXPORT:XePeIs64Process=_XePeIs64Process@12")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XePeIs64Process=XePeIs64Process")
#endif
EXTC_EXPORT
BOOL
NTAPI
XePeIs64Process(
	_In_  PCWSTR Path,
	_Out_ PBOOL  Is64Bit,
	_Out_ PDWORD Machine
)
{
	NTSTATUS            Status;
	NtFileDisk          File;
	PIMAGE_DOS_HEADER   DosHeader;
	PIMAGE_NT_HEADERS   NtHeader;
	static BYTE         Buffer[0x1000];

	if (!Is64Bit)
		return FALSE;

	*Is64Bit = FALSE;
	Status = File.Open(Path);
	if (NT_FAILED(Status))
		return FALSE;

	RtlZeroMemory(Buffer, sizeof(Buffer));
	Status = File.Read(Buffer, sizeof(Buffer));
	if (NT_FAILED(Status))
		return FALSE;

	DosHeader = (PIMAGE_DOS_HEADER)Buffer;
	NtHeader  = (PIMAGE_NT_HEADERS)((PBYTE)Buffer + DosHeader->e_lfanew);
	if (!IN_RANGEEX((PBYTE)NtHeader, Buffer, Buffer + sizeof(Buffer)))
		return FALSE;

	switch (NtHeader->FileHeader.Machine)
	{
	case IMAGE_FILE_MACHINE_AMD64:
	case IMAGE_FILE_MACHINE_ARM64:
		*Is64Bit = TRUE;
		break;
	} 

	if (Machine) {
		*Machine = NtHeader->FileHeader.Machine;
	}

	return TRUE;
}


#if ML_X86
#pragma comment(linker, "/EXPORT:XeCreateProcessWithDll=_XeCreateProcessWithDll@20")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeCreateProcessWithDll=XeCreateProcessWithDll")
#endif
EXTC_EXPORT
BOOL
NTAPI
XeCreateProcessWithDll(
	_In_  PCWSTR ProcessPath,
	_In_  PCWSTR DllPath,
	_In_  BOOL   Suspend,
	_In_  BOOL   FreeBuffer,
	_Out_ PDWORD ProcessID
)
{
	BOOL                Status;
	STARTUPINFOW        StartInfo;
	PROCESS_INFORMATION ProcessInfo;
	DWORD               Length;
	DWORD               Flags;
	DWORD               Machine;
	BOOL                IsValid64Bit;
	HCRYPTPROV          CryptProv;
	union {
		BYTE            HashData[16];
		INT             DwordData[4];
	};
	WCHAR               PrivId[0x100];

	if (ProcessID) {
		*ProcessID = 0;
	}

	RtlZeroMemory(PrivId, sizeof(PrivId));

	LOOP_ONCE
	{
		Status = CryptAcquireContextW(
			&CryptProv,
			NULL,
			(LPCWSTR)L"Microsoft Base Cryptographic Provider v1.0",
			PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT
		);
		
		if (!Status)
			break;

		Status = CryptGenRandom(
			CryptProv,
			16,
			HashData
		);

		if (!Status) {
			CryptReleaseContext(CryptProv, 0);
			break;
		}

		FormatStringW(
			PrivId,
			L"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			HashData[0],
			HashData[1],
			HashData[2],
			HashData[3],
			HashData[4],
			HashData[5],
			HashData[6],
			HashData[7],
			HashData[8],
			HashData[9],
			HashData[10],
			HashData[11],
			HashData[12],
			HashData[13],
			HashData[14],
			HashData[15]
		);

		CryptReleaseContext(CryptProv, 0);
	};

	if (!Status) {
		srand(time(nullptr));
		DwordData[0] = rand();
		DwordData[1] = rand();
		DwordData[2] = rand();
		DwordData[3] = rand();

		FormatStringW(
			PrivId,
			L"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			HashData[0],
			HashData[1],
			HashData[2],
			HashData[3],
			HashData[4],
			HashData[5],
			HashData[6],
			HashData[7],
			HashData[8],
			HashData[9],
			HashData[10],
			HashData[11],
			HashData[12],
			HashData[13],
			HashData[14],
			HashData[15]
		);
	}

	SetEnvironmentVariableW(L"KrkrRunMode",          L"Remote");
	SetEnvironmentVariableW(L"KrkrRpcIoPrivatePath", PrivId);

	RtlZeroMemory(&StartInfo, sizeof(StartInfo));
	RtlZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
	StartInfo.cb = sizeof(StartInfo);

	if (GetFileAttributesW(DllPath) == -1) {
		SetLastError(ERROR_DLL_NOT_FOUND);
		return FALSE;
	}

	if (Suspend) {
		PrintConsoleW(L"Process is going to be created with `CREATE_SUSPENDED` flag\n");
		Flags = CREATE_SUSPENDED;
	}
	else {
		Flags = 0;
	}


	Status = XePeIs64Process(ProcessPath, &IsValid64Bit, &Machine);
	if (!Status) {
		PrintConsoleW(L"Unable to open or parse pe file : %s\n", ProcessPath);
		return FALSE;
	}


	switch (Machine)
	{
	case IMAGE_FILE_MACHINE_I386:
		Status = CreateProcessWithDllW_ANY_TO_I386(
			ProcessPath,
			NULL,
			NULL,
			NULL,
			FALSE,
			Flags,
			NULL, 
			NULL,
			&StartInfo,
			&ProcessInfo,
			DllPath
		);
		break;

	case IMAGE_FILE_MACHINE_AMD64:
		Status = CreateProcessWithDllW_ANYX86_TO_AMD64(
			ProcessPath,
			NULL,
			NULL,
			NULL,
			FALSE,
			Flags,
			NULL,
			NULL,
			&StartInfo,
			&ProcessInfo,
			DllPath
		);
		break;

	default:
		PrintConsoleW(L"Unsupported machine type : %d", Machine);
		return FALSE;
	}

	if (Status && ProcessID) {
		*ProcessID = ProcessInfo.dwProcessId;
	}
	
	return TRUE;
}




