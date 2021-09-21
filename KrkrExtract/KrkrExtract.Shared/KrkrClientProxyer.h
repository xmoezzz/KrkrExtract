#pragma once

#include <my.h>
#include <Stubs.h>
#include <Xp3Access.h>
#include "Prototype.h"
#include "tp_stub.h"

class KrkrClientProxyer;

class IPlugin : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Bind(
		_In_ KrkrClientProxyer* Proxyer,
		_In_ HANDLE Handle,
		_Out_ PCWSTR** Extensions,
		_Out_ ULONG* Count
	) = 0;

	virtual HRESULT STDMETHODCALLTYPE Unpack(
		_In_ PCWSTR FilePath,
		_In_ IStream* Stream
	) = 0;

	virtual HANDLE STDMETHODCALLTYPE GetHandle(
	) = 0;
};


class KrkrClientProxyer
{
public:

	virtual PVOID                             NTAPI GetPrivatePointer() = 0;

	virtual PCWSTR                            NTAPI GetDllPath() = 0;
	virtual PCWSTR                            NTAPI GetModulePath() = 0;
	virtual PCWSTR                            NTAPI GetDllDir() = 0;
	virtual PCWSTR                            NTAPI GetWorkerDir() = 0;

	virtual PCWSTR                            NTAPI GetPackerBaseDir() = 0;
	virtual PCWSTR                            NTAPI GetPackerOriginalArchiveName() = 0;
	virtual PCWSTR                            NTAPI GetPackerOutputArchiveName() = 0;
	virtual PCWSTR                            NTAPI GetCurrentTempFileName() = 0;
	virtual HANDLE                            NTAPI GetCurrentTempHandle() = 0;


	virtual BOOL                               NTAPI GetIsSpcialChunkEncrypted() = 0;
	virtual Prototype::SpecialChunkDecoderFunc NTAPI GetSpecialChunkDecoder() = 0;
	virtual Prototype::V2LinkFunc              NTAPI GetV2Link() = 0;
	virtual PVOID                              NTAPI GetTVPCreateIStreamStub() = 0;
	virtual PVOID                              NTAPI GetTVPCreateIStreamP() = 0;
	virtual PVOID                              NTAPI GetTVPCreateBStream() = 0;
	virtual PVOID                              NTAPI GetAllocator() = 0;
	virtual PVOID                              NTAPI GetIStreamAdapterVtable() = 0;
	virtual iTVPFunctionExporter*              NTAPI GetTVPFunctionExporter() = 0;
	virtual PVOID                              NTAPI GetXP3Filter() = 0;

	virtual ULONG_PTR                          NTAPI GetFirstSectionAddress() = 0;
	virtual ULONG_PTR                          NTAPI GetSizeOfImage() = 0;
	virtual HMODULE                            NTAPI GetSelfModule() = 0;
	virtual HMODULE                            NTAPI GetHostModule() = 0;

	virtual void                               NTAPI SetCurrentTempFileName(PCWSTR Name) = 0;
	virtual void                               NTAPI SetCurrentTempHandle(HANDLE Handle) = 0;


	virtual KrkrPngMode                        NTAPI GetPngFlag()  = 0;
	virtual KrkrTlgMode                        NTAPI GetTlgFlag()  = 0;
	virtual KrkrTextMode                       NTAPI GetTextFlag() = 0;
	virtual KrkrAmvMode                        NTAPI GetAmvFlag()  = 0;
	virtual KrkrPbdMode                        NTAPI GetPbdFlag()  = 0;
	virtual KrkrTjs2Mode                       NTAPI GetTjsFlag()  = 0;
	virtual KrkrPsbMode                        NTAPI GetPsbFlag()  = 0;
	virtual ULONG                              NTAPI GetCustomFlag(HANDLE Handle) = 0;
	virtual IPlugin*                           NTAPI GetDefaultPlugin() = 0;
	virtual IPlugin*                           NTAPI GetPluginWithExtName(PCWSTR ExtName) = 0;
	
	using ENUMXP3PLUGINPROC  = BOOL(NTAPI*)(void* Param, Xp3ArchivePlugin*  Plugin);
	using ENUMXP3PLUGINPROC2 = BOOL(NTAPI*)(void* Param, Xp3ArchivePlugin2* Plugin);
	virtual BOOL                               NTAPI EnumXp3Plugin(void* Param,  ENUMXP3PLUGINPROC Proc) = 0;
	virtual BOOL                               NTAPI EnumXp3Plugin2(void* Param, ENUMXP3PLUGINPROC2 Proc) = 0;

	virtual void NTAPI UndocApi1() = 0;
	virtual void NTAPI UndocApi2() = 0;
	virtual void NTAPI UndocApi3() = 0;
	virtual void NTAPI UndocApi4() = 0;
	virtual void NTAPI UndocApi5() = 0;
	virtual void NTAPI UndocApi6() = 0;
	virtual void NTAPI UndocApi7() = 0;
	virtual void NTAPI UndocApi8() = 0;

	virtual NTSTATUS NTAPI GetEmotePrivateKey(ULONG* EmoteKey) = 0;

	virtual tTJSBinaryStream* NTAPI HostTVPCreateStream(PCWSTR FilePath) = 0;
	virtual IStream*          NTAPI HostConvertBStreamToIStream(tTJSBinaryStream* BStream) = 0;

	virtual NTSTATUS NTAPI ParseUtilCommand(PWSTR Command) = 0;
	

	virtual PVOID NTAPI GetProcAddressBypass(
		PVOID Module,
		PCSTR RoutineName
	) = 0;

	virtual BOOL NTAPI CreateProcessBypass(
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
	) = 0;

	virtual INT NTAPI MultiByteToWideCharBypass(
		UINT    CodePage,
		DWORD   dwFlags,
		LPCCH   lpMultiByteStr,
		int     cbMultiByte,
		LPWSTR  lpWideCharStr,
		int     cchWideChar
	) = 0;

	virtual HANDLE NTAPI CreateFileWBypass(
		LPCWSTR               lpFileName,
		DWORD                 dwDesiredAccess,
		DWORD                 dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		DWORD                 dwCreationDisposition,
		DWORD                 dwFlagsAndAttributes,
		HANDLE                hTemplateFile
	) = 0;

	virtual BOOL NTAPI ReadFileBypass(
		HANDLE       hFile,
		LPVOID       lpBuffer,
		DWORD        nNumberOfBytesToRead,
		LPDWORD      lpNumberOfBytesRead,
		LPOVERLAPPED lpOverlapped
	) = 0;

	virtual BOOL NTAPI IsDebuggerPresentBypass(
	) = 0;

	virtual PVOID NTAPI LoadLibraryABypass(
		PCSTR LibFileName
	) = 0;

	virtual PVOID NTAPI LoadLibraryWBypass(
		PCWSTR LibFileName
	) = 0;

	virtual BOOL NTAPI IsDBCSLeadByteBypass(
		BYTE TestChar
	) = 0;

	virtual VOID NTAPI UndocApiBypass1() = 0;
	virtual VOID NTAPI UndocApiBypass2() = 0;
	virtual VOID NTAPI UndocApiBypass3() = 0;
	virtual VOID NTAPI UndocApiBypass4() = 0;
	virtual VOID NTAPI UndocApiBypass5() = 0;
	virtual VOID NTAPI UndocApiBypass6() = 0;
	virtual VOID NTAPI UndocApiBypass7() = 0;
	virtual VOID NTAPI UndocApiBypass8() = 0;

	virtual BOOL NTAPI TellServerProgressBar(PCWSTR TaskName, ULONGLONG Current, ULONGLONG Total) = 0;
	virtual BOOL NTAPI TellServerLogOutput(LogLevel Level, PCWSTR FormatString, ...) = 0;
	virtual BOOL NTAPI TellServerCommandResultOutput(CommandStatus Status, PCWSTR FormatString, ...) = 0;
	virtual BOOL NTAPI TellServerUIReady(ULONG ClientPort, PCSTR SessionKey, ULONG Extra) = 0;
	virtual BOOL NTAPI TellServerMessageBox(PCWSTR Description, ULONG Flags, BOOL Locked) = 0;
	virtual BOOL NTAPI TellServerTaskStartAndDisableUI() = 0;
	virtual BOOL NTAPI TellServerTaskEndAndEnableUI(BOOL TaskCompleteStatus, PCWSTR Description) = 0;
	virtual BOOL NTAPI TellServerUIHeartbeatPackage() = 0;
	virtual BOOL NTAPI TellServerExitFromRemoteProcess() = 0;
};