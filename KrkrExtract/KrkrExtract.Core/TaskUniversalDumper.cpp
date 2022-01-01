#include "CoreTask.h"
#include "Decoder.h"
#include "tp_stub.h"
#include "TaskAutoUnlocker.h"
#include "SafeMemory.h"
#include "Helper.h"
#include <shlwapi.h>
#include <tlhelp32.h>
#include <unordered_set>
#include <tuple>
#include <Shlobj.h>


CoUniversalDumperTask::CoUniversalDumperTask(KrkrClientProxyer* Proxyer):
	m_Proxyer(Proxyer)
{
	m_FileList.clear();
}


CoUniversalDumperTask::~CoUniversalDumperTask()
{
	m_FileList.clear();
	m_Proxyer = nullptr;
}



INT AddPath(wstring& Path)
{
	return SHCreateDirectory(NULL, GetDirName(Path).c_str());
}


BOOL IsValid(ULONG value)
{
	const DWORD MaxChar = 0x10ffff;
	const DWORD MinReserved = 0x00d800;
	const DWORD MaxReserved = 0x00dfff;

	return (value <= MaxChar) && ((value < MinReserved) || (value > MaxReserved));
}

BOOL BasicValid(PWCHAR TestName, SIZE_T Length)
{
	BOOL Found = FALSE;

	for (SIZE_T i = 0; i < Length; i++)
	{
		if (TestName[i] == L'.')
		{
			Found = TRUE;
		}
		else if (TestName[i] == L'?' || TestName[i] == L'*' || TestName[i] == L':')
		{
			Found = FALSE;
			break;
		}
		else if (IsValid(TestName[i]) == FALSE)
		{
			Found = FALSE;
			break;
		}
	}
	return Found;
}


BOOL FinalValid(PWCHAR TestName)
{
	wstring Test(TestName);
	auto Index = Test.find_first_of(L'>');
	if (Index == wstring::npos)
		return TRUE;
	auto sub = Test.substr(Index + 1);
	BOOL Found = FALSE;
	for (auto& ch : sub)
	{
		if (ch == L'.')
		{
			Found = TRUE;
			break;
		}
	}
	return Found;
}


BOOL IdentifyFileNameWithOffset(PVOID Address, SIZE_T Size, SIZE_T Offset, std::unordered_set<wstring>& FileList)
{
	SIZE_T Length;
	PWSTR  Name;

	if (Offset >= Size)
		return FALSE;

	Name = (PWCH)((PBYTE)Address + Offset);
	Length = wcsnlen_s(Name, (Size - Offset) / 2);
	if (Length < 5)
		return FALSE;

	if (BasicValid(Name, Length) && FinalValid(Name))
	{
		wstring FileName(Name, Length);
		FileList.insert(FileName);
		return TRUE;
	}
}

void IdentifyFileName(PVOID Address, SIZE_T Size, std::unordered_set<wstring>& FileList)
{
	BOOL  Success;

	if (Size <= 4)
		return;

	for (ULONG Offset = 0; Offset < Size; Offset += 1)
	{
		Success = IdentifyFileNameWithOffset(Address, Size, Offset, FileList);
		if (Success)
			break;
	}
}


void CoUniversalDumperTask::IdentifyFileNameSafe(PVOID Address, SIZE_T Size, std::unordered_set<wstring>& FileList)
{
	SEH_TRY{
		IdentifyFileName(Address, Size, FileList);
	}
	SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {

	}
}

NTSTATUS CoUniversalDumperTask::LoadFiles(std::unordered_set<wstring>& FileList)
{
	DWORD              HeapCount;
	PROCESS_HEAP_ENTRY Entry;
	

	HeapCount = RtlGetProcessHeaps(0, NULL);
	if (HeapCount == 0)
		return STATUS_NOT_FOUND;

	auto Heaps = AllocateMemorySafeP<HANDLE>(HeapCount * sizeof(HANDLE));
	RtlGetProcessHeaps(HeapCount, Heaps.get());
	

	for (DWORD i = 0; i < HeapCount; i++)
	{
		if (IsBeingTerminated()) {
			return STATUS_CANCELLED;
		}

		RtlLockHeap(Heaps.get()[i]);
		Entry.lpData = NULL;
		while (HeapWalk(Heaps.get()[i], &Entry))
		{
			if (IsBeingTerminated()) {
				RtlUnlockHeap(Heaps.get()[i]);
				return STATUS_CANCELLED;
			}
			
			IdentifyFileNameSafe((PVOID)Entry.lpData, Entry.cbData, FileList);
		}
		RtlUnlockHeap(Heaps.get()[i]);
	}

	return STATUS_SUCCESS;
}


enum class DumpType : SIZE_T
{
	DUMP_NONE   = 0,
	DUMP_NORMAL = 1,
	DUMP_KRKRZ  = 2
};


wstring FixName(wstring& FixedName)
{
	auto Pos = FixedName.find_last_of(L'.');
	if (Pos != wstring::npos) {
		return FixedName.substr(0, Pos);
	}

	return FixedName;
}

std::tuple<wstring, wstring, DumpType> FetchPackageName(const wstring& Name)
{
	wstring PackageName = {};
	auto pos = Name.find_first_of(L'>');
	if (pos != wstring::npos)
	{
		PackageName = Name.substr(0, pos);
		auto ExtName = GetExtensionUpper(PackageName);
		if (ExtName == L"XP3" || ExtName == L"BIN") {
			return std::make_tuple(FixName(PackageName), Name.substr(pos + 1), DumpType::DUMP_NORMAL);
		}
	}

	pos = Name.find_first_of(L'/');
	if (pos != wstring::npos)
	{
		PackageName = Name.substr(0, pos);
		auto ExtName = GetExtensionUpper(PackageName);
		if (ExtName == L"XP3" || ExtName == L"BIN") {
			return std::make_tuple(FixName(PackageName), Name.substr(pos + 1), DumpType::DUMP_KRKRZ);
		}
	}
	
	return std::make_tuple(wstring(L""), wstring(L""), DumpType::DUMP_NONE);
}



INT NTAPI DumperEpFilter(ULONG Code, EXCEPTION_POINTERS* Ep);


void CoUniversalDumperTask::DumpStreamSafe(IStream* Stream, PCWSTR ExtName, PCWSTR OutputFileName)
{
	HRESULT    HResult;
	IPlugin*   Plugin;

	SEH_TRY
	{
		LOOP_ONCE
		{
			Plugin = m_Proxyer->GetPluginWithExtName(ExtName);
			if (Plugin == nullptr) {
				Plugin = m_Proxyer->GetDefaultPlugin();
			}

			HResult = Plugin->Unpack(OutputFileName, Stream);
			if (FAILED(HResult))
				break;

			if (IsBeingTerminated()) {
				if (Stream) Stream->Release();
				SetLastMessage(TRUE, L"Task cancelled");
				return;
			}

			Stream->Release();
			Stream = NULL;
		}
	}
	SEH_EXCEPT(DumperEpFilter(GetExceptionCode(), GetExceptionInformation())) {
	}
}


void CoUniversalDumperTask::ThreadFunction()
{
	NTSTATUS                        Status;
	wstring                         FixFileName, ExtName, OutFilePathFull, OutFilePath;
	ULONG_PTR                       Index, CurFile;
	tTJSBinaryStream*               BStream;
	IStream*                        Stream;
	STATSTG                         Stat;
	std::unordered_set<wstring>     FileList;
	TaskAutoUnlocker                Unlocker(this, m_Proxyer);


	if (m_Proxyer->GetTVPCreateBStream() == NULL) 
	{
		m_Proxyer->TellServerLogOutput(LogLevel::LOG_ERROR, L"Unknown engine type\n");
		SetLastMessage(FALSE, L"Unable to parse interface");
		return;
	}

	OutFilePath += m_Proxyer->GetWorkerDir();
	OutFilePath += L"\\KrkrExtract_Output\\";

	if (IsBeingTerminated()) {
		SetLastMessage(TRUE, L"Task cancelled");
		return;
	}

	LoadFiles(FileList);
	if (FileList.size() == 0) {
		return;
	}

	CurFile = 1;
	for (auto& FileName : FileList)
	{
		auto NameInfo    = FetchPackageName(FileName);
		auto PackageName = std::get<0>(NameInfo);
		auto FilePath    = std::get<1>(NameInfo);
		auto Type        = std::get<2>(NameInfo);
		FixFileName      = GetPackageName(FilePath);

		if (IsBeingTerminated()) {
			SetLastMessage(TRUE, L"Task cancelled");
			return;
		}

		m_Proxyer->TellServerProgressBar(L"Universal dumper", CurFile, FileList.size());
		m_Proxyer->TellServerLogOutput(LogLevel::LOG_INFO, FixFileName.c_str());

		OutFilePathFull = OutFilePath + PackageName + L"\\" + FixFileName;
		AddPath(OutFilePathFull);

		LOOP_ONCE
		{
			BStream = m_Proxyer->HostTVPCreateStream(FileName.c_str());
			if (BStream == NULL)
				break;

			Stream = m_Proxyer->HostConvertBStreamToIStream(BStream);
			if (Stream == NULL)
				break;

			ExtName = GetExtensionUpper(FileName);
			DumpStreamSafe(Stream, ExtName.c_str(), OutFilePathFull.c_str());
		}

		CurFile++;
	}
}




