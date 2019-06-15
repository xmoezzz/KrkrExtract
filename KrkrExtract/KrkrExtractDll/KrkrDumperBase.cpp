#include "KrkrDumperBase.h"
#include "KrkrExtract.h"
#include "TLGDecoder.h"
#include "PNGWorker.h"
#include "FakePNG.h"
#include "KrkrExtend.h"
#include "PbdDecode.h"
#include "MyLib.h"




wstring FASTCALL FormatPathFull(LPCWSTR Path)
{
	WCHAR  Buffer[MAX_PATH];

	if (RtlCompareMemory(Path, L"file:", 10) == 10)
		return Path;

	else if (Path[1] == L':' && (Path[0] <= L'Z' && Path[0] >= L'A' || Path[0] <= L'z' && Path[0] >= L'a'))
	{
		wstring Result(L"file://./");
		Result += Path[0];
		for (ULONG i = 2; Path[i] != 0; ++i)
		{
			if (Path[i] == L'\\')
				Result += L'/';
			else
				Result += Path[i];
		}
		return Result;
	}
	else
	{
		BOOL Flag = FALSE;
		for (ULONG i = 0; Path[i] != 0; ++i)
		{
			if (Path[i] == '/' || Path[i] == '\\' || Path[i] == '*')
			{
				Flag = TRUE;
				break;
			}
		}
		if (!Flag)
		{
			RtlZeroMemory(Buffer, countof(Buffer) * sizeof(WCHAR));
			GetCurrentDirectoryW(countof(Buffer), Buffer);
			wsprintfW(Buffer + lstrlenW(Buffer), L"/%s", Path);
			auto&& Result = FormatPathFull(Buffer);
			return Result;
		}
	}
	return L"";
}



NTSTATUS NTAPI KrkrDumperBase::ProcessFile(IStream* Stream, LPCWSTR OutFileName)
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


NTSTATUS NTAPI KrkrDumperBase::ProcessPSB(IStream* Stream, LPCWSTR OutFileName, wstring& ExtName)
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

NTSTATUS NTAPI KrkrDumperBase::ProcessTLG(IStream* Stream, LPCWSTR OutFileName)
{
	NTSTATUS         Status;
	STATSTG          Stat;
	LARGE_INTEGER    Offset;
	ULONG            ReadSize, OutSize;
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

NTSTATUS NTAPI KrkrDumperBase::ProcessPNG(IStream* Stream, LPCWSTR OutFileName)
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



NTSTATUS NTAPI KrkrDumperBase::ProcessPBD(IStream* Stream, LPCWSTR OutFileName)
{
	NTSTATUS     Status;
	WCHAR        FileName[MAX_PATH];

	RtlZeroMemory(FileName, sizeof(FileName));
	lstrcpyW(FileName, OutFileName);
	lstrcatW(FileName, L".json");

	if (GlobalData::GetGlobalData()->GetPbdFlag() == PBD_JSON)
	{
		SavePbd(GetPackageName(wstring(OutFileName)).c_str(), FileName);
	}
	else if (GlobalData::GetGlobalData()->GetPngFlag() == PBD_RAW)
	{
		Status = ProcessFile(Stream, OutFileName);
	}
	return Status;
}


NTSTATUS NTAPI KrkrDumperBase::ProcessTEXT(IStream* Stream, LPCWSTR OutFileName)
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

