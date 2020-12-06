#include <my.h>
#include <atomic>
#include <string>
#include <shlwapi.h>
#include <Psapi.h>
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
		NotifyServerExitFromRemoteProcessCallback NotifyServerExitFromRemoteProcessStub,
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
			NotifyServerExitFromRemoteProcessStub,
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

	HANDLE XeGetRemoteProcessHandle()
	{
		SectionProtector<RTL_CRITICAL_SECTION> Protection(&m_CriticalSection);

		if (!m_Server)
			return FALSE;

		return m_Server->GetRemoteProcessHandle();
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
	NotifyServerExitFromRemoteProcessCallback NotifyServerExitFromRemoteProcessStub,
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
			NotifyServerExitFromRemoteProcessStub,
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
	if (g_Server)
		return g_Server->XeGetRemoteProcessHandle();

	return NULL;
}


//-------------------------------------------------



HMODULE WINAPI GetRemoteModuleHandle(HANDLE hProcess, LPCSTR lpModuleName)
{
	HMODULE* ModuleArray = NULL;
	DWORD ModuleArraySize = 100;
	DWORD NumModules = 0;
	WCHAR lpModuleNameCopy[MAX_PATH] = { 0 };
	WCHAR ModuleNameBuffer[MAX_PATH] = { 0 };

	/* Make sure we didn't get a NULL pointer for the module name */
	if (lpModuleName == NULL)
		goto GRMH_FAIL_JMP;

	/* Convert lpModuleName to all lowercase so the comparison isn't case sensitive */
	for (size_t i = 0; lpModuleName[i] != '\0'; ++i)
	{
		if (lpModuleName[i] >= L'A' && lpModuleName[i] <= L'Z')
			lpModuleNameCopy[i] = lpModuleName[i] + 0x20; // 0x20 is the difference between uppercase and lowercase
		else
			lpModuleNameCopy[i] = lpModuleName[i];

		lpModuleNameCopy[i + 1] = '\0';
	}

	/* Allocate memory to hold the module handles */
	ModuleArray = new HMODULE[ModuleArraySize];

	/* Check if the allocation failed */
	if (ModuleArray == NULL)
		goto GRMH_FAIL_JMP;

	/* Get handles to all the modules in the target process */
	if (!::EnumProcessModulesEx(hProcess, ModuleArray,
		ModuleArraySize * sizeof(HMODULE), &NumModules, LIST_MODULES_ALL))
		goto GRMH_FAIL_JMP;

	/* We want the number of modules not the number of bytes */
	NumModules /= sizeof(HMODULE);

	/* Did we allocate enough memory for all the module handles? */
	if (NumModules > ModuleArraySize)
	{
		delete[] ModuleArray; // Deallocate so we can try again
		ModuleArray = NULL; // Set it to NULL se we can be sure if the next try fails
		ModuleArray = new HMODULE[NumModules]; // Allocate the right amount of memory

		/* Check if the allocation failed */
		if (ModuleArray == NULL)
			goto GRMH_FAIL_JMP;

		ModuleArraySize = NumModules; // Update the size of the array

		/* Get handles to all the modules in the target process */
		if (!::EnumProcessModulesEx(hProcess, ModuleArray,
			ModuleArraySize * sizeof(HMODULE), &NumModules, LIST_MODULES_ALL))
			goto GRMH_FAIL_JMP;

		/* We want the number of modules not the number of bytes */
		NumModules /= sizeof(HMODULE);
	}

	/* Iterate through all the modules and see if the names match the one we are looking for */
	for (DWORD i = 0; i <= NumModules; ++i)
	{
		/* Get the module's name */
		::GetModuleBaseName(hProcess, ModuleArray[i],
			ModuleNameBuffer, sizeof(ModuleNameBuffer));

		/* Convert ModuleNameBuffer to all lowercase so the comparison isn't case sensitive */
		for (size_t j = 0; ModuleNameBuffer[j] != '\0'; ++i)
		{
			if (ModuleNameBuffer[j] >= L'A' && ModuleNameBuffer[j] <= L'Z')
				ModuleNameBuffer[j] += 0x20; // 0x20 is the difference between uppercase and lowercase
		}

		/* Does the name match? */
		if (wcsstr(ModuleNameBuffer, lpModuleNameCopy) != NULL)
		{
			/* Make a temporary variable to hold return value*/
			HMODULE TempReturn = ModuleArray[i];

			/* Give back that memory */
			delete[] ModuleArray;

			/* Success */
			return TempReturn;
		}

		/* Wrong module let's try the next... */
	}

	/* Uh Oh... */
GRMH_FAIL_JMP:

	/* If we got to the point where we allocated memory we need to give it back */
	if (ModuleArray != NULL)
		delete[] ModuleArray;

	/* Failure... */
	return NULL;
}


//-----------------------------------------------------------------------------

FARPROC WINAPI GetRemoteProcAddress(HANDLE hProcess, HMODULE hModule, LPCSTR lpProcName, UINT Ordinal, BOOL UseOrdinal)
{
	BOOL Is64Bit = FALSE;
	MODULEINFO RemoteModuleInfo = { 0 };
	UINT_PTR RemoteModuleBaseVA = 0;
	IMAGE_DOS_HEADER DosHeader = { 0 };
	DWORD Signature = 0;
	IMAGE_FILE_HEADER FileHeader = { 0 };
	IMAGE_OPTIONAL_HEADER64 OptHeader64 = { 0 };
	IMAGE_OPTIONAL_HEADER32 OptHeader32 = { 0 };
	IMAGE_DATA_DIRECTORY ExportDirectory = { 0 };
	IMAGE_EXPORT_DIRECTORY ExportTable = { 0 };
	UINT_PTR ExportFunctionTableVA = 0;
	UINT_PTR ExportNameTableVA = 0;
	UINT_PTR ExportOrdinalTableVA = 0;
	DWORD* ExportFunctionTable = NULL;
	DWORD* ExportNameTable = NULL;
	WORD* ExportOrdinalTable = NULL;

	/* Temporary variables not used until much later but easier
	/* to define here than in all the the places they are used */
	CHAR TempChar;
	BOOL Done = FALSE;

	/* Check to make sure we didn't get a NULL pointer for the name unless we are searching by ordinal */
	if (lpProcName == NULL && !UseOrdinal)
		goto GRPA_FAIL_JMP;

	/* Get the base address of the remote module along with some other info we don't need */
	if (!::GetModuleInformation(hProcess, hModule, &RemoteModuleInfo, sizeof(RemoteModuleInfo)))
		goto GRPA_FAIL_JMP;
	RemoteModuleBaseVA = (UINT_PTR)RemoteModuleInfo.lpBaseOfDll;

	/* Read the DOS header and check it's magic number */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)RemoteModuleBaseVA, &DosHeader,
		sizeof(DosHeader), NULL) || DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
		goto GRPA_FAIL_JMP;

	/* Read and check the NT signature */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)(RemoteModuleBaseVA + DosHeader.e_lfanew),
		&Signature, sizeof(Signature), NULL) || Signature != IMAGE_NT_SIGNATURE)
		goto GRPA_FAIL_JMP;

	/* Read the main header */
	if (!::ReadProcessMemory(hProcess,
		(LPCVOID)(RemoteModuleBaseVA + DosHeader.e_lfanew + sizeof(Signature)),
		&FileHeader, sizeof(FileHeader), NULL))
		goto GRPA_FAIL_JMP;

	/* Which type of optional header is the right size? */
	if (FileHeader.SizeOfOptionalHeader == sizeof(OptHeader64))
		Is64Bit = TRUE;
	else if (FileHeader.SizeOfOptionalHeader == sizeof(OptHeader32))
		Is64Bit = FALSE;
	else
		goto GRPA_FAIL_JMP;

	if (Is64Bit)
	{
		/* Read the optional header and check it's magic number */
		if (!::ReadProcessMemory(hProcess,
			(LPCVOID)(RemoteModuleBaseVA + DosHeader.e_lfanew + sizeof(Signature) + sizeof(FileHeader)),
			&OptHeader64, FileHeader.SizeOfOptionalHeader, NULL)
			|| OptHeader64.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			goto GRPA_FAIL_JMP;
	}
	else
	{
		/* Read the optional header and check it's magic number */
		if (!::ReadProcessMemory(hProcess,
			(LPCVOID)(RemoteModuleBaseVA + DosHeader.e_lfanew + sizeof(Signature) + sizeof(FileHeader)),
			&OptHeader32, FileHeader.SizeOfOptionalHeader, NULL)
			|| OptHeader32.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			goto GRPA_FAIL_JMP;
	}

	/* Make sure the remote module has an export directory and if it does save it's relative address and size */
	if (Is64Bit && OptHeader64.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_EXPORT + 1)
	{
		ExportDirectory.VirtualAddress = (OptHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).VirtualAddress;
		ExportDirectory.Size = (OptHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).Size;
	}
	else if (OptHeader32.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_EXPORT + 1)
	{
		ExportDirectory.VirtualAddress = (OptHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).VirtualAddress;
		ExportDirectory.Size = (OptHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).Size;
	}
	else
		goto GRPA_FAIL_JMP;

	/* Read the main export table */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)(RemoteModuleBaseVA + ExportDirectory.VirtualAddress),
		&ExportTable, sizeof(ExportTable), NULL))
		goto GRPA_FAIL_JMP;

	/* Save the absolute address of the tables so we don't need to keep adding the base address */
	ExportFunctionTableVA = RemoteModuleBaseVA + ExportTable.AddressOfFunctions;
	ExportNameTableVA = RemoteModuleBaseVA + ExportTable.AddressOfNames;
	ExportOrdinalTableVA = RemoteModuleBaseVA + ExportTable.AddressOfNameOrdinals;

	/* Allocate memory for our copy of the tables */
	ExportFunctionTable = new DWORD[ExportTable.NumberOfFunctions];
	ExportNameTable = new DWORD[ExportTable.NumberOfNames];
	ExportOrdinalTable = new WORD[ExportTable.NumberOfNames];

	/* Check if the allocation failed */
	if (ExportFunctionTable == NULL || ExportNameTable == NULL || ExportOrdinalTable == NULL)
		goto GRPA_FAIL_JMP;

	/* Get a copy of the function table */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)ExportFunctionTableVA,
		ExportFunctionTable, ExportTable.NumberOfFunctions * sizeof(DWORD), NULL))
		goto GRPA_FAIL_JMP;

	/* Get a copy of the name table */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)ExportNameTableVA,
		ExportNameTable, ExportTable.NumberOfNames * sizeof(DWORD), NULL))
		goto GRPA_FAIL_JMP;

	/* Get a copy of the ordinal table */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)ExportOrdinalTableVA,
		ExportOrdinalTable, ExportTable.NumberOfNames * sizeof(WORD), NULL))
		goto GRPA_FAIL_JMP;

	/* If we are searching for an ordinal we do that now */
	if (UseOrdinal)
	{
		/* NOTE:
		/* Microsoft's PE/COFF specification does NOT say we need to subtract the ordinal base
		/* from our ordinal but it seems to always give the wrong function if we don't */

		/* Make sure the ordinal is valid */
		if (Ordinal < ExportTable.Base || (Ordinal - ExportTable.Base) >= ExportTable.NumberOfFunctions)
			goto GRPA_FAIL_JMP;

		UINT FunctionTableIndex = Ordinal - ExportTable.Base;

		/* Check if the function is forwarded and if so get the real address*/
		if (ExportFunctionTable[FunctionTableIndex] >= ExportDirectory.VirtualAddress &&
			ExportFunctionTable[FunctionTableIndex] <= ExportDirectory.VirtualAddress + ExportDirectory.Size)
		{
			Done = FALSE;
			std::string TempForwardString;
			TempForwardString.clear(); // Empty the string so we can fill it with a new name

			/* Get the forwarder string one character at a time because we don't know how long it is */
			for (UINT_PTR i = 0; !Done; ++i)
			{
				/* Get next character */
				if (!::ReadProcessMemory(hProcess,
					(LPCVOID)(RemoteModuleBaseVA + ExportFunctionTable[FunctionTableIndex] + i),
					&TempChar, sizeof(TempChar), NULL))
					goto GRPA_FAIL_JMP;

				TempForwardString.push_back(TempChar); // Add it to the string

				/* If it's NUL we are done */
				if (TempChar == (CHAR)'\0')
					Done = TRUE;
			}

			/* Find the dot that seperates the module name and the function name/ordinal */
			size_t Dot = TempForwardString.find('.');
			if (Dot == std::string::npos)
				goto GRPA_FAIL_JMP;

			/* Temporary variables that hold parts of the forwarder string */
			std::string RealModuleName, RealFunctionId;
			RealModuleName = TempForwardString.substr(0, Dot - 1);
			RealFunctionId = TempForwardString.substr(Dot + 1, std::string::npos);

			HMODULE RealModule = GetRemoteModuleHandle(hProcess, RealModuleName.c_str());
			FARPROC TempReturn;// Make a temporary variable to hold return value 


			/* Figure out if the function was exported by name or by ordinal */
			if (RealFunctionId.at(0) == '#') // Exported by ordinal
			{
				UINT RealOrdinal = 0;
				RealFunctionId.erase(0, 1); // Remove '#' from string

				/* My version of atoi() because I was too lazy to use the real one... */
				for (size_t i = 0; i < RealFunctionId.size(); ++i)
				{
					if (RealFunctionId[i] >= '0' && RealFunctionId[i] <= '9')
					{
						RealOrdinal *= 10;
						RealOrdinal += RealFunctionId[i] - '0';
					}
					else
						break;
				}

				/* Recursively call this function to get return value */
				TempReturn = GetRemoteProcAddress(hProcess, RealModule, NULL, RealOrdinal, TRUE);
			}
			else // Exported by name
			{
				/* Recursively call this function to get return value */
				TempReturn = GetRemoteProcAddress(hProcess, RealModule, RealFunctionId.c_str(), 0, FALSE);
			}

			/* Give back that memory */
			delete[] ExportFunctionTable;
			delete[] ExportNameTable;
			delete[] ExportOrdinalTable;

			/* Success!!! */
			return TempReturn;
		}
		else // Not Forwarded
		{

			/* Make a temporary variable to hold return value*/
			FARPROC TempReturn = (FARPROC)(RemoteModuleBaseVA + ExportFunctionTable[FunctionTableIndex]);

			/* Give back that memory */
			delete[] ExportFunctionTable;
			delete[] ExportNameTable;
			delete[] ExportOrdinalTable;

			/* Success!!! */
			return TempReturn;
		}
	}


	/* Iterate through all the names and see if they match the one we are looking for */
	for (DWORD i = 0; i < ExportTable.NumberOfNames; ++i) {
		std::string TempFunctionName;

		Done = FALSE;// Reset for next name
		TempFunctionName.clear(); // Empty the string so we can fill it with a new name

		/* Get the function name one character at a time because we don't know how long it is */
		for (UINT_PTR j = 0; !Done; ++j)
		{
			/* Get next character */
			if (!::ReadProcessMemory(hProcess, (LPCVOID)(RemoteModuleBaseVA + ExportNameTable[i] + j),
				&TempChar, sizeof(TempChar), NULL))
				goto GRPA_FAIL_JMP;

			TempFunctionName.push_back(TempChar); // Add it to the string

			/* If it's NUL we are done */
			if (TempChar == (CHAR)'\0')
				Done = TRUE;
		}

		/* Does the name match? */
		if (TempFunctionName.find(lpProcName) != std::string::npos)
		{
			/* NOTE:
			/* Microsoft's PE/COFF specification says we need to subtract the ordinal base
			/*from the value in the ordinal table but that seems to always give the wrong function */

			/* Check if the function is forwarded and if so get the real address*/
			if (ExportFunctionTable[ExportOrdinalTable[i]] >= ExportDirectory.VirtualAddress &&
				ExportFunctionTable[ExportOrdinalTable[i]] <= ExportDirectory.VirtualAddress + ExportDirectory.Size)
			{
				Done = FALSE;
				std::string TempForwardString;
				TempForwardString.clear(); // Empty the string so we can fill it with a new name

				/* Get the forwarder string one character at a time because we don't know how long it is */
				for (UINT_PTR j = 0; !Done; ++j)
				{
					/* Get next character */
					if (!::ReadProcessMemory(hProcess,
						(LPCVOID)(RemoteModuleBaseVA + ExportFunctionTable[i] + j),
						&TempChar, sizeof(TempChar), NULL))
						goto GRPA_FAIL_JMP;

					TempForwardString.push_back(TempChar); // Add it to the string

					/* If it's NUL we are done */
					if (TempChar == (CHAR)'\0')
						Done = TRUE;
				}

				/* Find the dot that seperates the module name and the function name/ordinal */
				size_t Dot = TempForwardString.find('.');
				if (Dot == std::string::npos)
					goto GRPA_FAIL_JMP;

				/* Temporary variables that hold parts of the forwarder string */
				std::string RealModuleName, RealFunctionId;
				RealModuleName = TempForwardString.substr(0, Dot);
				RealFunctionId = TempForwardString.substr(Dot + 1, std::string::npos);

				HMODULE RealModule = GetRemoteModuleHandle(hProcess, RealModuleName.c_str());
				FARPROC TempReturn;// Make a temporary variable to hold return value 


				/* Figure out if the function was exported by name or by ordinal */
				if (RealFunctionId.at(0) == '#') // Exported by ordinal
				{
					UINT RealOrdinal = 0;
					RealFunctionId.erase(0, 1); // Remove '#' from string

					/* My version of atoi() because I was to lazy to use the real one... */
					for (size_t i = 0; i < RealFunctionId.size(); ++i)
					{
						if (RealFunctionId[i] >= '0' && RealFunctionId[i] <= '9')
						{
							RealOrdinal *= 10;
							RealOrdinal += RealFunctionId[i] - '0';
						}
						else
							break;
					}

					/* Recursively call this function to get return value */
					TempReturn = GetRemoteProcAddress(hProcess, RealModule, NULL, RealOrdinal, TRUE);
				}
				else // Exported by name
				{
					/* Recursively call this function to get return value */
					TempReturn = GetRemoteProcAddress(hProcess, RealModule, RealFunctionId.c_str(), 0, FALSE);
				}

				/* Give back that memory */
				delete[] ExportFunctionTable;
				delete[] ExportNameTable;
				delete[] ExportOrdinalTable;

				/* Success!!! */
				return TempReturn;
			}
			else // Not Forwarded
			{

				/* Make a temporary variable to hold return value*/
				FARPROC TempReturn;

				/* NOTE:
				/* Microsoft's PE/COFF specification says we need to subtract the ordinal base
				/*from the value in the ordinal table but that seems to always give the wrong function */
				//TempReturn = (FARPROC)(RemoteModuleBaseVA + ExportFunctionTable[ExportOrdinalTable[i] - ExportTable.Base]);

				/* So we do it this way instead */
				TempReturn = (FARPROC)(RemoteModuleBaseVA + ExportFunctionTable[ExportOrdinalTable[i]]);

				/* Give back that memory */
				delete[] ExportFunctionTable;
				delete[] ExportNameTable;
				delete[] ExportOrdinalTable;

				/* Success!!! */
				return TempReturn;
			}
		}

		/* Wrong function let's try the next... */
	}

	/* Uh Oh... */
GRPA_FAIL_JMP:

	/* If we got to the point where we allocated memory we need to give it back */
	if (ExportFunctionTable != NULL)
		delete[] ExportFunctionTable;
	if (ExportNameTable != NULL)
		delete[] ExportNameTable;
	if (ExportOrdinalTable != NULL)
		delete[] ExportOrdinalTable;

	/* Falure... */
	return NULL;
}




DWORD GetFuncAdress()
{
	return (DWORD)GetProcAddress(GetModuleHandleA("Kernel32"), "LoadLibraryW");
}

typedef BOOL(WINAPI* Proc_CreateProcessW)(LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation);

typedef HMODULE(WINAPI* Func_LoadLibraryW)(LPCWSTR lpLibFileName);


BYTE* mov_eax_xx(BYTE* lpCurAddres, DWORD eax)
{
	*lpCurAddres = 0xB8;
	*(DWORD*)(lpCurAddres + 1) = eax;
	return lpCurAddres + 5;
}

BYTE* mov_ebx_xx(BYTE* lpCurAddres, DWORD ebx)
{
	*lpCurAddres = 0xBB;
	*(DWORD*)(lpCurAddres + 1) = ebx;
	return lpCurAddres + 5;
}

BYTE* mov_ecx_xx(BYTE* lpCurAddres, DWORD ecx)
{
	*lpCurAddres = 0xB9;
	*(DWORD*)(lpCurAddres + 1) = ecx;
	return lpCurAddres + 5;
}

BYTE* mov_edx_xx(BYTE* lpCurAddres, DWORD edx)
{
	*lpCurAddres = 0xBA;
	*(DWORD*)(lpCurAddres + 1) = edx;
	return lpCurAddres + 5;
}

BYTE* mov_esi_xx(BYTE* lpCurAddres, DWORD esi)
{
	*lpCurAddres = 0xBE;
	*(DWORD*)(lpCurAddres + 1) = esi;
	return lpCurAddres + 5;
}

BYTE* mov_edi_xx(BYTE* lpCurAddres, DWORD edi)
{
	*lpCurAddres = 0xBF;
	*(DWORD*)(lpCurAddres + 1) = edi;
	return lpCurAddres + 5;
}

BYTE* mov_ebp_xx(BYTE* lpCurAddres, DWORD ebp)
{
	*lpCurAddres = 0xBD;
	*(DWORD*)(lpCurAddres + 1) = ebp;
	return lpCurAddres + 5;
}

BYTE* mov_esp_xx(BYTE* lpCurAddres, DWORD esp)
{
	*lpCurAddres = 0xBC;
	*(DWORD*)(lpCurAddres + 1) = esp;
	return lpCurAddres + 5;
}

BYTE* mov_eip_xx(BYTE* lpCurAddres, DWORD eip, DWORD newEip)
{
	if (!newEip)
	{
		newEip = (DWORD)lpCurAddres;
	}

	*lpCurAddres = 0xE9;
	*(DWORD*)(lpCurAddres + 1) = eip - (newEip + 5);
	return lpCurAddres + 5;
}

BYTE* push_xx(BYTE* lpCurAddres, DWORD dwAdress)
{

	*lpCurAddres = 0x68;
	*(DWORD*)(lpCurAddres + 1) = dwAdress;

	return lpCurAddres + 5;
}

BYTE* Call_xx(BYTE* lpCurAddres, DWORD eip, DWORD newEip)
{
	if (!newEip)
	{
		newEip = (DWORD)lpCurAddres;
	}

	*lpCurAddres = 0xE8;
	*(DWORD*)(lpCurAddres + 1) = eip - (newEip + 5);
	return lpCurAddres + 5;
}

BOOL SuspendTidAndInjectCode(HANDLE hProcess, HANDLE hThread, DWORD dwFuncAdress, const BYTE * lpShellCode, size_t uCodeSize)
{
	SIZE_T NumberOfBytesWritten = 0;
	BYTE ShellCodeBuf[0x480];
	CONTEXT Context;
	DWORD flOldProtect = 0;
	LPBYTE lpCurESPAddress = NULL;
	LPBYTE lpCurBufAdress = NULL;
	BOOL bResult = FALSE;

	SuspendThread(hThread);

	memset(&Context, 0, sizeof(Context));
	Context.ContextFlags = CONTEXT_FULL;

	if (GetThreadContext(hThread, &Context))
	{
		lpCurESPAddress = (LPBYTE)((Context.Esp - 0x480) & 0xFFFFFFE0);

		lpCurBufAdress = &ShellCodeBuf[0];

		if (lpShellCode)
		{
			memcpy(ShellCodeBuf + 128, lpShellCode, uCodeSize);
			lpCurBufAdress = push_xx(lpCurBufAdress, (DWORD)lpCurESPAddress + 128); // push
			lpCurBufAdress = Call_xx(lpCurBufAdress, dwFuncAdress, (DWORD)lpCurESPAddress + (DWORD)lpCurBufAdress - (DWORD)&ShellCodeBuf); //Call
		}

		lpCurBufAdress = mov_eax_xx(lpCurBufAdress, Context.Eax);
		lpCurBufAdress = mov_ebx_xx(lpCurBufAdress, Context.Ebx);
		lpCurBufAdress = mov_ecx_xx(lpCurBufAdress, Context.Ecx);
		lpCurBufAdress = mov_edx_xx(lpCurBufAdress, Context.Edx);
		lpCurBufAdress = mov_esi_xx(lpCurBufAdress, Context.Esi);
		lpCurBufAdress = mov_edi_xx(lpCurBufAdress, Context.Edi);
		lpCurBufAdress = mov_ebp_xx(lpCurBufAdress, Context.Ebp);
		lpCurBufAdress = mov_esp_xx(lpCurBufAdress, Context.Esp);
		lpCurBufAdress = mov_eip_xx(lpCurBufAdress, Context.Eip, (DWORD)lpCurESPAddress + (DWORD)lpCurBufAdress - (DWORD)&ShellCodeBuf);
		Context.Esp = (DWORD)(lpCurESPAddress - 4);
		Context.Eip = (DWORD)lpCurESPAddress;

		if (VirtualProtectEx(hProcess, lpCurESPAddress, 0x480, PAGE_EXECUTE_READWRITE, &flOldProtect)
			&& WriteProcessMemory(hProcess, lpCurESPAddress, &ShellCodeBuf, 0x480, &NumberOfBytesWritten)
			&& FlushInstructionCache(hProcess, lpCurESPAddress, 0x480)
			&& SetThreadContext(hThread, &Context))
		{
			bResult = TRUE;
		}

	}

	ResumeThread(hThread);

	return TRUE;
}

DWORD GetFuncAdress()
{
	return (DWORD)GetProcAddress(GetModuleHandleA("Kernel32"), "LoadLibraryW");
}

BOOL WINAPI CreateProcessWithDllW(
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	LPCWSTR lpDllFullPath,
	Proc_CreateProcessW FuncAdress
)
{
	BOOL bResult = FALSE;
	size_t uCodeSize = 0;
	DWORD dwCreaFlags;
	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if (FuncAdress == NULL) {
		FuncAdress = CreateProcessW;
	}


	dwCreaFlags = dwCreationFlags | CREATE_SUSPENDED;
	if (CreateProcessW(lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreaFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		&pi
	))
	{
		if (lpDllFullPath)
			uCodeSize = 2 * wcslen(lpDllFullPath) + 2;
		else
			uCodeSize = 0;

		DWORD dwLoadDllProc = GetFuncAdress();

		if (SuspendTidAndInjectCode(pi.hProcess, pi.hThread, dwLoadDllProc, (BYTE*)lpDllFullPath, uCodeSize))
		{
			if (lpProcessInformation) {
				RtlCopyMemory(lpProcessInformation, &pi, sizeof(PROCESS_INFORMATION));
			}

			if (!(dwCreationFlags & CREATE_SUSPENDED)) {
				ResumeThread(pi.hThread);
			}

			bResult = TRUE;
		}
	}

	return bResult;
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
	STARTUPINFOW        StartInfo;
	PROCESS_INFORMATION ProcessInfo;
	WCHAR               CurrentPath[MAX_PATH];
	DWORD               Length;

	if (ProcessID) {
		*ProcessID = 0;
	}

	RtlZeroMemory(&StartInfo, sizeof(StartInfo));
	RtlZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

	RtlZeroMemory(CurrentPath, sizeof(CurrentPath));
	Length = GetCurrentDirectoryW(_countof(CurrentPath), CurrentPath);

	if (GetFileAttributesW(DllPath) == -1) {
		SetLastError(ERROR_DLL_NOT_FOUND);
		return FALSE;
	}

	if (!CreateProcessWithDllW(ProcessPath, NULL, NULL, NULL, FALSE, NULL, NULL, NULL, &StartInfo, &ProcessInfo, DllPath, NULL)) {
		return FALSE;
	}

	if (ProcessID) {
		*ProcessID = ProcessInfo.dwProcessId;
	}

	return TRUE;
}




