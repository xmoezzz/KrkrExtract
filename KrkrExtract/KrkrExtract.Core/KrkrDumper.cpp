#include "CoreTask.h"
#include "XP3Parser.h"
#include "Helper.h"
#include "TaskAutoUnlocker.h"
#include "SafeMemory.h"
#include <zlib.h>
#include <Shlobj.h>

void FormatM2PackName(wstring& PackageName, ttstr& OutName);

CoDumperTask::CoDumperTask(KrkrClientProxyer* Proxyer, PCWSTR FilePath)
{
	m_Proxyer  = Proxyer;
	m_FilePath = FilePath;
}

CoDumperTask::~CoDumperTask()
{
	SendKill();
	Ps::Sleep(500);
	Stop();
}

NTSTATUS CoDumperTask::ExtractFiles(Xp3WalkerProxy& Proxyer)
{
	ttstr M2Prefix;
	ttstr NormalPrefix;

	FormatPath(m_FilePath, NormalPrefix);
	FormatM2PackName(m_FilePath, M2Prefix);

	if (!Proxyer.IsExtractFile()) {
		return this->DumpFileWithIStream(M2Prefix, NormalPrefix, Proxyer);
	}

	return this->DumpFileWithArchive(Proxyer);
}


struct EnumXp3PluginArgs
{
	CoDumperTask*   Task;
	Xp3IndexAccess* Proxyer;
};


void CoDumperTask::ThreadFunction()
{
	NTSTATUS                          Status;
	DWORD                             M2MagicChunk;
	Xp3WalkerProxy                    Xp3Proxyer;
	TaskAutoUnlocker                  Unlocker(this, m_Proxyer);

	if (IsBeingTerminated()) {
		SetLastMessage(TRUE, L"Task cancelled");
		return;
	}

	Status = WalkXp3ArchiveIndex(
		m_Proxyer,
		m_FilePath.c_str(),
		Xp3Proxyer,
		M2MagicChunk,
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			BOOL              Status;
			EnumXp3PluginArgs Args;

			Args.Task    = this;
			Args.Proxyer = &Proxyer;
			
			Status = KrkrProxyer->EnumXp3Plugin(
					&Args,
					[](void* Param, Xp3ArchivePlugin* Plugin)->BOOL
					{
						BOOL               Success;
						EnumXp3PluginArgs* Args;

						Args    = (EnumXp3PluginArgs*)Param;
						Success = Plugin->WalkXp3Index(Args->Task, Args->Proxyer);

						//
						// FALSE : continue
						// TRUE  : break
						//

						return Success ? FALSE : TRUE;
					}
				);

			if (!Status) 
			{
				KrkrProxyer->TellServerLogOutput(
					LogLevel::LOG_WARN,
					L"no custom xp3 plugin available"
				);

				return STATUS_UNSUCCESSFUL;
			}

			return STATUS_SUCCESS;
		},
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			NTSTATUS NtStatus;
			NtStatus = WalkNormalIndexBuffer(KrkrProxyer, Buffer, (ULONG)Size, File, Magic, Proxyer);
			if (NT_FAILED(NtStatus)) {
				PrintConsoleW(L"WalkNormalIndexBuffer failed, %08x\n", NtStatus);
			}
			return NtStatus;
		},
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			NTSTATUS NtStatus;
			NtStatus = WalkSenrenBankaIndexBuffer(KrkrProxyer, Buffer, (ULONG)Size, File, Magic, Proxyer);
			if (NT_FAILED(NtStatus)) {
				PrintConsoleW(L"WalkSenrenBankaIndexBuffer failed, %08x\n", NtStatus);
			}
			return NtStatus;
		},
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			NTSTATUS NtStatus;
			NtStatus = WalkSenrenBankaIndexV2Buffer(KrkrProxyer, Buffer, (ULONG)Size, File, Magic, Proxyer);
			if (NT_FAILED(NtStatus)) {
				PrintConsoleW(L"WalkSenrenBankaIndexV2Buffer failed, %08x\n", NtStatus);
			}
			return NtStatus;
		},
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			NTSTATUS NtStatus;
			NtStatus = WalkNekoVol0IndexBuffer(KrkrProxyer, Buffer, (ULONG)Size, File, Magic, Proxyer);
			if (NT_FAILED(NtStatus)) {
				PrintConsoleW(L"WalkNekoVol0IndexBuffer failed, %08x\n", NtStatus);
			}
			return NtStatus;
		},
		WalkXp3IndexCallbackM(KrkrProxyer, Buffer, Size, File, Magic, Proxyer)
		{
			NTSTATUS NtStatus;
			NtStatus = WalkKrkrZIndexBuffer(KrkrProxyer, Buffer, (ULONG)Size, File, Magic, Proxyer);
			if (NT_FAILED(NtStatus)) {
				PrintConsoleW(L"WalkKrkrZIndexBuffer failed, %08x\n", NtStatus);
			}
			return NtStatus;
		}
	);

	if (NT_FAILED(Status))
	{
		PrintConsoleW(L"WalkXp3ArchiveIndex failed, %08x\n", Status);
		SetLastMessage(FALSE, L"WalkXp3ArchiveIndex failed");
		return;
	}

	if (IsBeingTerminated()) {
		SetLastMessage(TRUE, L"Task cancelled");
		return;
	}

	Status = ExtractFiles(Xp3Proxyer);
	if (Status == STATUS_CANCELLED) {
		SetLastMessage(TRUE, L"Task cancelled");
		return;
	}

	return;
}

void FormatM2PackName(wstring& PackageName, ttstr& OutName)
{
	OutName.Clear();
	OutName = L"archive://./";
	OutName += GetPackageName(PackageName).c_str();
	OutName += L"/";
}


void CoDumperTask::DecryptWorker(ULONG64 EncryptOffset, PBYTE Buffer, ULONG BufferSize, ULONG Hash)
{
	tTVPXP3ArchiveExtractionFilter Filter;

	Filter = (tTVPXP3ArchiveExtractionFilter)m_Proxyer->GetXP3Filter();

	tTVPXP3ExtractionFilterInfo Info(0, Buffer, BufferSize, Hash);
	if (Filter != NULL) {
		Filter(&Info);
	}
}


wstring CoDumperTask::GetPossibleName(XP3Index& Item)
{
	LOOP_ONCE
	{
		if (StrLengthW(Item.info.FileName) != 0)
			break;
		
		if (wcschr(Item.info.FileName, L'.') != nullptr)
			break;

		return Item.yuzu.FileName;
	};
	
	return Item.info.FileName;
}


INT NTAPI DumperEpFilter(ULONG Code, EXCEPTION_POINTERS* Ep)
{
	NTSTATUS   Status;
	
	switch (Code)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		PrintConsoleW(L"DumperPluginsEpFilter : EXCEPTION_ACCESS_VIOLATION\n");
		PrintConsoleW(L"Eip : %p", Ep->ContextRecord->Eip);
		PrintConsoleW(L"Dll Base : %p", Nt_GetModuleHandle(L"KrkExtract.Core.dll"));
		PrintConsoleW(L"Exe Base : %p", Nt_GetExeModuleHandle());
		switch (Ep->ExceptionRecord->ExceptionInformation[0])
		{
		case 0:
			PrintConsoleW(L"Reading at address : %p\n", Ep->ExceptionRecord->ExceptionAddress);
			break;

		case 1:
			PrintConsoleW(L"Writing at address : %p\n", Ep->ExceptionRecord->ExceptionAddress);
			break;

		case 8:
			PrintConsoleW(L"Executing at address : %p\n", Ep->ExceptionRecord->ExceptionAddress);
			break;
		}
		
		Status = CreateMiniDump(Ep);
		if (NT_FAILED(Status))
			return EXCEPTION_EXECUTE_HANDLER;

		PrintConsoleW(
			L"Mini crash dump is generated, please send it to my email:\n"
			L"xmoe.project@gmail.com\n"
			L"Or create a new issue on github.\n"
			L"Mini crash dump contains only limited info(no sensitive info)\n"
		);
		return EXCEPTION_EXECUTE_HANDLER;

	}

	return EXCEPTION_CONTINUE_SEARCH;
}


HRESULT CoDumperTask::DumpStreamSafe(IStream* Stream, PCWSTR ExtName, PCWSTR OutputFileName)
{
	HRESULT    Success;
	IPlugin*   Plugin;

	SEH_TRY
	{
		LOOP_ONCE
		{
			Plugin = m_Proxyer->GetPluginWithExtName(ExtName);
			if (Plugin == nullptr) {
				Plugin = m_Proxyer->GetDefaultPlugin();
			}

			Success = Plugin->Unpack(OutputFileName, Stream);
			if (FAILED(Stream)) {
				PrintConsoleW(L"CoDumperTask::DumpStreamSafe : Plugin->Unpack failed, hr = %08x\n", Success);
				break;
			}

			Stream->Release();
		}
	}
	SEH_EXCEPT(DumperEpFilter(GetExceptionCode(), GetExceptionInformation()))
	{
		PrintConsoleW(L"CoDumperTask::DumpStreamSafe : fatal error.\n");
		return E_FAIL;
	}

	return Success;
}

NTSTATUS CoDumperTask::DumpFileWithIStream(ttstr M2Prefix, ttstr NormalPrefix, Xp3WalkerProxy& Proxyer)
{
	NTSTATUS                               Status;
	HRESULT                                HStatus;
	ttstr                                  OutFileName;
	wstring                                OutFilePath, OutFilePathFull;
	ULONG                                  Index, Pos;
	wstring                                ExtName, FixedPathName, FileName;
	IStream*                               Stream;
	tTJSBinaryStream*                      BStream;
	vector<wstring>                        Failed;
	std::wstring                           CurrentDir;
	STATSTG                                Stat;

	CurrentDir = m_Proxyer->GetWorkerDir();
	m_Proxyer->GetTVPCreateBStream();
	
	FixedPathName = GetPackageName(m_FilePath);
	Pos = FixedPathName.find_last_of(L'.');
	if (Pos != wstring::npos) {
		FixedPathName = FixedPathName.substr(0, Pos);
	}

	OutFilePath += CurrentDir;
	OutFilePath += L"KrkrExtract_Output\\";
	OutFilePath += FixedPathName;
	OutFilePath += L"\\";

	Index = 1;
	auto& Items = Proxyer.GetItems();
	for (auto& Item : Items)
	{
		if (IsBeingTerminated())
			return STATUS_CANCELLED;

		m_Proxyer->TellServerProgressBar(L"Dumper", Index, Items.size());

		if (Proxyer.IsM2Krkr() && Item.m_IsM2Format)
		{
			OutFileName = M2Prefix + Item.yuzu.FileName;
			m_Proxyer->TellServerLogOutput(LogLevel::LOG_INFO, Item.yuzu.FileName);

			ExtName = GetExtensionUpper(Item.yuzu.FileName);
			OutFilePathFull = OutFilePath + Item.yuzu.FileName;
		}
		else
		{
			OutFileName = NormalPrefix + Item.info.FileName;
			m_Proxyer->TellServerLogOutput(LogLevel::LOG_INFO, Item.info.FileName);

			ExtName = GetExtensionUpper(Item.info.FileName);
			OutFilePathFull = OutFilePath + Item.info.FileName;
		}

		SHCreateDirectory(NULL, GetDirName(OutFilePathFull).c_str());

		try
		{
			LOOP_ONCE
			{
				Stream = TVPCreateIStream(OutFileName, TJS_BS_READ);
				if (Stream != NULL)
					break;

				OutFileName = GetPossibleName(Item).c_str();
				FileName    = GetPackageName(wstring(OutFileName.c_str()));
				BStream     = m_Proxyer->HostTVPCreateStream(FileName.c_str());
				if (BStream == NULL)
					break;

				Stream = m_Proxyer->HostConvertBStreamToIStream(BStream);
				if (Stream == NULL)
					break;
			};

			HStatus = E_FAIL;

			if (Stream) {
				HStatus = DumpStreamSafe(Stream, ExtName.c_str(), OutFilePathFull.c_str());
			}

			if (FAILED(HStatus)) {
				Failed.push_back(OutFilePathFull);
			}
		}
		catch (...)
		{
			Failed.push_back(OutFilePathFull);
		}

		Index++;
	}

	for (auto& Item : Failed) {
		m_Proxyer->TellServerLogOutput(LogLevel::LOG_ERROR, L"Failed to open : %s\n", Item.c_str());
	}

	return STATUS_SUCCESS;
}

NTSTATUS CoDumperTask::DumpFileWithArchive(Xp3WalkerProxy& Proxyer)
{
	NTSTATUS           Status;
	NtFileDisk         FakeWorker, File;
	ULONG              OriSize, ArcSize, Index;
	wstring            CurrentPath, PackageName, OutputFilePath;

	CurrentPath = m_Proxyer->GetWorkerDir();
	PackageName = GetPackageName(wstring(m_FilePath));
	auto&& Items = Proxyer.GetItems();

	Status = FakeWorker.Open(m_FilePath.c_str());
	if (NT_FAILED(Status))
		return Status;

	Index = 1;
	for (auto& Item : Items)
	{
		if (IsBeingTerminated())
			return STATUS_CANCELLED;

		OutputFilePath = CurrentPath;
		OutputFilePath += L"KrkrExtract_Output\\";
		OutputFilePath += PackageName;
		OutputFilePath += L"\\";
		OutputFilePath += Item.info.FileName;
		SHCreateDirectory(NULL, GetDirName(OutputFilePath).c_str());

		m_Proxyer->TellServerProgressBar(L"Dumper", Index, Items.size());
		m_Proxyer->TellServerLogOutput(LogLevel::LOG_INFO, Item.info.FileName);

		if (Item.info.EncryptedFlag & 7)
		{
			auto OutBuffer   = AllocateMemorySafeP<BYTE>(Item.info.ArchiveSize.LowPart);
			auto WriteBuffer = AllocateMemorySafeP<BYTE>(Item.info.OriginalSize.LowPart);

			if (!OutBuffer || !WriteBuffer)
				continue;

			FakeWorker.Seek(Item.segm.segm[0].Offset.LowPart, FILE_BEGIN);
			FakeWorker.Read(OutBuffer.get(), Item.info.ArchiveSize.LowPart);

			OriSize = Item.info.OriginalSize.LowPart;
			ArcSize = Item.info.ArchiveSize.LowPart;

			if (uncompress(WriteBuffer.get(), &OriSize, OutBuffer.get(), ArcSize) != Z_OK || OriSize != Item.info.OriginalSize.LowPart)
				continue;

			Status = File.Create(OutputFilePath.c_str());
			if (NT_FAILED(Status))
				continue;

			DecryptWorker(0, WriteBuffer.get(), Item.info.ArchiveSize.LowPart, Item.adlr.Hash);
			File.Write(WriteBuffer.get(), Item.info.OriginalSize.LowPart);
		}
		else
		{
			auto WriteBuffer = AllocateMemorySafeP<BYTE>(Item.info.OriginalSize.LowPart);
			if (!WriteBuffer)
				continue;

			FakeWorker.Seek(Item.segm.segm[0].Offset.LowPart, FILE_BEGIN);
			FakeWorker.Read(WriteBuffer.get(), Item.info.OriginalSize.LowPart);

			DecryptWorker(0, WriteBuffer.get(), Item.info.ArchiveSize.LowPart, Item.adlr.Hash);

			Status = File.Create(OutputFilePath.c_str());
			if (NT_FAILED(Status))
				continue;

			File.Write(WriteBuffer.get(), Item.info.OriginalSize.LowPart);
		}
		File.Close();
		Index++;
	}
	FakeWorker.Close();
	return Status;
}

