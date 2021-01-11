#include <my.h>
#include "KrkrExtract.h"
#include "resource.h"
#include "Decoder.h"
#include "SectionProtector.h"
#include "SafeMemory.h"
#include "CoreTask.h"
#include "Helper.h"
#include <Shlobj.h>
#include "DebuggerHandler.h"
#include "SectionProtector.h"
#include "mt64.h"
#include "xxhash.h"
#include "WellknownPlugin.h"
#include "KrkrHookExporter.h"
#include "WinReg.h"
#include <stdint.h>
#include <json/json.h>
#include <zlib.h>


struct KrkrExportedData
{
	ULONG64 Hash;
	ULONG64 TVPCreateIStreamStub;
	ULONG64 TVPCreateIStreamP;
	ULONG64 TVPCreateBStream;
	ULONG64 Allocator;
	ULONG64 IStreamAdapterVtable;
	ULONG64 Exporter;
};

NTSTATUS KrkrExtractCore::InitExporterPrivate(iTVPFunctionExporter *Exporter)
{
	BOOL                  Success;
	ULONG64               Crc;
	PVOID                 EncodedExporter;
	PVOID                 CreateStreamAddress;
	PBYTE                 PointRva;
	PBYTE                 ExecuteHandle;
	NtFileDisk            File;
	NTSTATUS              Status;
	Json::Value           Json;
	KrkrExportedData      ExportedData;
	WCHAR                 FullPath[MAX_NTPATH];
	WCHAR                 ExePath[MAX_NTPATH];

	static BYTE JsonMagic[4] = { 0x44, 0x33, 0x22, 0x11 };

	LOOP_ONCE
	{
		Success = TVPInitImportStub(Exporter);
		if (!Success) {
			PrintConsoleW(L"KrkrExtractCore::InitExporterPrivate : TVPInitImportStub failed\n");
			break;
		}

		if (m_Allocator            == nullptr ||
			m_IStreamAdapterVtable == nullptr ||
			m_TVPCreateBStream     == nullptr ||
			m_TVPCreateIStreamP    == nullptr ||
			m_TVPCreateIStreamStub == nullptr)
		{
			PrintConsoleW(L"KrkrExtractCore::InitExporterPrivate : Update private pointers\n");

			Status = InitializePrivatePointers();
			if (NT_FAILED(Status)) {
				PrintConsoleW(L"KrkrExtractCore::InitExporterPrivate : InitializePrivatePointers failed, %08x\n", Status);
				break;
			}
		}


		//
		// TODO:
		// dynamic generated code?
		//

		RtlZeroMemory(&ExportedData, sizeof(ExportedData));
		ExportedData.Allocator            = m_Allocator ?              (ULONG_PTR)m_Allocator            - (ULONG_PTR)m_SelfModule : 0;
		ExportedData.IStreamAdapterVtable = m_IStreamAdapterVtable ?   (ULONG_PTR)m_IStreamAdapterVtable - (ULONG_PTR)m_SelfModule : 0;
		ExportedData.TVPCreateBStream     = m_TVPCreateBStream ?       (ULONG_PTR)m_TVPCreateBStream     - (ULONG_PTR)m_SelfModule : 0;
		ExportedData.TVPCreateIStreamP    = m_TVPCreateIStreamP ?      (ULONG_PTR)m_TVPCreateIStreamP    - (ULONG_PTR)m_SelfModule : 0;
		ExportedData.TVPCreateIStreamStub = m_TVPCreateIStreamStub ?   (ULONG_PTR)m_TVPCreateIStreamStub - (ULONG_PTR)m_SelfModule : 0;
		ExportedData.Exporter             = m_TVPGetFunctionExporter ? (ULONG_PTR)m_TVPGetFunctionExporter - (ULONG_PTR)m_SelfModule : 0;

		ExportedData.Hash = XXH64(&ExportedData, sizeof(ExportedData), 0);

		
		//
		// save data
		//
#if 0
		Nt_GetModuleFileName(m_HostModule, ExePath, countof(ExePath));
		wnsprintfW(FullPath, countof(FullPath), L"%s.krconfig", ExePath);
		Status = File.Create(FullPath);
		if (NT_SUCCESS(Status))
		{
			Json["Hash"]                 = (uint64_t)ExportedData.Hash;
			Json["Allocator"]            = (uint64_t)ExportedData.Allocator;
			Json["IStreamAdapterVtable"] = (uint64_t)ExportedData.IStreamAdapterVtable;
			Json["TVPCreateBStream"]     = (uint64_t)ExportedData.TVPCreateBStream;
			Json["TVPCreateIStreamP"]    = (uint64_t)ExportedData.TVPCreateIStreamP;
			Json["TVPCreateIStreamStub"] = (uint64_t)ExportedData.TVPCreateIStreamStub;
			Json["exporter"]             = (uint64_t)ExportedData.Exporter;

			auto&& JsonData = Json.toStyledString();
			for (size_t i = 0; i < JsonData.length(); i++) {
				JsonData[i] ^= 0x6F;
			}

			File.Write(JsonMagic, sizeof(JsonMagic));
			File.Write((PVOID)JsonData.c_str(), JsonData.length());
			File.Close();
		}

#endif
	}

	return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


NTSTATUS KrkrExtractCore::InitExporterWithExe(iTVPFunctionExporter *Exporter)
{
	NTSTATUS     Status;

	if (GetTVPExporterInitializationType() != TVPExporterInitializationType::NotInited) {
		PrintConsoleW(L"KrkrExtractCore::InitExporterWithExe : already initialized\n");
		return STATUS_SUCCESS;
	}
	
	LOOP_ONCE
	{
		Status  = STATUS_UNSUCCESSFUL;
		if (!Exporter)
			break;

		PrintConsoleW(L"[exporter:exe] %p\n", Exporter);
		
		Status = InitExporterPrivate(Exporter);
		if (NT_FAILED(Status))
			break;

		SetTVPExporterInitializationType(TVPExporterInitializationType::ByCallingTVPGetExporter);
		Status = STATUS_SUCCESS;
	}
	return Status;
}


NTSTATUS KrkrExtractCore::InitExporterWithDll(iTVPFunctionExporter *Exporter)
{
	NTSTATUS     Status;

	if (GetTVPExporterInitializationType() != TVPExporterInitializationType::NotInited) {
		PrintConsoleW(L"KrkrExtractCore::InitExporterWithDll : already initialized\n");
		return STATUS_SUCCESS;
	}

	LOOP_ONCE
	{
		Status = STATUS_UNSUCCESSFUL;
		if (!Exporter)
			break;

		PrintConsoleW(L"[exporter:dll] %p\n", Exporter);

		Status = InitExporterPrivate(Exporter);
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"KrkrExtractCore::InitExporterWithDll : InitExporterPrivate failed£¬ %08x\n", Status);
			break;
		}

		PrintConsoleW(L"KrkrExtractCore::InitExporterWithDll...  ok\n");
		SetTVPExporterInitializationType(TVPExporterInitializationType::ByCallingV2Link);
		Status = STATUS_SUCCESS;
	}
	return Status;
}


KrkrExtractCore::KrkrExtractCore()
{
}


KrkrExtractCore::~KrkrExtractCore()
{
	m_SpecialChunkMap.clear();
}


NTSTATUS KrkrExtractCore::Initialize(HMODULE DllModule)
{
	NTSTATUS           Status;
	DWORD              Size;
	PBYTE              Buffer;
	DWORD              FakeHandle;
	WCHAR              FileName[MAX_PATH];
	UINT               Length;
	WCHAR              ModeString[0x200];
	WCHAR              RunModeString[0x200];

	static WCHAR PatternZ[] = L"TVP(KIRIKIRI) Z core / Scripting Platform for Win32";
	static WCHAR Pattern2[] = L"TVP(KIRIKIRI) 2 core / Scripting Platform for Win32";

	//
	// Initialize private libs
	//
	ml::MlInitialize();

	//
	// Initialize module handle
	//
	m_SelfModule = DllModule;
	m_HostModule = (HMODULE)Nt_GetModuleHandle(NULL);

	//
	//Initialize random generator
	//
	InitializeGlobalFlags();
	InitializeRand();
	InitializePlugins();
	InitializeCommandUtils();
	
	RtlZeroMemory(FileName, sizeof(FileName));
	GetModuleFileNameW(m_HostModule, FileName, countof(FileName));
	m_ModulePath = FileName;

	//
	// Is kirikiriZ?
	//

	LOOP_ONCE
	{
		FakeHandle = 0;
		Size   = GetFileVersionInfoSizeW(FileName, &FakeHandle);
		if (Size == 0)
			break;

		auto Buffer = AllocateMemorySafeP<BYTE>(Size);
		if (!GetFileVersionInfoW(FileName, FakeHandle, Size, Buffer.get()))
			break;

		if (KMP(Buffer.get(), Size, PatternZ, CONST_STRLEN(PatternZ)))
		{
			m_ModuleType = KrkrVersion::KRKRZ;
			break;
		}

		if (KMP(Buffer.get(), Size, Pattern2, CONST_STRLEN(Pattern2)))
		{
			m_ModuleType = KrkrVersion::KRKR2;
			break;
		}
	};

	RtlZeroMemory(FileName, sizeof(FileName));
	GetModuleFileNameW(m_SelfModule, FileName, countof(FileName));
	m_DllPath = FileName;

	RtlZeroMemory(FileName, sizeof(FileName));
	Nt_GetExeDirectory(FileName, countof(FileName));
	m_WorkerDir = FileName;

	//
	// Initialize modes
	//


	RtlZeroMemory(ModeString, sizeof(ModeString));
	Size = GetEnvironmentVariableW(L"KrkrMode", ModeString, countof(ModeString));
	RtlZeroMemory(RunModeString, sizeof(RunModeString));
	Size = GetEnvironmentVariableW(L"KrkrRunMode", RunModeString, countof(RunModeString));

	switch ((USHORT)ModeString[0])
	{
	case L'B':
		m_Mode = KrkrMode::BASIC_LOCK;
		PrintConsoleW(L"mode : basic lockdown mode\n");
		break;

	case L'A':
		m_Mode = KrkrMode::ADV_LOCK;
		PrintConsoleW(L"mode : advance lockdown mode\n");
		break;

	case L'H':
		m_Mode = KrkrMode::HYPERVISOR;
		PrintConsoleW(L"mode : hypervisor mode\n");
		break;

	default:
		m_Mode = KrkrMode::NORMAL;
		PrintConsoleW(L"mode : normal mode\n");
		break;
	}

	//
	// Create Hook instance
	// bad design...
	//

	m_HookEngine = new (std::nothrow) KrkrHook(
		this,
		m_Mode == KrkrMode::HYPERVISOR ? HookMode::HOOK_EPT : HookMode::HOOK_NATIVE
	);

	if (m_HookEngine == nullptr)
		return STATUS_NO_MEMORY;


	switch ((USHORT)RunModeString[0])
	{
	case L'R':
		m_RunMode = KrkrRunMode::REMOTE_MODE;

		//
		// Connect to server
		//
		Status = InitializeWithRpcMode();
		PrintConsoleW(L"Rpc Mode\n");
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"KrkrExtractCore::Initialize : InitializeWithRpcMode failed, %08x\n", Status);
			return Status;
		}
		break;

	default:
		m_RunMode = KrkrRunMode::LOCAL_MODE;

		//
		// Load dll and create stubs
		//

		Status = InitializeWithLocalMode();
		PrintConsoleW(L"Local Mode\n");
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"KrkrExtractCore::Initialize : InitializeWithLocalMode failed, %08x\n", Status);
			return Status;
		}
		break;
	}


	//
	// ok, now we can initialize other stuffs
	//

	Status = InitializeAndDetectBuiltinCxdec2();
	if (NT_FAILED(Status)) {
		PrintConsoleW(L"InitializeAndDetectBuiltinCxdec2 failed\n");
	}

	Status = InitializeHook();
	if (NT_FAILED(Status)) {
		PrintConsoleW(L"InitializeHook failed\n");
	}

	return STATUS_SUCCESS;
}


NTSTATUS KrkrExtractCore::InitializeDarkMode()
{
	return STATUS_SUCCESS;
}

INT NTAPI InitializePluginsEpFilter(ULONG Code, EXCEPTION_POINTERS* Ep)
{
	switch ((NTSTATUS)Code)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		PrintConsoleW(L"InitializePlugin : EXCEPTION_ACCESS_VIOLATION\n");
		return EXCEPTION_EXECUTE_HANDLER;
	
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

struct PluginDescription
{
	PluginDescription(IPlugin* (*PluginCreator)(), HANDLE PluginHandle, PCWSTR PluginDescription)
	{
		Creator     = PluginCreator;
		Handle      = PluginHandle;
		Description = PluginDescription;
	}

	IPlugin* (*Creator)();
	HANDLE   Handle;
	PCWSTR   Description;
};


NTSTATUS KrkrExtractCore::InitializePluginsPrivateWithCustom()
{
	SEH_TRY
	{
		//
		// TODO
		//
	}
	SEH_EXCEPT(InitializePluginsEpFilter(GetExceptionCode(), GetExceptionInformation()))
	{
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

NTSTATUS KrkrExtractCore::InitializePlugins()
{
	HRESULT  Success;
	ULONG    Count;
	PCWSTR*  Extensions;
	IPlugin* Plugin;

	m_FilePlugin = CFilePlugin::CreateInstance();
	if (!m_FilePlugin) {
		PrintConsoleW(L"CFilePlugin::CreateInstance() failed\n");
		return STATUS_NO_MEMORY;
	}

	auto Plugins = 
	{
		PluginDescription(CTextPlugin::CreateInstance, HANDLE_EXT_TEXT, L"CTextPlugin::CreateInstance"),
		PluginDescription(CAmvPlugin::CreateInstance,  HANDLE_EXT_AMV,  L"CAmvPlugin::CreateInstance"),
		PluginDescription(CPbdPlugin::CreateInstance,  HANDLE_EXT_PBD,  L"CPbdPlugin::CreateInstance"),
		PluginDescription(CTlgPlugin::CreateInstance,  HANDLE_EXT_TLG,  L"CTlgPlugin::CreateInstance"),
		PluginDescription(CPngPlugin::CreateInstance,  HANDLE_EXT_PNG,  L"CPngPlugin::CreateInstance"),
		PluginDescription(CPsbPlugin::CreateInstance,  HANDLE_EXT_PSB,  L"CPsbPlugin::CreateInstance")
	};

	for (auto& PluginDesc : Plugins)
	{
		//
		// be careful with 3-rd party plugins
		//

		Plugin = PluginDesc.Creator();
		if (!Plugin) {
			PrintConsoleW(L"%s() failed\n", PluginDesc.Description);
			break;
		}

		Count = 0;
		Extensions = NULL;
		Success = Plugin->Bind(this, PluginDesc.Handle, &Extensions, &Count);
		for (ULONG i = 0; i < Count; i++) {
			m_RegisteredPlugins[Extensions[i]] = Plugin;
		}
	}

	return InitializePluginsPrivateWithCustom();
}


IPlugin* KrkrExtractCore::GetPluginWithExtName(PCWSTR ExtName)
{
	auto Iterator = m_RegisteredPlugins.find(ExtName);
	if (Iterator != m_RegisteredPlugins.end()) {
		return Iterator->second;
	}

	return nullptr;
}


NTSTATUS KrkrExtractCore::InitializeAndDetectBuiltinCxdec2()
{
	NTSTATUS                  Status;
	std::vector<std::wstring> ArchiveList;
	Xp3WalkerProxy            Proxyer;
	DWORD                     M2ChunkMagic;
	DWORD                     NextIncreasement;

	Status = ReadRiddleJokerSpecial();
	if (NT_FAILED(Status)) {
		PrintConsoleW(L"ReadRiddleJokerSpecial failed\n");
	}

	Status = ListAllXp3Files(ArchiveList);
	if (NT_FAILED(Status))
		return Status;

	NextIncreasement = 1;
	for (ULONG RetryTimes = 0; RetryTimes < min(5, ArchiveList.size()); RetryTimes += NextIncreasement)
	{
		NextIncreasement = 1;

		PrintConsoleW(L"KrkrExtractCore::InitializeAndDetectBuiltinCxdec2 : now tring %s\n",
			ArchiveList[RetryTimes].c_str()
		);

		if (m_HookEngine->IsReadFileHooked())
			break;

		Status = WalkXp3ArchiveIndex(
			this,
			ArchiveList[RetryTimes].c_str(),
			Proxyer,
			M2ChunkMagic,
			WalkXp3IndexCallbackDummy,
			WalkXp3IndexCallbackDummy,
			WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, M2Magic, Xp3Proxyer)
			{
				NTSTATUS NtStatus;
				BOOL     IsEncrypted;

				PrintConsoleW(L"Pre-init : found Special chunk.\n");
				IsEncrypted = FALSE;

				LOOP_ONCE
				{
					NtStatus = IsEncryptedSenrenBanka(Buffer, (ULONG)Size, File, IsEncrypted, M2Magic);
					if (NT_FAILED(NtStatus))
					{
						PrintConsoleW(L"IsEncryptedSenrenBanka failed\n");
						break;
					}

					if (!IsEncrypted)
						break;

					m_IsSpcialChunkEncrypted = TRUE;
					NtStatus = m_HookEngine->HookReadFile();
					if (NT_FAILED(NtStatus)) {
						PrintConsoleW(L"Failed to hook ReadFile\n");
					}
				}
				return NtStatus;
			},
			WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, M2Magic, Xp3Proxyer)
			{
				NTSTATUS NtStatus;
				BOOL     IsEncrypted;

				PrintConsoleW(L"Pre-init : found Special chunk. (v2)\n");
				IsEncrypted = FALSE;

				LOOP_ONCE
				{
					NtStatus = IsEncryptedSenrenBankaV2(Buffer, (ULONG)Size, File, IsEncrypted, M2Magic);
					if (NT_FAILED(NtStatus))
					{
						PrintConsoleW(L"IsEncryptedSenrenBankaV2 failed\n");
						break;
					}

					if (!IsEncrypted)
						break;

					m_IsSpcialChunkEncrypted = TRUE;
					NtStatus = m_HookEngine->HookReadFile();
					if (NT_FAILED(NtStatus)) {
						PrintConsoleW(L"Failed to hook ReadFile\n");
					}
				}
				return NtStatus;
			},
			WalkXp3IndexCallbackDummy,
			WalkXp3IndexCallbackDummy
		);

		if (NT_FAILED(Status)) {
			auto&& ExtName = GetExtensionUpper(ArchiveList[RetryTimes]);
			if (ExtName == L"EXE") {
				NextIncreasement = 0;
			}
		}

		if (NT_FAILED(Status) && NextIncreasement == 1) {
			PrintConsoleW(L"InitializeAndDetectBuiltinCxdec2 failed, %08x, Retry time(s) : %d\n", Status, RetryTimes);
		}

		if (NT_SUCCESS(Status))
			break;
	}

	return Status;
}


DWORD NTAPI AlpcClientThread(PVOID Param)
{
	NTSTATUS         Status;
	KrkrExtractCore* Instance;
	PPORT_MESSAGE    ReceivedMessage;
	SIZE_T           MessageSize;
	static BYTE      SharedBuffer[0x1000];
	
	Instance = (KrkrExtractCore*)Param;
	if (!Instance)
		return STATUS_INVALID_PARAMETER;

	while (Instance->AlpcClientShouldRun())
	{
		LOOP_ONCE
		{
			MessageSize     = 0;
			ReceivedMessage = (PPORT_MESSAGE)SharedBuffer;
			RtlZeroMemory(SharedBuffer, sizeof(SharedBuffer));

			Status = NtAlpcSendWaitReceivePort(
				Instance->AlpcGetPortHandle(),
				0,
				NULL,
				NULL,
				ReceivedMessage,
				&MessageSize,
				NULL,
				NULL
				);

			if (NT_FAILED(Status))
				break;
			
			Instance->AlpcProcessClientPackage(ReceivedMessage + sizeof(PORT_MESSAGE), ReceivedMessage->u1.s1.DataLength);
		}


		Ps::Sleep(20);
	}

	return 0;
}

NTSTATUS KrkrExtractCore::InitializeWithRpcMode()
{
	NTSTATUS       Status;
	UNICODE_STRING PortName;
	WCHAR          Path[0x200];
	WCHAR          IoPrivatePath[0x100];
	ULONG          Size;

	Size = GetEnvironmentVariableW(L"KrkrRpcIoPrivatePath", IoPrivatePath, countof(IoPrivatePath));
	if (Size == 0)
		return STATUS_NOT_FOUND;
	
	FormatStringW(Path, L"\\RPC Control\\%s", IoPrivatePath);
	RtlInitUnicodeString(&PortName, Path);
	Status = NtAlpcConnectPort(&m_AlpcPort, &PortName, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
	if (NT_FAILED(Status))
		return Status;
	
	return Nt_CreateThread(AlpcClientThread, this, FALSE, NtCurrentProcess(), &m_AlpcClientThread);
}

BOOL KrkrExtractCore::RpcSendToServer(ULONG MessageType, std::string&& Request)
{
	NTSTATUS     Status;
	PORT_MESSAGE PortMessage;

	RtlZeroMemory(&PortMessage, sizeof(PortMessage));

	PortMessage.u1.s1.DataLength  = (CSHORT)Request.length();
	PortMessage.u1.s1.TotalLength = (CSHORT)(Request.length() + sizeof(PortMessage));
	
	auto Message= AllocateAlpcMessage(&PortMessage, MessageType, Request.length(), Request.data());
	if (!Message)
		return STATUS_NO_MEMORY;

	Status = NtAlpcSendWaitReceivePort(m_AlpcPort, 0, Message.get(), NULL, NULL, NULL, NULL, NULL);
	return NT_SUCCESS(Status);
}

BOOL KrkrExtractCore::RpcSendToServer(ULONG MessageType, std::string&  Request)
{
	NTSTATUS     Status;
	PORT_MESSAGE PortMessage;

	RtlZeroMemory(&PortMessage, sizeof(PortMessage));

	PortMessage.u1.s1.DataLength = (CSHORT)Request.length();
	PortMessage.u1.s1.TotalLength = (CSHORT)(Request.length() + sizeof(PortMessage));

	auto Message = AllocateAlpcMessage(&PortMessage, MessageType, Request.length(), Request.data());
	if (!Message)
		return STATUS_NO_MEMORY;

	Status = NtAlpcSendWaitReceivePort(m_AlpcPort, 0, Message.get(), NULL, NULL, NULL, NULL, NULL);
	return NT_SUCCESS(Status);
}

using KrCreateWindowFunc = NTSTATUS (*NTAPI) (
	_In_  PVOID DllModule,
	_In_  ClientStub* Client,
	_Out_ ServerStub** Server
);

NTSTATUS KrkrExtractCore::InitializeWithLocalMode()
{
	WCHAR              UIModulePath[MAX_NTPATH];
	KrCreateWindowFunc KrCreateWindow;

	if (m_UIModule)
		return STATUS_SUCCESS;

	wnsprintfW(UIModulePath, countof(UIModulePath), L"%sKrkrExtract.UI.Lite.dll", m_WorkerDir.c_str());
	m_UIModule = (HMODULE)Nt_LoadLibrary(UIModulePath);

	if (!m_UIModule)
		return STATUS_DLL_NOT_FOUND;

	KrCreateWindow = (KrCreateWindowFunc)Nt_GetProcAddress(m_UIModule, "KrCreateWindow");
	if (!KrCreateWindow) 
	{
		NtClose(m_UIModule);
		m_UIModule = nullptr;
		return STATUS_NOT_FOUND;
	}

	return KrCreateWindow(m_SelfModule, this, &m_LocalServer);
}


NTSTATUS KrkrExtractCore::InitializePrivatePointersFromFile()
{
	NTSTATUS     Status;
	NtFileDisk   File;
	ULONG        Size;
	BYTE         Magic[4];
	WCHAR        FullPath[MAX_NTPATH];
	WCHAR        ExePath[MAX_NTPATH];
	Json::Value  Root;
	Json::Reader Reader;
	BOOL         Success;

	static BYTE JsonMagic[4] = { 0x44, 0x33, 0x22, 0x11 };

	Nt_GetModuleFileName(m_HostModule, ExePath, countof(ExePath));
	wnsprintfW(FullPath, countof(FullPath), L"%s.krconfig", ExePath);
	Status = File.Open(FullPath);
	if (NT_FAILED(Status))
		return Status;

	Size = File.GetSize32();
	if (Size <= 4)
		return STATUS_BUFFER_TOO_SMALL;

	Status = File.Read(Magic, sizeof(Magic));
	if (NT_FAILED(Status))
		return Status;

	if (RtlCompareMemory(Magic, JsonMagic, sizeof(Magic)) != sizeof(Magic))
		return STATUS_UNSUCCESSFUL;

	auto JsonData = AllocateMemorySafeP<BYTE>(Size - sizeof(Magic));
	if (!JsonData)
		return STATUS_NO_MEMORY;

	for (ULONG i = 0; i < Size - sizeof(Magic); i++) {
		JsonData.get()[i] ^= 0x6F;
	}

	Success = Reader.parse((PCSTR)JsonData.get(), (PCSTR)JsonData.get() + Size - sizeof(Magic), Root);
	if (Success) 
	{
		File.Delete();
		PrintConsoleW(L"Corrupted data : %s\n", FullPath);
		return STATUS_UNSUCCESSFUL;
	}

	for (auto Ptr = Root.begin(); Ptr != Root.end(); Ptr++)
	{
		if (Ptr.key().isString() && StrCompareA(Ptr.key().asString().c_str(), "Allocator") == 0 && Ptr->isIntegral()) {
			m_Allocator = Ptr->asUInt64() ? (PBYTE)m_HostModule + Ptr->asUInt64() : 0;
		}
		
		if (Ptr.key().isString() && StrCompareA(Ptr.key().asString().c_str(), "IStreamAdapterVtable") == 0 && Ptr->isIntegral()) {
			m_IStreamAdapterVtable = Ptr->asUInt64() ? (PBYTE)m_HostModule + Ptr->asUInt64() : 0;
		}

		if (Ptr.key().isString() && StrCompareA(Ptr.key().asString().c_str(), "TVPCreateBStream") == 0 && Ptr->isIntegral()) {
			m_TVPCreateBStream = Ptr->asUInt64() ? (PBYTE)m_HostModule + Ptr->asUInt64() : 0;
		}

		if (Ptr.key().isString() && StrCompareA(Ptr.key().asString().c_str(), "TVPCreateIStreamP") == 0 && Ptr->isIntegral()) {
			m_TVPCreateIStreamP = Ptr->asUInt64() ? (PBYTE)m_HostModule + Ptr->asUInt64() : 0;
		}

		if (Ptr.key().isString() && StrCompareA(Ptr.key().asString().c_str(), "TVPCreateIStreamStub") == 0 && Ptr->isIntegral()) {
			m_TVPCreateIStreamStub = Ptr->asUInt64() ? (PBYTE)m_HostModule + Ptr->asUInt64() : 0;
		}

		if (Ptr.key().isString() && StrCompareA(Ptr.key().asString().c_str(), "exporter") == 0 && Ptr->isIntegral())
		{
			m_TVPGetFunctionExporter = Ptr->asUInt64() ?
				(Prototype::TVPGetFunctionExporterFunc)((PBYTE)m_HostModule + Ptr->asUInt64()) :
				0;
		}
	}

	File.Close();
	return STATUS_SUCCESS;
}

NTSTATUS KrkrExtractCore::InitializeHook()
{
	NTSTATUS              Status;

	//
	// Krkr2 & normal Krkrz Module
	//

	Status = InitializePrivatePointersFromFile();
	if (NT_FAILED(Status)) {
		PrintConsoleW(L"Failed to initialize private pointers from config file : 0x%08x\n", Status);
	}

	if (GetTVPGetFunctionExporter() == nullptr) {
		SetTVPGetFunctionExporter((Prototype::TVPGetFunctionExporterFunc)Nt_GetProcAddress(m_HostModule, "TVPGetFunctionExporter"));
	}

	if (GetTVPGetFunctionExporter() != nullptr)
	{
		PrintConsoleW(L"TVPGetFunctionExporter : %08x\n", GetTVPGetFunctionExporter());

		Status = m_HookEngine->HookTVPGetFunctionExporter();
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"Failed to hook TVPGetFunctionExporter : 0x%08x\n", Status);
		}
	}
	else
	{
		Status = m_HookEngine->HookLoadLibraryA();
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"Failed to hook LoadLibraryA : 0x%08x\n", Status);
			return Status;
		}

		Status = m_HookEngine->HookLoadLibraryW();
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"Failed to hook LoadLibraryW : 0x%08x\n", Status);
			return Status;
		}

		Status = m_HookEngine->HookIsDebuggerPresent();
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"Failed to hook IsDebuggerPresent\n");
			return Status;
		}

		Status = m_HookEngine->HookCreateFileW();
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"Failed to hook CreateFileW\n");
			return Status;
		}
	}

	Status = m_HookEngine->HookCreateProcessInternalW();
	if (NT_FAILED(Status)) {
		PrintConsoleW(L"Failed to hook CreateProcessInternalW\n");
	}

	if (m_ModuleType == KrkrVersion::KRKRZ)
	{
		Status = m_HookEngine->HookMultiByteToWideChar();
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"Failed to hook MultiByteToWideChar\n");
		}
	}
	else if (m_ModuleType == KrkrVersion::KRKR2)
	{
		Status = m_HookEngine->HookIsDBCSLeadByte();
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"Failed to hook IsDBCSLeadByte\n");
		}

		Status = m_HookEngine->HookMultiByteToWideChar();
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"Failed to hook MultiByteToWideChar\n");
		}
	}

	//
	// module path check
	//
	
	switch (Nt_CurrentPeb()->OSMajorVersion)
	{
	case 6:
		if (Nt_CurrentPeb()->OSMinorVersion >= 2) {
			m_HookEngine->HookGetProcAddress();
		}
		break;

	case 10:
		m_HookEngine->HookGetProcAddress();
		break;
	}

	return STATUS_SUCCESS;
}



NTSTATUS KrkrExtractCore::InitializePrivatePointers()
{
	NTSTATUS Status;

	if (m_PointersInitialized)
		return STATUS_SUCCESS;

	switch (GetModuleType())
	{
	case KrkrExtractCore::KrkrVersion::KRKR2:
		Status = InitializeTVPCreateStreamCallKrkr2();
		break;

	case KrkrExtractCore::KrkrVersion::KRKRZ:
		Status = InitializeTVPCreateStreamCallKrkrZ();
		break;

	default:
		PrintConsoleW(L"Unknown module version\n");
		return STATUS_NOT_SUPPORTED;
	}

	if (NT_FAILED(Status)) {
		PrintConsoleW(L"InitializePrivatePointers failed : %08x\n", Status);
	}

	return Status;
}


//
// RiddleJoker+
//

NTSTATUS KrkrExtractCore::AddToRiddleJokerSpecialEntry(LPCWSTR FileName, PBYTE Decompress, ULONG Size, NtFileDisk& File, DWORD M2Magic)
{
	NTSTATUS                Status;
	LARGE_INTEGER           ReadSize;
	Xp3WalkerProxy          Proxyer;

	Status = WalkSenrenBankaIndexBuffer(
		this,
		Decompress,
		Size,
		File,
		M2Magic,
		Proxyer,
		[](
			KrkrClientProxyer* Proxyer, 
			std::shared_ptr<BYTE> CompressedIndexBuffer, 
			ULONG Size, 
			ULONG OriginalSize, 
			NtFileDisk& File,
			PVOID UserContext)->WalkerCallbackStatus
		{
			MemEntry  Entry;
			Entry.Buffer = CompressedIndexBuffer;
			Entry.Size   = Size;
			Entry.Hash   = MurmurHash64B(Entry.Buffer.get(), Entry.Size);

			((KrkrExtractCore*)Proxyer->GetPrivatePointer())->m_SpecialChunkMap.insert      (std::make_pair(Entry.Hash, Entry));
			((KrkrExtractCore*)Proxyer->GetPrivatePointer())->m_SpecialChunkMapBySize.insert(std::make_pair(Entry.Size, Entry));
			
			return WalkerCallbackStatus::STATUS_SKIP;
		}
	);

	if (NT_SUCCESS(Status)) {
		PrintConsoleW(L"AddToRiddleJokerSpecialEntry : Entry for [%s] was added\n", FileName);
	}

	return Status;
}


NTSTATUS KrkrExtractCore::AddToRiddleJokerSpecialEntryV2(LPCWSTR FileName, PBYTE Decompress, ULONG Size, NtFileDisk& File, DWORD M2Magic)
{
	NTSTATUS                Status;
	LARGE_INTEGER           ReadSize;
	Xp3WalkerProxy          Proxyer;

	Status = WalkSenrenBankaIndexV2Buffer(
		this,
		Decompress,
		Size,
		File,
		M2Magic,
		Proxyer,
		[](
			KrkrClientProxyer* Proxyer,
			std::shared_ptr<BYTE> CompressedIndexBuffer,
			ULONG Size,
			ULONG OriginalSize,
			NtFileDisk& File,
			PVOID UserContext)->WalkerCallbackStatus
		{
			MemEntry  Entry;
			Entry.Buffer = CompressedIndexBuffer;
			Entry.Size = Size;
			Entry.Hash = MurmurHash64B(Entry.Buffer.get(), Entry.Size);

			((KrkrExtractCore*)Proxyer->GetPrivatePointer())->m_SpecialChunkMap.insert(std::make_pair(Entry.Hash, Entry));
			((KrkrExtractCore*)Proxyer->GetPrivatePointer())->m_SpecialChunkMapBySize.insert(std::make_pair(Entry.Size, Entry));

			return WalkerCallbackStatus::STATUS_SKIP;
		}
	);

	if (NT_SUCCESS(Status)) {
		PrintConsoleW(L"AddToRiddleJokerSpecialEntryV2 : Entry for [%s] was added\n", FileName);
	}

	return Status;
}


NTSTATUS KrkrExtractCore::IsEncryptedSenrenBanka(PBYTE Decompress, ULONG Size, NtFileDisk& File, BOOL& IsEncrypted, DWORD M2Magic)
{
	NTSTATUS                Status;
	Xp3WalkerProxy          Proxyer;


	IsEncrypted = FALSE;

	Status = WalkSenrenBankaIndexBuffer(
		this,
		Decompress,
		Size,
		File,
		M2Magic,
		Proxyer,
		[](
			KrkrClientProxyer* Proxyer, 
			std::shared_ptr<BYTE> CompressedIndexBuffer, 
			ULONG Size, 
			ULONG OriginalSize, 
			NtFileDisk& File, 
			PVOID UserContext
			)->WalkerCallbackStatus
		{
			ULONG DecompressedSize;
			PBOOL IsEncryptedPtr = (PBOOL)UserContext;

			if (IsEncryptedPtr) {
				*IsEncryptedPtr = FALSE;
			}

			auto OriginalBuffer = AllocateMemorySafeP<BYTE>(OriginalSize);
			if (!OriginalBuffer)
				return WalkerCallbackStatus::STATUS_ERROR;

			DecompressedSize = OriginalSize;
			auto Success = uncompress(
				OriginalBuffer.get(),
				&OriginalSize,
				CompressedIndexBuffer.get(),
				Size);

			if (Success == Z_OK && OriginalSize == DecompressedSize)
				return WalkerCallbackStatus::STATUS_SKIP;

			IsEncryptedPtr = (PBOOL)UserContext;
			if (IsEncryptedPtr) {
				*IsEncryptedPtr = TRUE;
			}

			return WalkerCallbackStatus::STATUS_SKIP;
		},
		&IsEncrypted
	);

	return Status;
}



NTSTATUS KrkrExtractCore::IsEncryptedSenrenBankaV2(PBYTE Decompress, ULONG Size, NtFileDisk& File, BOOL& IsEncrypted, DWORD M2Magic)
{
	NTSTATUS                Status;
	Xp3WalkerProxy          Proxyer;


	IsEncrypted = FALSE;

	Status = WalkSenrenBankaIndexV2Buffer(
		this,
		Decompress,
		Size,
		File,
		M2Magic,
		Proxyer,
		[](
			KrkrClientProxyer* Proxyer,
			std::shared_ptr<BYTE> CompressedIndexBuffer,
			ULONG Size,
			ULONG OriginalSize,
			NtFileDisk& File,
			PVOID UserContext
			)->WalkerCallbackStatus
			{
				ULONG DecompressedSize;
				PBOOL IsEncryptedPtr = (PBOOL)UserContext;

				if (IsEncryptedPtr) {
					*IsEncryptedPtr = FALSE;
				}

				auto OriginalBuffer = AllocateMemorySafeP<BYTE>(OriginalSize);
				if (!OriginalBuffer)
					return WalkerCallbackStatus::STATUS_ERROR;

				DecompressedSize = OriginalSize;
				auto Success = uncompress(
					OriginalBuffer.get(),
					&OriginalSize,
					CompressedIndexBuffer.get(),
					Size
				);

				if (Success == Z_OK && OriginalSize == DecompressedSize)
					return WalkerCallbackStatus::STATUS_SKIP;

				IsEncryptedPtr = (PBOOL)UserContext;
				if (IsEncryptedPtr) {
					*IsEncryptedPtr = TRUE;
				}

				return WalkerCallbackStatus::STATUS_SKIP;
			},
		&IsEncrypted
		);
	
	return Status;
}



BOOL ValidateMaybeXp3File(PCWSTR FilePath)
{
	NTSTATUS      Status;
	NtFileDisk    File;
	WORD          DosSignature;
	LARGE_INTEGER BytesTransferred;
	BYTE          Buffer[countof(StaticXP3V2Magic)];

	Status = File.Open(FilePath);
	if (NT_FAILED(Status))
		return FALSE;

	BytesTransferred.QuadPart = 0;
	Status = File.Read(Buffer, countof(StaticXP3V2Magic), &BytesTransferred);
	if (NT_FAILED(Status) || BytesTransferred.QuadPart != countof(StaticXP3V2Magic))
		return FALSE;
		
	if (RtlCompareMemory(Buffer, StaticXP3V2Magic, countof(StaticXP3V2Magic)) == countof(StaticXP3V2Magic))
		return TRUE;

	DosSignature = *((PWORD)Buffer);
	if (DosSignature == IMAGE_DOS_SIGNATURE && GetExtensionUpper(FilePath) == L"EXE")
		return TRUE;

	File.Close();
	return FALSE;
}

BOOL ListAllFileInPath(PCWSTR Root, std::vector<std::wstring>& FileList)
{
	std::wstring     CurrentSearchPath, FullPath;
	WIN32_FIND_DATAW FindInfo;
	HANDLE           FileHandle;
	LONG_PTR         Length;

	Length = StrLengthW(Root);
	if (Root == nullptr || Length == 0)
		return FALSE;

	CurrentSearchPath = Root;
	if (Root[Length - 1] != L'/' && Root[Length - 1] != L'\\') {
		CurrentSearchPath += L"\\";
	}
		
	CurrentSearchPath += L"*.*";

	FileHandle = Nt_FindFirstFile(CurrentSearchPath.c_str(), &FindInfo);
	if (FileHandle == INVALID_HANDLE_VALUE)
		return FALSE;

	do
	{
		if (StrCompareW(FindInfo.cFileName, L".") == 0 || StrCompareW(FindInfo.cFileName, L"..") == 0)
			continue;

		FullPath = Root;
		if (Root[Length - 1] != L'/' && Root[Length - 1] != L'\\') {
			FullPath += L"\\";
		}

		FullPath += FindInfo.cFileName;

		if (FindInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			ListAllFileInPath(FullPath.c_str(), FileList);
		}

		if (ValidateMaybeXp3File(FullPath.c_str()))
		{
			PrintConsoleW(L"Found file : %s\n", FullPath.c_str());
			FileList.push_back(FullPath);
		}

	} while (Nt_FindNextFile(FileHandle, &FindInfo));

	return Nt_FindClose(FileHandle);
}

NTSTATUS KrkrExtractCore::ListAllXp3Files(std::vector<std::wstring>& Xp3List)
{
	NTSTATUS                  Status;
	BOOL                      Success;
	std::vector<std::wstring> FileList;

	Xp3List.clear();
	if (!ListAllFileInPath(m_WorkerDir.c_str(), FileList) || FileList.size() == 0)
		return STATUS_UNSUCCESSFUL;

	for (auto& FilePath : FileList)
	{
		Success = FALSE;
		Status = IsXp3File(FilePath.c_str(), Success);
		if (NT_SUCCESS(Status))
		{
			if (Success) {
				Xp3List.push_back(FilePath);
			}
		}
	}

	if (Xp3List.size() == 0)
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS;
}

NTSTATUS KrkrExtractCore::GetRiddleJokerSpecialEntry(PCWSTR FileName)
{
	NTSTATUS                Status;
	DWORD                   ChunkMagic;
	Xp3WalkerProxy          Xp3Proxyer;

	Status = WalkXp3ArchiveIndex(
		this,
		FileName,
		Xp3Proxyer,
		ChunkMagic,
		WalkXp3IndexCallbackDummy,
		WalkXp3IndexCallbackDummy,
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			PrintConsoleW(L"Pre-init : reading [%s] special entry.\n", FileName);
			return AddToRiddleJokerSpecialEntry(FileName, Buffer, (ULONG)Size, File, Magic);
		},
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			PrintConsoleW(L"Pre-init : reading [%s] special entry. (v2)\n", FileName);
			return AddToRiddleJokerSpecialEntryV2(FileName, Buffer, (ULONG)Size, File, Magic);
		},
		WalkXp3IndexCallbackDummy,
		WalkXp3IndexCallbackDummy
	);

	return Status;
}



NTSTATUS KrkrExtractCore::ReadRiddleJokerSpecial()
{
	NTSTATUS                  Status;
	std::vector<std::wstring> ArchiveList;

	Status = ListAllXp3Files(ArchiveList);
	if (NT_FAILED(Status))
		return Status;

	for (auto& FilePath : ArchiveList) 
	{
		Status = GetRiddleJokerSpecialEntry(FilePath.c_str());
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"KrkrExtractCore::ReadRiddleJokerSpecial failed : %s, %08x\n", FilePath.c_str(), Status);
		}
	}

	return STATUS_SUCCESS;
}


NTSTATUS KrkrExtractCore::InitializeRand()
{
	ULONG64  Seeds[4];
	WCHAR    Path[MAX_PATH];

	RtlZeroMemory(Path, sizeof(Path));
	Nt_GetExeDirectory(Path, countof(Path));

	Seeds[0] = MakeQword(GetCurrentProcessId(), GetCurrentThreadId());
	Seeds[1] = MakeQword(Nt_CurrentPeb()->ProcessHeap, Nt_CurrentTeb()->EnvironmentPointer);
	Seeds[2] = MurmurHash64B(Path, lstrlenW(Path) * 2);
	Seeds[3] = MakeQword(m_SelfModule, Nt_GetModuleHandle(NULL));

	init_by_array64(Seeds, countof(Seeds));
	return STATUS_SUCCESS;
}


NTSTATUS KrkrExtractCore::InitializeGlobalFlags()
{
	try {
		auto Reg = winreg::RegKey(HKEY_CURRENT_USER, L"SOFTWARE\\KrkrExtract\\Settings");
		if (Reg.TryGetQwordValue(L"OpenConsole") == 1) {
			AllocConsole();
		}
	}
	catch (...) {
		return STATUS_UNSUCCESSFUL;
	}
}


BOOL KrkrExtractCore::FindCodeSlow(PCCHAR Start, ULONG Size, PCCHAR Pattern, ULONG PatternLen)
{
	return KMP(Start, Size, Pattern, PatternLen) ? TRUE : FALSE;
}


BOOL KrkrExtractCore::AddFileEntry(PCWSTR FileName, ULONG Length)
{
	BOOL             FindDot;
	SectionProtector Protector(m_AddFileEntryLock.Get());

	if (!FileName || !Length)
		return FALSE;

	FindDot = FALSE;
	for (ULONG i = 0; i < Length; i++)
	{
		if (FileName[i] == L'.')
		{
			FindDot = TRUE;
			break;
		}
	}

	if (FindDot) {
		FileNameList.push_back(std::wstring(FileName, Length));
	}

	return FindDot;
}


NTSTATUS KrkrExtractCore::InitWindow()
{
	BOOL  Status;

	if (m_WindowIsActived) {
		TellServerLogOutput(LogLevel::LOG_WARN, L"UI : The main window is alreay actived");
		return STATUS_SUCCESS;
	}
	
	if (!TellServerUIReady())
	{
		TellServerLogOutput(LogLevel::LOG_ERROR, L"UI : Failed to change UI status");
		return STATUS_UNSUCCESSFUL;
	}

	TellServerLogOutput(LogLevel::LOG_INFO, L"Virtual Console initializated, type `help` to see list of commands");
	m_WindowIsActived = TRUE;

	return STATUS_SUCCESS;
}


NTSTATUS KrkrExtractCore::InitHookWithExe()
{
	NTSTATUS Status;

	if (m_InitializationType != ModuleInitializationType::NotInited)
		return STATUS_SUCCESS;

	LOOP_ONCE
	{
		Status = InitWindow();
		if (NT_FAILED(Status))
			break;

		m_InitializationType = ModuleInitializationType::ByExeModule;
	}
	return Status;
}

NTSTATUS KrkrExtractCore::InitHookWithDll(LPCWSTR ModuleName, PVOID ImageBase)
{
	NTSTATUS    Status;
	ULONG_PTR   Length;
	ULONG64     Extension;
	PVOID       pV2Link;

	LOOP_ONCE
	{
		Status = STATUS_ALREADY_REGISTERED;
		if (m_InitializationType != ModuleInitializationType::NotInited)
			break;

		Status = STATUS_UNSUCCESSFUL;
		if (ImageBase == NULL)
			break;

		Length = StrLengthW(ModuleName);
		if (Length <= 4)
			break;

		Extension = *(PULONG64)&ModuleName[Length - 4];
		
		if (Extension != TAG4W('.dll') && Extension != TAG4W('.tpm'))
			break;

		if (Nt_GetProcAddress(ImageBase, "FlushInstructionCache"))
			break;

		pV2Link = Nt_GetProcAddress(ImageBase, "V2Link");
		if (pV2Link == NULL)
			break;

		Status = m_HookEngine->HookV2Link(ImageBase);
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"KrkrExtractCore::InitHookWithDll : m_HookEngine->HookV2Link failed, %08x\n", Status);
			return Status;
		}

		Status = InitWindow();
		m_InitializationType = ModuleInitializationType::ByDllModule;
	}
	return Status;
}


