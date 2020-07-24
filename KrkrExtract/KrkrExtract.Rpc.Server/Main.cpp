#include <my.h>
#include <atomic>
#include "ServerImpl.h"
#include "SectionProtector.h"

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

	BOOL XeRunServer(
		NotifyServerProgressChangedCallback       NotifyServerProgressChangedStub,
		NotifyServerLogOutputCallback             NotifyServerLogOutputStub,
		NotifyServerUIReadyCallback               NotifyServerUIReadyStub,
		NotifyServerMessageBoxCallback            NotifyServerMessageBoxStub,
		NotifyServerTaskStartAndDisableUICallback NotifyServerTaskStartAndDisableUIStub,
		NotifyServerTaskEndAndEnableUICallback    NotifyServerTaskEndAndEnableUIStub,
		NotifyServerRaiseErrorCallback            NotifyServerRaiseErrorStub
	)
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return FALSE;

		return m_Server->RunServer(
			NotifyServerProgressChangedStub,
			NotifyServerLogOutputStub,
			NotifyServerUIReadyStub,
			NotifyServerMessageBoxStub,
			NotifyServerTaskStartAndDisableUIStub,
			NotifyServerTaskEndAndEnableUIStub,
			NotifyServerRaiseErrorStub
		);
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

BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
		LdrDisableThreadCalloutsForDll(hModule);
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
#pragma comment(linker, "/EXPORT:XeRunServer=_XeRunServer@28")
#elif ML_ARM64
#pragma comment(linker, "/EXPORT:XeRunServer=XeRunServer")
#endif
EXTC_EXPORT
BOOL 
NTAPI 
XeRunServer(
	NotifyServerProgressChangedCallback       NotifyServerProgressChangedStub,
	NotifyServerLogOutputCallback             NotifyServerLogOutputStub,
	NotifyServerUIReadyCallback               NotifyServerUIReadyStub,
	NotifyServerMessageBoxCallback            NotifyServerMessageBoxStub,
	NotifyServerTaskStartAndDisableUICallback NotifyServerTaskStartAndDisableUIStub,
	NotifyServerTaskEndAndEnableUICallback    NotifyServerTaskEndAndEnableUIStub,
	NotifyServerRaiseErrorCallback            NotifyServerRaiseErrorStub
)
{
	if (g_Server)
	{
		return g_Server->XeRunServer(
			NotifyServerProgressChangedStub,
			NotifyServerLogOutputStub,
			NotifyServerUIReadyStub,
			NotifyServerMessageBoxStub,
			NotifyServerTaskStartAndDisableUIStub,
			NotifyServerTaskEndAndEnableUIStub,
			NotifyServerRaiseErrorStub
		);
	}

	return FALSE;
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
	if (g_Server)
	{
		return g_Server->XeClientUniversalDumperModeChecked(
			PsbMode,
			TextMode,
			PngMode,
			Tjs2Mode,
			TlgMode,
			AmvMode,
			PdbMode
		);
	}

	return FALSE;
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
	if (g_Server)
	{
		return g_Server->XeClientUniversalPatchMakeChecked(
			Protect,
			Icon
		);
	}

	return FALSE;
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
	if (g_Server)
	{
		return g_Server->XeClientPackerChecked(
			BaseDir,
			OriginalArchiveName,
			OutputArchiveName
		);
	}

	return FALSE;
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
	if (g_Server)
	{
		return g_Server->XeClientCommandEmitted(
			Command
		);
	}

	return FALSE;
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
	if (g_Server)
	{
		return g_Server->XeClientTaskDumpStart(
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

	return FALSE;
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
	if (g_Server)
		return g_Server->XeClientCancelTask();

	return FALSE;
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
	if (g_Server)
		return g_Server->XeClientTaskCloseWindow();

	return FALSE;
}


