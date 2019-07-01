//////////////////////////UNIMPL Area///////////////////////////////////////
//[+] Pls ingore this feature..
///////////////////////////////////////////////////////////////////////////


#include "KrkrUniversalDumper.h"
#include "KrkrExtract.h"
#include "KrkrExtend.h"
#include "MyLib.h"
#include "FakePNG.h"
#include "PNGWorker.h"
#include "TLGDecoder.h"
#include "tp_stub.h"
#include <shlwapi.h>
#include <tlhelp32.h>
#include <unordered_set>
#include <tuple>

PVOID GetTVPCreateStreamCall();
tTJSBinaryStream* FASTCALL CallTVPCreateStream(const ttstr& FilePath);
IStream* FASTCALL ConvertBStreamToIStream(tTJSBinaryStream* BStream);
wstring FASTCALL FormatPathFull(LPCWSTR Path);

KrkrUniversalDumper::KrkrUniversalDumper()
{
	hThread = INVALID_HANDLE_VALUE;
}


KrkrUniversalDumper::~KrkrUniversalDumper()
{
}


VOID NTAPI KrkrUniversalDumper::InternalReset()
{
	GlobalData*   Handle;

	Handle = GlobalData::GetGlobalData();
	Handle->Reset();
}


VOID NTAPI KrkrUniversalDumper::AddPath(LPWSTR FileName)
{
	ULONG iPos = 0;

	WCHAR NewPath[MAX_PATH];

	while (FileName[iPos] != NULL)
	{
		if (FileName[iPos] == L'/' || FileName[iPos] == L'\\')
		{
			RtlZeroMemory(NewPath, countof(NewPath) * sizeof(WCHAR));
			RtlCopyMemory(NewPath, FileName, iPos * sizeof(WCHAR));
			lstrcatW(NewPath, L"\\");

			if (!PathIsDirectoryW(NewPath))
				CreateDirectoryW(NewPath, NULL);

			FileName[iPos] = L'\\';
		}
		iPos++;
	}
}

// @keenfuzz
void DumpHex(const void* data, size_t size)
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		PrintConsoleA("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		}
		else {
			ascii[i % 16] = '.';
		}
		if ((i + 1) % 8 == 0 || i + 1 == size) {
			PrintConsoleA(" ");
			if ((i + 1) % 16 == 0) {
				PrintConsoleA("|  %s \n", ascii);
			}
			else if (i + 1 == size) {
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8) {
					PrintConsoleA(" ");
				}
				for (j = (i + 1) % 16; j < 16; ++j) {
					PrintConsoleA("   ");
				}
				PrintConsoleA("|  %s \n", ascii);
			}
		}
	}
}


VOID IdentifyFileName(PVOID Address, SIZE_T Size, std::unordered_set<wstring>& FileList)
{
	SIZE_T Length;
	INT    Count;
	BOOL   Found, Success;
	PWSTR  Name;


	auto BasicValid = [](PWCHAR TestName, SIZE_T Length)
	{
		BOOL Found = FALSE;

		auto isValid = [](unsigned int value)
		{
			const DWORD MaxChar = 0x10ffff;
			const DWORD MinReserved = 0x00d800;
			const DWORD MaxReserved = 0x00dfff;

			return (value <= MaxChar) && ((value < MinReserved) || (value > MaxReserved));
		};

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
			else if (isValid(TestName[i]) == FALSE)
			{
				Found = FALSE;
				break;
			}
		}
		return Found;
	};

	auto FinalValid = [](PWCHAR TestName)
	{
		wstring Test(TestName);
		auto Index = Test.find_first_of(L'>');
		if (Index == wstring::npos)
			return TRUE;
		auto sub = Test.substr(Index + 1);
		auto Found = FALSE;
		for (auto& ch : sub)
		{
			if (ch == L'.')
			{
				Found = TRUE;
				break;
			}
		}
		return Found;
	};

	if (Size <= 4)
		return;

	Success = FALSE;

	LOOP_ONCE
	{
		Name   = (PWCH)((PBYTE)Address);
		Length = wcsnlen_s(Name, Size / 2);
		if (Length < 5)
			break;

		if (BasicValid(Name, Length) && FinalValid(Name))
		{
			wstring FileName(Name, Length);
			FileList.insert(FileName);
			Success = TRUE;
		}
	}

	if (Success == FALSE)
	{
		LOOP_ONCE
		{
			Name   = (PWCH)((PBYTE)Address + 4);
			Length = wcsnlen_s(Name, (Size - 4) / 2);
			if (Length < 5)
				break;

			if (BasicValid(Name, Length) && FinalValid(Name))
			{
				wstring FileName(Name, Length);
				FileList.insert(FileName);
				Success = TRUE;
			}
		}
	}
}

VOID LoadFiles(std::unordered_set<wstring>& FileList)
{
	DWORD              HeapCount;
	HANDLE*            Heaps;
	PROCESS_HEAP_ENTRY Entry;
	

	HeapCount = GetProcessHeaps(0, NULL);
	if (HeapCount == 0)
		return;

	Heaps = (HANDLE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, HeapCount * sizeof(HANDLE));
	GetProcessHeaps(HeapCount, Heaps);

	for (DWORD i = 0; i < HeapCount; i++)
	{
		HeapLock(Heaps[i]);
		Entry.lpData = NULL;
		while (HeapWalk(Heaps[i], &Entry))
		{
			SEH_TRY
			{
				IdentifyFileName((PVOID)Entry.lpData, Entry.cbData, FileList);
			}
			SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
			{

			}
		}
		HeapUnlock(Heaps[i]);
	}
}
enum DumpType : SIZE_T
{
	DUMP_NONE   = 0,
	DUMP_NORMAL = 1,
	DUMP_KRKRZ  = 2
};

std::tuple<wstring, wstring, DumpType> FetchPackageName(const wstring& Name)
{
	auto FixName = [](wstring& FixedName)->wstring
	{
		auto Pos = FixedName.find_last_of(L'.');
		if (Pos != wstring::npos)
			return FixedName.substr(0, Pos);
		return FixedName;
	};

	wstring PackageName = {};
	auto pos = Name.find_first_of(L'>');
	if (pos != wstring::npos)
	{
		PackageName = Name.substr(0, pos);
		auto ExtName = GetExtensionUpper(PackageName);
		if (ExtName == L"XP3" || ExtName == L"BIN")
			return std::make_tuple(FixName(PackageName), Name.substr(pos + 1), DumpType::DUMP_NORMAL);
	}

	pos = Name.find_first_of(L'/');
	if (pos != wstring::npos)
	{
		PackageName = Name.substr(0, pos);
		auto ExtName = GetExtensionUpper(PackageName);
		if (ExtName == L"XP3" || ExtName == L"BIN")
			return std::make_tuple(FixName(PackageName), Name.substr(pos + 1), DumpType::DUMP_KRKRZ);
	}
	
	return std::make_tuple(wstring(L""), wstring(L""), DumpType::DUMP_NONE);
}

NTSTATUS NTAPI KrkrUniversalDumper::DumpFile()
{
	NTSTATUS            Status;
	wstring             FixFileName, ExtName, OutFilePathFull, OutFilePath;
	WCHAR               CurDir[MAX_PATH];
	ULONG_PTR           Index, CurFile;
	tTJSBinaryStream*   BStream;
	IStream*            Stream;
	STATSTG             Stat;
	GlobalData*         Handle;

	Handle = GlobalData::GetGlobalData();

	RtlZeroMemory(CurDir, countof(CurDir) * sizeof(WCHAR));
	GetCurrentDirectoryW(MAX_PATH, CurDir);

	OutFilePath += CurDir;
	OutFilePath += L"\\KrkrExtract_Output\\";

	this->FileList.clear();
	LoadFiles(this->FileList);

	LOOP_ONCE
	{
		CurFile = 0;
		Status  = STATUS_SUCCESS;
		if (FileList.size() == 0)
		{
			Handle->isRunning = FALSE;
			break;
		}

		Handle->CountFile = FileList.size();

		for (auto& FileName : FileList)
		{
			auto NameInfo = FetchPackageName(FileName);
			auto PackageName = std::get<0>(NameInfo);
			auto FilePath    = std::get<1>(NameInfo);
			auto Type        = std::get<2>(NameInfo);

			Handle->SetCurFile(CurFile);

			if (PackageName.length() == 0 || FilePath.length() == 0)
			{
				CurFile++;
				continue;
			}

			Index = FilePath.find_last_of(L'\\');
			if (Index != wstring::npos)
				FixFileName = FilePath.substr(Index + 1, wstring::npos);
			else
				FixFileName = FilePath;
			Index = FixFileName.find_last_of(L'/');
			if (Index != wstring::npos)
				FixFileName = FixFileName.substr(Index + 1, wstring::npos);

			try
			{
				BStream = NULL;
				BStream = CallTVPCreateStream(ttstr(FixFileName.c_str()));

				if (BStream)
				{
					Stream = ConvertBStreamToIStream(BStream);
					ExtName = GetExtensionUpper(FixFileName);
					if (Stream)
					{
						OutFilePathFull = OutFilePath + PackageName + L"\\" + FixFileName;
						AddPath(&OutFilePathFull[0]);
					}


					if (ExtName == L"PNG")
					{
						Status = ProcessPNG(Stream, OutFilePathFull.c_str());
					}
					else if (ExtName == L"TLG")
					{
						Status = ProcessTLG(Stream, OutFilePathFull.c_str());
					}
					else if (ExtName == L"PBD")
					{
						Status = ProcessPBD(Stream, OutFilePathFull.c_str());
					}
					else if (ExtName == L"AMV")
					{
						Status = ProcessAMV(Stream, OutFilePathFull.c_str());
					}
					else if (ExtName == L"PSB" ||
						ExtName == L"SCN" ||
						ExtName == L"MTN" ||
						ExtName == L"PIMG")
					{
						Status = ProcessPSB(Stream, OutFilePathFull.c_str(), ExtName);
					}
					else if (Handle->GetTextFlag() == TEXT_DECODE &&
						(
							ExtName == L"KSD" ||
							ExtName == L"KDT" ||
							ExtName == L"TXT" ||
							ExtName == L"KS" ||
							ExtName == L"CSV" ||
							ExtName == L"PSB" ||
							ExtName == L"FUNC" ||
							ExtName == L"STAND" ||
							ExtName == L"ASD" ||
							ExtName == L"INI" ||
							ExtName == L"TJS"))
					{
						if (ExtName == L"TJS" && Handle->GetTjsFlag() == TJS2_DECOM)
						{
							BYTE           Buffer[8];
							LARGE_INTEGER  Offset;
							ULARGE_INTEGER NewOffset;
							ULONG          Bytes;

							static BYTE TjsMark[8] = { 0x54, 0x4A, 0x53, 0x32, 0x31, 0x30, 0x30, 0x00 };

							Stream->Stat(&Stat, STATFLAG_DEFAULT);
							if (Stat.cbSize.LowPart > 8)
							{
								Stream->Read(Buffer, 8, &Bytes);
							}
							Offset.QuadPart = 0;
							Stream->Seek(Offset, FILE_BEGIN, &NewOffset);

							Status = ProcessTEXT(Stream, OutFilePathFull.c_str());
						}
						else
						{
							Status = ProcessTEXT(Stream, OutFilePathFull.c_str());
						}

					}
					else if (ExtName == L"AMV")
					{
						Status = ProcessFile(Stream, OutFilePathFull.c_str());
					}
					else
					{
						if (ExtName == L"TJS")
						{
							BYTE           Buffer[8];
							LARGE_INTEGER  Offset;
							ULARGE_INTEGER NewOffset;
							ULONG          Bytes;

							static BYTE TjsMark[8] = { 0x54, 0x4A, 0x53, 0x32, 0x31, 0x30, 0x30, 0x00 };

							Stream->Stat(&Stat, STATFLAG_DEFAULT);
							if (Stat.cbSize.LowPart > 8)
							{
								Stream->Read(Buffer, 8, &Bytes);
							}
							Offset.QuadPart = 0;
							Stream->Seek(Offset, FILE_BEGIN, &NewOffset);

							if (Stat.cbSize.LowPart < 8)
							{
								Status = ProcessFile(Stream, OutFilePathFull.c_str());
							}
							else
							{
								Status = ProcessFile(Stream, OutFilePathFull.c_str());
							}
						}
						else
						{
							Status = ProcessFile(Stream, OutFilePathFull.c_str());
						}
					}
				}
			}
			catch (...)
			{

			}
			CurFile++;
		}

	}
	
	Handle->Reset();
	return Status;
}


NTSTATUS NTAPI KrkrUniversalDumper::DoDump()
{
	NTSTATUS     Status;
	GlobalData*  Handle;

	Handle = GlobalData::GetGlobalData();

	Handle->isRunning = TRUE;
	Handle->DisableAll(Handle->MainWindow);
	Status = DumpFile();
	return Status;
}


////////////////////////////////////////////


static KrkrUniversalDumper* LocalKrkrUniversalDumper = NULL;

DWORD WINAPI UniversalExtractThread(LPVOID lParam)
{
	return LocalKrkrUniversalDumper->DoDump();
}

HANDLE NTAPI StartUniversalDumper()
{
	NTSTATUS     Status;
	GlobalData*  Handle;

	Handle = GlobalData::GetGlobalData();

	if (LocalKrkrUniversalDumper == NULL)
		LocalKrkrUniversalDumper = new KrkrUniversalDumper;

	LOOP_ONCE
	{
		LocalKrkrUniversalDumper->InternalReset();
		Status = Nt_CreateThread(UniversalExtractThread, NULL, FALSE, NtCurrentProcess(), &LocalKrkrUniversalDumper->hThread);

		if (NT_FAILED(Status))
		{
			MessageBoxW(NULL, L"Failed to Start Universal Extraction Thread!", L"KrkrExtract", MB_OK);
			break;
		}
	}
	return LocalKrkrUniversalDumper->hThread;
}
