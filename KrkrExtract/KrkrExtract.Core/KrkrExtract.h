#pragma once

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 26812 26495)
#endif

#include "tp_stub.h"

#ifdef _MSC_VER
#pragma warning (pop)
#endif

#include <RpcDefine.h>
#include <RpcUIDefine.h>
#include "my.h"
#include "SafeMemory.h"
#include "KrkrHeaders.h"
#include <atomic>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <Shobjidl.h>
#include "SectionProtector.h"
#include "Signals.h"
#include "CoreTask.h"
#include "Prototype.h"
#include "WellknownPlugin.h"
#include <Stubs.h>
#include "KrkrClientProxyer.h"
#include "MemoryEntry.h"
#include "AsyncCommandExecutor.h"
#include "HardwareBreakpoint.h"
#include "trie.h"

#ifdef _MSC_VER
#pragma warning (push)
// warning C4100: unreferenced formal parameter
// warning C4127: conditional expression is constant
// warning C4702: unreachable code
// warning C4800: 'int': forcing value to bool 'true' or 'false' (performance warning)
#pragma warning (disable: 4100 4127 4702 4800 26495)
#endif

#include "GrpcConnectionApi.h"
#include "GrpcCoreApi.h"

#ifdef _MSC_VER
#pragma warning (pop)
#endif

enum class KrkrRunMode
{
	LOCAL_MODE   = 0,
	REMOTE_MODE  = 1,
	MIXED_MODE   = 2
};


class KrkrExtractCore final : 
	public ClientStub,
	public KrkrClientProxyer
{
public:
	KrkrExtractCore();
	~KrkrExtractCore();

	static KrkrExtractCore* GetInstance();

	NTSTATUS Initialize(HMODULE DllModule);
	NTSTATUS InitializePrivatePointers();

private:
	NTSTATUS InitializeHook();

public:
	enum class KrkrVersion : ULONG_PTR
	{
		Unknown = 0,
		KRKR2 = 1,
		KRKRZ = 2,
		KRKR3 = 3
	};

	enum class ModuleInitializationType : ULONG_PTR
	{
		NotInited   = 0,
		ByExeModule = 1,
		ByDllModule = 2
	};

	enum class TVPExporterInitializationType : ULONG_PTR
	{
		NotInited               = 0,
		ByCallingTVPGetExporter = 1,
		ByCallingV2Link         = 2
	};

	std::vector<std::wstring>              FileNameList;

private:
	// Flags (Wellknown)
	KrkrPngMode            m_PngFlag  = KrkrPngMode::PNG_RAW;
	KrkrTlgMode            m_TlgFlag  = KrkrTlgMode::TLG_RAW;
	KrkrTextMode           m_TextFlag = KrkrTextMode::TEXT_RAW;
	KrkrPsbMode            m_PsbFlag  = KrkrPsbMode::PSB_RAW;
	KrkrTjs2Mode           m_TjsFlag  = KrkrTjs2Mode::TJS2_RAW;
	KrkrAmvMode            m_AmvFlag  = KrkrAmvMode::AMV_RAW;
	KrkrPbdMode            m_PbdFlag  = KrkrPbdMode::PBD_RAW;

	// Paths
	std::wstring           m_BaseDir;
	std::wstring           m_OriginalArchivePath;
	std::wstring           m_OutputArchivePath;

	// Packer info
	std::wstring           m_CurrentTempFileName;
	std::atomic<HANDLE>    m_CurrentTempHandle = INVALID_HANDLE_VALUE;

	// Universal patch
	BOOL                   m_InheritIcon    = FALSE;
	BOOL                   m_IsProtection   = FALSE;

	// Tasks
	std::atomic<CoTaskBase*> m_CurrentTask     = nullptr;
	AsyncCommandExecutor*    m_CommandExecutor = nullptr;

	// Debug
	std::atomic<BOOL>      m_ConsoleIsAttached     = FALSE;
	BOOL                   m_DebugOn               = FALSE;

	// Module Info
	ULONG_PTR              m_FirstSectionAddress   = 0;
	ULONG                  m_SizeOfImage           = 0;
	HMODULE                m_SelfModule            = nullptr;
	HMODULE                m_HostModule            = nullptr;
	HMODULE                m_UIModule              = nullptr;
	ServerStub*            m_LocalServer           = nullptr;

	// Decoder, Pointers
	BOOL                                  m_IsSpcialChunkEncrypted = FALSE;
	Prototype::SpecialChunkDecoderFunc    m_SpecialChunkDecoder    = nullptr;
	PVOID                                 m_TVPCreateIStreamStub   = nullptr;
	PVOID                                 m_TVPCreateIStreamP      = nullptr;
	PVOID                                 m_TVPCreateBStream       = nullptr;
	PVOID                                 m_Allocator              = nullptr;
	PVOID                                 m_IStreamAdapterVtable   = nullptr;
	iTVPFunctionExporter*                 m_TVPFunctionExporter    = nullptr;
	std::atomic<tTVPXP3ArchiveExtractionFilter>     m_XP3Filter = nullptr;

	public:
	BOOL m_ShouldHookZlib = FALSE;
	std::wstring InitModulePath;
	PBYTE  InitModuleBase = 0;

	private:
	// utils
	std::atomic<BOOL>                     m_WindowIsActived = FALSE;
	std::atomic<KrkrVersion>              m_ModuleType      = KrkrVersion::Unknown;
	BOOL                                  m_IsAllPackReaded = FALSE;
	std::atomic<KrkrRunMode>              m_RunMode         = KrkrRunMode::LOCAL_MODE;
	std::atomic<ModuleInitializationType> m_InitializationType    = ModuleInitializationType::NotInited;
	std::atomic<TVPExporterInitializationType> m_TVPExporterInitializationType = TVPExporterInitializationType::NotInited;
	std::atomic<BOOL>                          m_PointersInitialized           = FALSE;
	std::unique_ptr<ConnectionApi>             m_ConnectionApi = nullptr;
	HANDLE                                     m_CoreApiThread              = nullptr;
	std::atomic<BOOL>                          m_CoreApiInitialized         = FALSE;
	HANDLE                                     m_CoreApiHeartbeatThread     = nullptr;
	ULONG                                      m_HeartbeatInterval          = 5000;
	
	// Stubs (local or remote)
	ClientStub*  m_Client = nullptr;
	ServerStub*  m_Server = nullptr;

public:

	PVOID NTAPI GetPrivatePointer() { return this; }

	// init
	NTSTATUS InitExporterWithDll(iTVPFunctionExporter *Exporter);
	NTSTATUS InitExporterWithExe(iTVPFunctionExporter *Exporter);

	// Status
	inline ModuleInitializationType GetInitializationType() { return m_InitializationType.load(); };
	inline TVPExporterInitializationType GetTVPExporterInitializationType() { return m_TVPExporterInitializationType.load();  };
	inline KrkrVersion GetModuleType() { return m_ModuleType.load();  };
	inline void SetInitializationType(ModuleInitializationType Type) { m_InitializationType = Type; };
	inline void SetTVPExporterInitializationType(TVPExporterInitializationType Type) { m_TVPExporterInitializationType = Type; };

	// Rpc UI server
	BOOL NTAPI TellServerProgressBar(PCWSTR TaskName, ULONGLONG Current, ULONGLONG Total);
	BOOL NTAPI TellServerLogOutput(LogLevel Level, PCWSTR FormatString, ...);
	BOOL NTAPI TellServerCommandResultOutput(CommandStatus Status, PCWSTR FormatString, ...);
	BOOL NTAPI TellServerUIReady(ULONG ClientPort, PCSTR SessionKey, ULONG Extra);
	BOOL NTAPI TellServerMessageBox(PCWSTR Description, ULONG Flags, BOOL Locked);
	BOOL NTAPI TellServerTaskStartAndDisableUI();
	BOOL NTAPI TellServerTaskEndAndEnableUI(BOOL TaskCompleteStatus, PCWSTR Description);
	BOOL NTAPI TellServerUIHeartbeatPackage();
	BOOL NTAPI TellServerExitFromRemoteProcess();

public:
	VOID   SetCoreApiInitialized()   { m_CoreApiInitialized = TRUE; };
	VOID   SetCoreApiUninitialized() { m_CoreApiInitialized = FALSE; };
	BOOL   IsCoreApiInitialized()  { return m_CoreApiInitialized; };
	BOOL   CreateHeartbeatThread();
	ULONG  GetHeartbeatInterval() { return m_HeartbeatInterval; };
	VOID   MakeHeartbeat();

	//
	// Rpc client
	// process requests
	//

	BOOL NotifyClientUniversalDumperModeChecked(
		KrkrPsbMode PsbMode,
		KrkrTextMode TextMode,
		KrkrPngMode  PngMode,
		KrkrTjs2Mode Tjs2Mode,
		KrkrTlgMode  TlgMode,
		KrkrAmvMode  AmvMode,
		KrkrPbdMode  PdbMode
	);

	BOOL NotifyClientUniversalPatchMakeChecked(
		BOOL Protect,
		BOOL Icon
	);

	BOOL NotifyClientPackerChecked(
		PCWSTR BaseDir,
		PCWSTR OriginalArchiveName,
		PCWSTR OutputArchiveName
	);

	BOOL NotifyClientCommandEmitted(
		PCWSTR Command
	);

	BOOL NotifyClientTaskDumpStart(
		KrkrPsbMode PsbMode,
		KrkrTextMode TextMode,
		KrkrPngMode  PngMode,
		KrkrTjs2Mode Tjs2Mode,
		KrkrTlgMode  TlgMode,
		KrkrAmvMode  AmvMode,
		KrkrPbdMode  PdbMode,
		PCWSTR File
	);

	BOOL NotifyClientCancelTask();
	BOOL NotifyClientTaskCloseWindow();


	// Command
	NTSTATUS NTAPI ParseUtilCommand(PWSTR Command);

	// Hooks bypass
	PVOID NTAPI GetProcAddressBypass(
		PVOID Module,
		PCSTR RoutineName
	);

	BOOL NTAPI CreateProcessBypass(
			HANDLE                  hToken,
			LPCWSTR                 lpApplicationName,
			LPWSTR                  lpCommandLine,
			LPSECURITY_ATTRIBUTES   lpProcessAttributes,
			LPSECURITY_ATTRIBUTES   lpThreadAttributes,
			BOOL                    bInheritHandles,
			ULONG                   dwCreationFlags,
			LPVOID                  lpEnvironment,
			LPCWSTR                 lpCurrentDirectory,
			LPSTARTUPINFOW          lpStartupInfo,
			LPPROCESS_INFORMATION   lpProcessInformation,
			PHANDLE                 phNewToken
	);


	INT NTAPI MultiByteToWideCharBypass(
		UINT    CodePage,
		DWORD   dwFlags,
		LPCCH   lpMultiByteStr,
		int     cbMultiByte,
		LPWSTR  lpWideCharStr,
		int     cchWideChar
	);

	HANDLE NTAPI CreateFileWBypass(
		LPCWSTR               lpFileName,
		DWORD                 dwDesiredAccess,
		DWORD                 dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		DWORD                 dwCreationDisposition,
		DWORD                 dwFlagsAndAttributes,
		HANDLE                hTemplateFile
	);

	BOOL NTAPI ReadFileBypass(
		HANDLE       hFile,
		LPVOID       lpBuffer,
		DWORD        nNumberOfBytesToRead,
		LPDWORD      lpNumberOfBytesRead,
		LPOVERLAPPED lpOverlapped
	);

	BOOL NTAPI IsDebuggerPresentBypass(
	);

	PVOID NTAPI LoadLibraryABypass(
		PCSTR LibFileName
	);

	PVOID NTAPI LoadLibraryWBypass(
		PCWSTR LibFileName
	);

	BOOL NTAPI IsDBCSLeadByteBypass(
		BYTE TestChar
	);

	VOID NTAPI UndocApiBypass1() {}
	VOID NTAPI UndocApiBypass2() {}
	VOID NTAPI UndocApiBypass3() {}
	VOID NTAPI UndocApiBypass4() {}
	VOID NTAPI UndocApiBypass5() {}
	VOID NTAPI UndocApiBypass6() {}
	VOID NTAPI UndocApiBypass7() {}
	VOID NTAPI UndocApiBypass8() {}
	
	// getters (Packer)
	PCWSTR NTAPI GetPackerBaseDir()             { return m_BaseDir.c_str(); };
	PCWSTR NTAPI GetPackerOriginalArchiveName() { return m_OriginalArchivePath.c_str(); };
	PCWSTR NTAPI GetPackerOutputArchiveName()   { return m_OutputArchivePath.c_str(); };
	PCWSTR NTAPI GetCurrentTempFileName()       { return m_CurrentTempFileName.c_str(); };
	HANDLE NTAPI GetCurrentTempHandle()         { return m_CurrentTempHandle.load(); };

	// setters (Packer)
	inline void SetPackerBaseDir(PCWSTR Dir)               { m_BaseDir = Dir; };
	inline void SetPackerOriginalArchiveName(PCWSTR Name)  { m_OriginalArchivePath = Name; };
	inline void SetPackerOutputArchiveName(PCWSTR Name)    { m_OutputArchivePath = Name; };
	inline void SetCurrentTempFileName(std::wstring& Name) { m_CurrentTempFileName = Name; };
	void NTAPI SetCurrentTempFileName(PCWSTR Name)        { m_CurrentTempFileName = Name; };
	void NTAPI SetCurrentTempHandle(HANDLE Handle)        { m_CurrentTempHandle = Handle; };

	// getters (Pointers, Decoders)
	BOOL                               NTAPI GetIsSpcialChunkEncrypted() { return m_IsSpcialChunkEncrypted; };
	Prototype::SpecialChunkDecoderFunc NTAPI GetSpecialChunkDecoder()    { return m_SpecialChunkDecoder; }
	Prototype::V2LinkFunc              NTAPI GetV2Link()                 { return m_V2Link; }
	PVOID                   NTAPI GetTVPCreateIStreamStub()   { return m_TVPCreateIStreamStub; };
	PVOID                   NTAPI GetTVPCreateIStreamP()      { return m_TVPCreateIStreamP; };
	PVOID                   NTAPI GetTVPCreateBStream()       { return m_TVPCreateBStream; };
	PVOID                   NTAPI GetAllocator()              { return m_Allocator; };
	PVOID                   NTAPI GetIStreamAdapterVtable()   { return m_IStreamAdapterVtable; };
	iTVPFunctionExporter*   NTAPI GetTVPFunctionExporter()    { return m_TVPFunctionExporter; };
	PVOID                   NTAPI GetXP3Filter()              { return m_XP3Filter; };
	PVOID                   NTAPI GetTVPGetFunctionExporter() { return m_TVPGetFunctionExporter; };

	// setters (Pointers, Decoders)
	inline void SetIsSpcialChunkEncrypted(BOOL Value)                 { m_IsSpcialChunkEncrypted = Value; };
	inline void SetSpecialChunkDecoder(Prototype::SpecialChunkDecoderFunc Value) { m_SpecialChunkDecoder = Value; }
	inline void SetTVPCreateIStreamStub(PVOID Value)                  { m_TVPCreateIStreamStub = Value; };
	inline void SetTVPCreateIStreamP(PVOID Value)                     { m_TVPCreateIStreamP = Value; };
	inline void SetTVPCreateBStream(PVOID Value)                      { m_TVPCreateBStream = Value; };
	inline void SetAllocator(PVOID Value)                             { m_Allocator = Value; };
	inline void SetIStreamAdapterVtable(PVOID Value)                  { m_IStreamAdapterVtable = Value; };
	inline void SetTVPFunctionExporter(iTVPFunctionExporter* Value)   { m_TVPFunctionExporter = Value; };
	inline void SetXP3Filter(tTVPXP3ArchiveExtractionFilter Value)    { m_XP3Filter = Value; };
	inline void SetTVPGetFunctionExporter(Prototype::TVPGetFunctionExporterFunc Value) { m_TVPGetFunctionExporter = Value; };
	
	// getters (Module)
	inline ULONG_PTR NTAPI GetFirstSectionAddress()   { return m_FirstSectionAddress; };
	inline ULONG_PTR NTAPI GetSizeOfImage()           { return m_SizeOfImage; };
	inline HMODULE   NTAPI GetSelfModule()            { return m_SelfModule; };
	inline HMODULE   NTAPI GetHostModule()            { return m_HostModule; };

	// setters (Module)
	inline void SetFirstSectionAddress(ULONG_PTR Value) { m_FirstSectionAddress = Value; };
	inline void SetSizeOfImage(ULONG_PTR Value)         { m_SizeOfImage = Value; };

	// Flags
	inline KrkrPngMode  NTAPI GetPngFlag()  { return m_PngFlag; };
	inline KrkrTlgMode  NTAPI GetTlgFlag()  { return m_TlgFlag; };
	inline KrkrTextMode NTAPI GetTextFlag() { return m_TextFlag; };
	inline KrkrAmvMode  NTAPI GetAmvFlag()  { return m_AmvFlag; };
	inline KrkrPbdMode  NTAPI GetPbdFlag()  { return m_PbdFlag; };
	inline KrkrTjs2Mode NTAPI GetTjsFlag()  { return m_TjsFlag; };
	inline KrkrPsbMode  NTAPI GetPsbFlag()  { return m_PsbFlag; };


	// Paths
	std::wstring           m_DllPath;
	std::wstring           m_ModulePath;
	std::wstring           m_DllDir;
	std::wstring           m_WorkerDir;

	PCWSTR NTAPI GetDllPath()    { return m_DllPath.c_str(); };
	PCWSTR NTAPI GetModulePath() { return m_ModulePath.c_str(); };
	PCWSTR NTAPI GetDllDir()     { return m_DllDir.c_str(); };
	PCWSTR NTAPI GetWorkerDir()  { return m_WorkerDir.c_str(); };

	NTSTATUS SetHwBreakPoint(PVOID Address, SIZE_T Size, HardwareBreakpoint::Condition AccessStatus, INT& Index);
	NTSTATUS RemoveHwBreakPoint(INT Index);
	NTSTATUS RemoveAllHwBreakPoint(PCONTEXT Context);
	NTSTATUS GetBusyHwBreakPoint(_Out_ HwBreakPointStatus& Status);

	// Special
	std::unordered_map<ULONG64, MemEntry> m_SpecialChunkMap;
	std::unordered_map<ULONG, MemEntry>   m_SpecialChunkMapBySize;
	SectionLock                           m_SpecialLock;

	SectionLock                           m_CodeAnaLock;
	SectionLock                           m_AddFileEntryLock;

	ULONG                                 m_EmotePrivateKey = 0;
	std::atomic<BOOL>                     m_EmoteKeyInitialized = FALSE;


private:
	NTSTATUS AddToRiddleJokerSpecialEntry(LPCWSTR FileName, PBYTE Decompress, ULONG Size, NtFileDisk& File, DWORD M2Magic);
	NTSTATUS AddToRiddleJokerSpecialEntryV2(LPCWSTR FileName, PBYTE Decompress, ULONG Size, NtFileDisk& File, DWORD M2Magic);
	NTSTATUS ListAllXp3Files(std::vector<std::wstring>& Xp3List);
	NTSTATUS ReadRiddleJokerSpecial();
	NTSTATUS GetRiddleJokerSpecialEntry(PCWSTR lpFileName);
	NTSTATUS IsEncryptedSenrenBanka(PBYTE Decompress, ULONG Size, NtFileDisk& File, BOOL& IsEncrypted, DWORD M2Magic);
	NTSTATUS IsEncryptedSenrenBankaV2(PBYTE Decompress, ULONG Size, NtFileDisk& File, BOOL& IsEncrypted, DWORD M2Magic);
	NTSTATUS IsEncryptedSenrenBankaV3(PBYTE Decompress, ULONG Size, NtFileDisk& File, BOOL& IsEncrypted, DWORD M2Magic);
	BOOL     AddFileEntry(PCWSTR FileName, ULONG Length);

public:
	BOOL     FindCodeSlow(PCCHAR Start, ULONG Size, PCCHAR Pattern, ULONG PatternLen);
	NTSTATUS InitWindow();
	NTSTATUS InitWindowLocalMode();
	NTSTATUS InitWindowRemoteMode();
	NTSTATUS InitWindowMixedMode();
	NTSTATUS InitHookWithDll(LPCWSTR ModuleName, PVOID ImageBase);
	NTSTATUS InitHookWithExe();

	ULONG GetClientPort() { return m_ClientPort.load(); }
	VOID SetClientPort(ULONG Port) { m_ClientPort = Port; }

	//
	// getters (Plsugins)
	//

	ULONG    NTAPI GetCustomFlag(HANDLE Handle) { return 0; };

	IPlugin* NTAPI GetDefaultPlugin() { return m_FilePlugin; };
	IPlugin* NTAPI GetPluginWithExtName(PCWSTR ExtName);

	BOOL NTAPI EnumXp3Plugin(void* Param, ENUMXP3PLUGINPROC Proc);
	BOOL NTAPI EnumXp3Plugin2(void* Param, ENUMXP3PLUGINPROC2 Proc);

	void NTAPI UndocApi1() {};
	void NTAPI UndocApi2() {};
	void NTAPI UndocApi3() {};
	void NTAPI UndocApi4() {};
	void NTAPI UndocApi5() {};
	void NTAPI UndocApi6() {};
	void NTAPI UndocApi7() {};
	void NTAPI UndocApi8() {};

	NTSTATUS NTAPI GetEmotePrivateKey(PULONG EmoteKey);

	tTJSBinaryStream* NTAPI HostTVPCreateStream(PCWSTR FilePath);
	IStream*          NTAPI HostConvertBStreamToIStream(tTJSBinaryStream* BStream);


	NTSTATUS HookGetProcAddress();
	NTSTATUS UnHookGetProcAddress();
	NTSTATUS HookCreateProcessInternalW();
	NTSTATUS UnHookCreateProcessInternalW();
	NTSTATUS HookMultiByteToWideChar();
	NTSTATUS UnHookMultiByteToWideChar();
	NTSTATUS HookCreateFileW();
	NTSTATUS UnHookCreateFileW();
	NTSTATUS HookZLIBUncompress();
	NTSTATUS HookReadFile();
	NTSTATUS UnHookReadFile();
	NTSTATUS HookIsDebuggerPresent();
	NTSTATUS UnHookIsDebuggerPresent();
	NTSTATUS HookLoadLibraryA();
	NTSTATUS UnHookLoadLibraryA();
	NTSTATUS HookLoadLibraryW();
	NTSTATUS UnHookLoadLibraryW();
	NTSTATUS HookTVPGetFunctionExporter();
	NTSTATUS UnHookTVPGetFunctionExporter();
	NTSTATUS HookTVPSetXP3ArchiveExtractionFilter();
	NTSTATUS UnHookTVPSetXP3ArchiveExtractionFilter();
	NTSTATUS HookV2Link(PVOID Module);
	NTSTATUS UnHookV2Link();
	NTSTATUS HookIsDBCSLeadByte();
	NTSTATUS UnHookIsDBCSLeadByte();

	// Status
	ForceInline BOOL IsMultiByteToWideCharHooked() { return m_IsMultiByteToWideCharHooked.load(); }
	ForceInline BOOL IsGetProcAddressHooked() { return m_IsGetProcAddressHooked.load(); }
	ForceInline BOOL IsCreateProcessInternalWHooked() { return m_IsCreateProcessInternalWHooked.load(); }
	ForceInline BOOL IsCreateFileWHooked() { return m_IsCreateFileWHooked.load(); }
	ForceInline BOOL IsReadFileHooked() { return m_IsReadFileHooked.load(); }
	ForceInline BOOL IsIsDebuggerPresentHooked() { return m_IsIsDebuggerPresentHooked.load(); }
	ForceInline BOOL IsTVPGetFunctionExporterHooked() { return m_IsTVPGetFunctionExporterHooked.load(); }
	ForceInline BOOL IsLoadLibraryAHooked() { return m_IsLoadLibraryAHooked.load(); }
	ForceInline BOOL IsLoadLibraryWHooked() { return m_IsLoadLibraryWHooked.load(); }
	ForceInline BOOL IsV2LinkHooked() { return m_IsV2LinkHooked.load(); }
	ForceInline BOOL IsTVPSetXP3ArchiveExtractionFilterHooked() { return m_IsTVPSetXP3ArchiveExtractionFilterHooked.load(); }
	ForceInline BOOL IsDBCSLeadByteHooked() { return m_IsDBCSLeadByteHooked.load(); }


	PVOID                                 m_ExceptionHandler = NULL;
	SectionLock                           m_ReadFileLocker;
	ULONG                                 m_RetAddr = 0;
	BOOL                                  m_SetBpOnce = FALSE;
	std::atomic<BOOL>              m_BreakOnce = FALSE;

	API_POINTER(GetProcAddress)            m_GetProcAddress = nullptr;
	API_POINTER(CreateProcessInternalW)    m_CreateProcessInternalW = nullptr;
	API_POINTER(MultiByteToWideChar)       m_MultiByteToWideChar = nullptr;
	API_POINTER(CreateFileW)               m_CreateFileW = nullptr;
	API_POINTER(ReadFile)                  m_ReadFile = nullptr;
	API_POINTER(IsDebuggerPresent)         m_IsDebuggerPresent = nullptr;
	Prototype::SetXP3FilterFunc            m_TVPSetXP3ArchiveExtractionFilter = nullptr;
	Prototype::V2LinkFunc                  m_V2Link = nullptr;
	Prototype::TVPGetFunctionExporterFunc m_TVPGetFunctionExporter = nullptr;
	API_POINTER(IsDBCSLeadByte)            m_IsDBCSLeadByte = nullptr;
	
private:
	NTSTATUS InitializeRand();
	NTSTATUS InitializeGlobalFlags();
	NTSTATUS InitializeAndDetectBuiltinCxdec2();
	NTSTATUS InitializePlugins();
	NTSTATUS InitializePluginsPrivateWithCustom();
	NTSTATUS InitializeCommandUtils();
	NTSTATUS InitializePrivatePointersFromFile();
	NTSTATUS InitializeDarkMode();
	
	NTSTATUS InitExporterPrivate(iTVPFunctionExporter *Exporter);
	NTSTATUS InitializeTVPCreateStreamCallKrkrZ();
	NTSTATUS InitializeTVPCreateStreamCallKrkr2();

	NTSTATUS InitializeWithRpcMode();
	NTSTATUS InitializeWithLocalMode();
	NTSTATUS InitializeWithMixedMode();

	std::string          m_SessionKey;
	std::atomic<ULONG>   m_ClientPort = 0;

	// Hook Status
	std::atomic<BOOL> m_IsMultiByteToWideCharHooked = FALSE;
	std::atomic<BOOL> m_IsGetProcAddressHooked = FALSE;
	std::atomic<BOOL> m_IsCreateProcessInternalWHooked = FALSE;
	std::atomic<BOOL> m_IsCreateFileWHooked = FALSE;
	std::atomic<BOOL> m_IsReadFileHooked = FALSE;
	std::atomic<BOOL> m_IsIsDebuggerPresentHooked = FALSE;
	std::atomic<BOOL> m_IsTVPGetFunctionExporterHooked = FALSE;
	std::atomic<BOOL> m_IsLoadLibraryAHooked = FALSE;
	std::atomic<BOOL> m_IsLoadLibraryWHooked = FALSE;
	std::atomic<BOOL> m_IsV2LinkHooked = FALSE;
	std::atomic<BOOL> m_IsTVPSetXP3ArchiveExtractionFilterHooked = FALSE;
	std::atomic<BOOL> m_IsDBCSLeadByteHooked = FALSE;

	HardwareBreakpoint                    m_BreakPoint[4];
	HwBreakPointStatus                    m_BreakPointIndexSet;
	ULONG_PTR                             m_BreakPointAddresses[4];


	//
	// Plugins
	//

	std::unordered_map<std::wstring, IPlugin*>               m_RegisteredPlugins;
	IPlugin*                                                 m_FilePlugin = NULL;
	std::vector<Xp3ArchivePlugin*>                           m_Xp3PluginsVersion1;
	Trie<char, std::shared_ptr<CommandPlugin>>               m_CommandBuilder;

};


