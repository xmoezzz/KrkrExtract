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

NTSTATUS NTAPI KrkrUniversalDumper::ProcessFile(IStream* Stream, LPCWSTR OutFileName)
{
	NTSTATUS         Status;
	STATSTG          Stat;
	NtFileDisk       File;
	LARGE_INTEGER    Tranferred, WriteSize, TempSize, Offset;
	ULONG            ReadSize;

	static BYTE Buffer[1024 * 64];

	Offset.QuadPart = 0;
	Stream->Seek(Offset, FILE_BEGIN, NULL);
	Stream->Stat(&Stat, STATFLAG_DEFAULT);
	Tranferred.QuadPart = 0;

	Status = File.Create(OutFileName);
	if (NT_FAILED(Status))
	{
		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"Failed to write %s[%08x]\n", OutFileName, Status);

		return Status;
	}

	while (Tranferred.QuadPart < (LONG64)Stat.cbSize.QuadPart)
	{
		Stream->Read(&Buffer, sizeof(Buffer), &ReadSize);
		Tranferred.QuadPart += ReadSize;
		TempSize.QuadPart = 0;
		while (TempSize.QuadPart < ReadSize)
		{
			File.Write(Buffer, ReadSize, &WriteSize);
			TempSize.QuadPart += WriteSize.QuadPart;
		}
	}
	File.Close();
	return Status;
}


NTSTATUS NTAPI KrkrUniversalDumper::ProcessPSB(IStream* Stream, LPCWSTR OutFileName, wstring& ExtName)
{
	NTSTATUS    Status;
	NtFileDisk  File;
	STATSTG     Stat;
	GlobalData* Handle;

	Handle = GlobalData::GetGlobalData();
	Stream->Stat(&Stat, STATFLAG_DEFAULT);

	Status = STATUS_ABANDONED;
	if (Handle->PsbFlagOn(PSB_RAW) ||
		Handle->PsbFlagOn(PSB_ALL))
	{
		Status = ProcessFile(Stream, OutFileName);
	}

	if (Handle->PsbFlagOn(PSB_ALL) ||
		Handle->PsbFlagOn(PSB_TEXT) ||
		Handle->PsbFlagOn(PSB_DECOM) ||
		Handle->PsbFlagOn(PSB_ANM))
	{
		Status = ExtractPsb(Stream, GlobalData::GetGlobalData()->PsbFlagOn(PSB_IMAGE) ||
			GlobalData::GetGlobalData()->PsbFlagOn(PSB_ANM),
			GlobalData::GetGlobalData()->PsbFlagOn(PSB_DECOM),
			ExtName, OutFileName);

		Status = Status >= 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	}

	if (Handle->PsbFlagOn(PSB_ALL) ||
		Handle->PsbFlagOn(PSB_JSON)
		)
	{
		wstring FileName = ReplaceFileNameExtension(GetPackageName(wstring(OutFileName)), L"");
		wstring BaseName = GetFileBasePath(wstring(OutFileName));

		PrintConsoleW(L"%s %s\n", FileName.c_str(), BaseName.c_str());

		Status = DecompilePsbJson(Stream, BaseName.c_str(), FileName.c_str());
	}

	return Status;
}

NTSTATUS NTAPI KrkrUniversalDumper::ProcessTLG(IStream* Stream, LPCWSTR OutFileName)
{
	NTSTATUS         Status;
	STATSTG          Stat;
	LARGE_INTEGER    Offset;
	ULONG            ReadSize;
	ULONG            OutSize;
	PBYTE            Buffer, RawBuffer, OutBuffer;
	BOOL             TempDecode;
	NtFileDisk       File;
	GlobalData*      Handle;

	Handle = GlobalData::GetGlobalData();

	Stream->Stat(&Stat, STATFLAG_DEFAULT);

	Buffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Stat.cbSize.LowPart);
	if (!Buffer)
	{
		MessageBoxW(Handle->MainWindow, L"Failed to Allocate memory for tlg Decoder", L"KrkrExtract", MB_OK);
		return STATUS_UNSUCCESSFUL;
	}

	Status = STATUS_SUCCESS;
	Stream->Read(Buffer, Stat.cbSize.LowPart, &ReadSize);
	RawBuffer = Buffer;

	switch (Handle->GetTlgFlag())
	{
	case TLG_RAW:
		Offset.QuadPart = 0;
		Stream->Seek(Offset, FILE_BEGIN, NULL);
		HeapFree(GetProcessHeap(), 0, RawBuffer);
		return ProcessFile(Stream, OutFileName);

	case TLG_SYS:
		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"Using System Decode Mode[TLG]\n");

		HeapFree(GetProcessHeap(), 0, RawBuffer);
		SavePng(GetPackageName(wstring(OutFileName)).c_str(), FormatPathFull((wstring(OutFileName) + L".png").c_str()).c_str());
		return Status;


	case TLG_BUILDIN:
	case TLG_PNG:
	{

		OutBuffer = NULL;
		OutSize = 0;
		TempDecode = TRUE;

		if (RtlCompareMemory(Buffer, KRKR2_TLG0_MAGIC, 6) == 6)
			Buffer += 0xF;

		if (RtlCompareMemory(Buffer, KRKR2_TLG5_MAGIC, 6) == 6)
		{
			if (!DecodeTLG5(Buffer, Stat.cbSize.LowPart, (PVOID*)&OutBuffer, &OutSize))
			{
				TempDecode = FALSE;
				OutSize = Stat.cbSize.LowPart;
				OutBuffer = Buffer;
			}
		}
		else if (RtlCompareMemory(Buffer, KRKR2_TLG6_MAGIC, 6) == 6)
		{
			if (!DecodeTLG6(Buffer, Stat.cbSize.LowPart, (PVOID*)&OutBuffer, &OutSize))
			{
				TempDecode = FALSE;
				OutSize = Stat.cbSize.LowPart;
				OutBuffer = Buffer;
			}
		}
		else
		{
			TempDecode = FALSE;
			OutSize = Stat.cbSize.LowPart;
			OutBuffer = Buffer;
		}

		if (Handle->GetTlgFlag() == TLG_BUILDIN)
		{
			if (Handle->DebugOn)
				PrintConsoleW(L"Decoding tlg image to bmp...\n");

			if (!TempDecode)
				Status = File.Create(OutFileName);
			else
				Status = File.Create((wstring(OutFileName) + L".bmp").c_str());

			if (NT_FAILED(Status))
			{
				HeapFree(GetProcessHeap(), 0, RawBuffer);
				if (TempDecode)
					HeapFree(GetProcessHeap(), 0, OutBuffer);

				TempDecode = NULL;
				File.Close();
				return STATUS_UNSUCCESSFUL;
			}

			File.Write(OutBuffer, OutSize);

			HeapFree(GetProcessHeap(), 0, RawBuffer);
			if (TempDecode)
				HeapFree(GetProcessHeap(), 0, OutBuffer);
		}
		else if (Handle->GetTlgFlag() == TLG_PNG)
		{
			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleW(L"Decoding tlg image to png file(Build-in)...\n");

			if (TempDecode)
			{
				if (Bmp2PNG(OutBuffer, OutSize, (wstring(OutFileName) + L".png").c_str()) < 0)
				{
					Status = File.Create((wstring(OutFileName) + L".bmp").c_str());

					if (NT_FAILED(Status))
						return Status;

					File.Write(OutBuffer, OutSize);
					File.Close();
				}

				HeapFree(GetProcessHeap(), 0, RawBuffer);
				HeapFree(GetProcessHeap(), 0, OutBuffer);
			}
			else
			{
				HeapFree(GetProcessHeap(), 0, RawBuffer);
				Offset.QuadPart = 0;
				Stream->Seek(Offset, FILE_BEGIN, NULL);
				return ProcessFile(Stream, OutFileName);
			}
		}
		else if (Handle->GetTlgFlag() == TLG_JPG)
		{
			if (GlobalData::GetGlobalData()->DebugOn)
				PrintConsoleW(L"Decoding tlg image to jpg file(Build-in)...\n");

			if (TempDecode)
			{
				if (Bmp2JPG(OutBuffer, OutSize, (wstring(OutFileName) + L".jpg").c_str()) < 0)
				{
					Status = File.Create((wstring(OutFileName) + L".bmp").c_str());

					if (NT_FAILED(Status))
						return Status;

					File.Write(OutBuffer, OutSize);
					File.Close();
				}

				HeapFree(GetProcessHeap(), 0, RawBuffer);
				HeapFree(GetProcessHeap(), 0, OutBuffer);
			}
			else
			{
				HeapFree(GetProcessHeap(), 0, RawBuffer);
				Offset.QuadPart = 0;
				Stream->Seek(Offset, FILE_BEGIN, NULL);
				return ProcessFile(Stream, OutFileName);
			}
		}
	}
	break;
	}
	return Status;
}

NTSTATUS NTAPI KrkrUniversalDumper::ProcessPNG(IStream* Stream, LPCWSTR OutFileName)
{
	NTSTATUS         Status;
	NtFileDisk       File;

	if (GlobalData::GetGlobalData()->GetPngFlag() == PNG_SYS)
	{
		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"Using Build-in Decode Mode\n");

		SavePng(GetPackageName(wstring(OutFileName)).c_str(), FormatPathFull(OutFileName).c_str());
	}
	else if (GlobalData::GetGlobalData()->GetPngFlag() == PNG_RAW)
	{
		Status = ProcessFile(Stream, OutFileName);
	}
	return Status;
}



NTSTATUS NTAPI KrkrUniversalDumper::ProcessTEXT(IStream* Stream, LPCWSTR OutFileName)
{
	NTSTATUS     Status;
	STATSTG      Stat;
	ULONG        ReadSize;
	PBYTE        OriBuffer;
	WCHAR        FileName[MAX_PATH];

	Stream->Stat(&Stat, STATFLAG_DEFAULT);
	RtlZeroMemory(FileName, countof(FileName) * sizeof(WCHAR));
	lstrcpyW(FileName, OutFileName);

	LOOP_ONCE
	{
		Status = STATUS_SUCCESS;
	OriBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Stat.cbSize.LowPart);
	if (!OriBuffer)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		break;
	}

	Stream->Read(OriBuffer, Stat.cbSize.LowPart, &ReadSize);

	Status = STATUS_SUCCESS;
	if (DecodeText(OriBuffer, Stat.cbSize.LowPart, FileName) != -1)
		HeapFree(GetProcessHeap(), 0, OriBuffer);
	else
		Status = ProcessFile(Stream, FileName);
	}
	return Status;
}

NTSTATUS NTAPI KrkrUniversalDumper::DumpFile()
{
	NTSTATUS            Status;
	wstring             FixFileName, ExtName, OutFilePathFull, OutFilePath;
	WCHAR               CurDir[MAX_PATH];
	ULONG_PTR           Index;
	tTJSBinaryStream*   BStream;
	IStream*            Stream;
	STATSTG             Stat;
	GlobalData*         Handle;

	Handle = GlobalData::GetGlobalData();

	RtlZeroMemory(CurDir, countof(CurDir) * sizeof(WCHAR));
	GetCurrentDirectoryW(MAX_PATH, CurDir);

	OutFilePath += CurDir;
	OutFilePath += L"\\KrkrExtract_Output\\";

	LOOP_ONCE
	{
		Status = STATUS_SUCCESS;
		if (Handle->FileNameList.size() == 0)
		{
			Handle->isRunning = FALSE;
			break;
		}

		for (auto& FileName : Handle->FileNameList)
		{
			Index = FileName.find_last_of(L'\\');
			if (Index != wstring::npos)
				FixFileName = FileName.substr(Index + 1, wstring::npos);
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
						OutFilePathFull = OutFilePath + FixFileName;
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
