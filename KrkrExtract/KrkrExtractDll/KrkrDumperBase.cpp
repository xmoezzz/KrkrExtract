#include "KrkrDumperBase.h"
#include "KrkrExtract.h"
#include "TLGDecoder.h"
#include "PNGWorker.h"
#include "FakePNG.h"
#include "KrkrExtend.h"
#include "PbdDecode.h"
#include "MyLib.h"
#include "unzip.h"
#include "resource.h"
#include "md5.h"
#include "PNGWorker.h"
#include "gif.h"
#include <bcrypt.h>
#include <Shlobj.h>
#include <shlobj_core.h>
#include <Shlwapi.h>
#include "libpng\png.h"

#pragma comment(lib, "libpng16.lib")
#pragma comment(lib, "Bcrypt.lib")

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


//video
std::vector<std::wstring> GetPNGFiles(std::wstring BasePath)
{
	HANDLE               hFile;
	wstring              Pattern;
	WIN32_FIND_DATAW     Info;
	std::vector<wstring> FileList;
	

	Pattern = BasePath + L"\\*.png";
	hFile = FindFirstFileW(Pattern.c_str(), &Info);
	if (hFile == INVALID_HANDLE_VALUE)
		return FileList;

	do
	{
		if (Info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		
		wstring FileName = BasePath + L"\\";
		FileName += Info.cFileName;
		FileList.push_back(FileName);

	} while (FindNextFileW(hFile, &Info));

	FindClose(hFile);
	return FileList;
}



NTSTATUS NTAPI KrkrDumperBase::ProcessAmvToPNG(vector<wstring>& FileList, wstring DestDir)
{
	BOOL     Success;
	wstring  NewPath;
	
	for (auto& FileName : FileList)
	{
		NewPath = DestDir + L"\\" + GetPackageName(FileName);
		Success = CopyFileW(FileName.c_str(), NewPath.c_str(), FALSE);
	}
	
	return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


NTSTATUS NTAPI KrkrDumperBase::ProcessAmvToJPG(vector<wstring>& FileList, wstring DestDir)
{
	NTSTATUS Status;

	for (auto& FileName : FileList)
	{
		NtFileDisk File;
		PBYTE      Buffer;
		ULONG_PTR  Size;
		wstring    NewPath;

		Status = File.Open(FileName.c_str());
		if (NT_FAILED(Status))
			continue;
		
		Size = File.GetSize32();
		if (Size == 0)
		{
			File.Close();
			continue;
		}

		Buffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
		if (!Buffer)
		{
			File.Close();
			continue;
		}

		Status = File.Read(Buffer, Size);
		if (NT_FAILED(Status))
		{
			HeapFree(GetProcessHeap(), 0, Buffer);
			Buffer = NULL;
			File.Close();
			continue;
		}

		NewPath = ReplaceFileNameExtension(DestDir + L"\\" + GetPackageName(FileName), L".jpg");
		Image2JPG(Buffer, Size, NewPath.c_str());
		File.Close();
		HeapFree(GetProcessHeap(), 0, Buffer);
		Buffer = NULL;
	}
	return STATUS_SUCCESS;
}


PBYTE ReadPngFile(const WCHAR* FileName, DWORD& PngWidth, DWORD& PngHeight)
{
	FILE* fp;

	fp = NULL;
	_wfopen_s(&fp, FileName, L"rb");
	if (!fp)
		return nullptr;

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
		return nullptr;

	png_infop info = png_create_info_struct(png);
	if (!info)
		return nullptr;

	if (setjmp(png_jmpbuf(png)))
		return nullptr;

	png_init_io(png, fp);

	png_read_info(png, info);

	DWORD width  = png_get_image_width(png, info);
	DWORD height = png_get_image_height(png, info);
	BYTE  color_type = png_get_color_type(png, info);
	BYTE  bit_depth  = png_get_bit_depth(png, info);

	if (bit_depth == 16)
		png_set_strip_16(png);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

	// PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png);

	if (png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	// These color_type don't have an alpha channel then fill it with 0xff.
	if (color_type == PNG_COLOR_TYPE_RGB ||
		color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	png_read_update_info(png, info);

	png_bytepp row_pointers = nullptr;

	row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for (int y = 0; y < height; y++) {
		row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
	}

	png_read_image(png, row_pointers);
	fclose(fp);

	PBYTE Data = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, width * height * 4);
	for (int y = 0; y < height; y++) {
		RtlCopyMemory(Data + y * width * 4, row_pointers[y], png_get_rowbytes(png, info));
		free(row_pointers[y]);
		row_pointers[y] = nullptr;
	}

	free(row_pointers);
	row_pointers = nullptr;
	png_destroy_read_struct(&png, &info, NULL);

	return Data;
}

NTSTATUS NTAPI KrkrDumperBase::ProcessAmvToGIF(vector<wstring>& FileList, wstring OriName, wstring DestDir)
{
	NTSTATUS  Status;
	wstring   NewPath;
	BOOL      IsFirst;
	GifWriter Gif;
	DWORD     Width, Height;
	PBYTE     Data;

	IsFirst = TRUE;
	Width  = 0;
	Height = 0;
	Data   = NULL;
	NewPath = ReplaceFileNameExtension(DestDir + L"\\" + GetPackageName(OriName), L".gif");
	for (auto& FileName : FileList)
	{
		Data = ReadPngFile(FileName.c_str(), Width, Height);
		if (!Data)
			continue;

		if (IsFirst)
		{
			GifBegin(&Gif, NewPath.c_str(), Width, Height, 3);
			IsFirst = FALSE;
		}
		GifWriteFrame(&Gif, Data, Width, Height, 3);
		HeapFree(GetProcessHeap(), 0, Data);
		Data = NULL;
	}

	GifEnd(&Gif);
	return STATUS_SUCCESS;
}


int RemoveDirectoryR(wstring Dir)
{
	std::vector<std::wstring::value_type> doubleNullTerminatedPath;
	std::copy(Dir.begin(), Dir.end(), std::back_inserter(doubleNullTerminatedPath));
	doubleNullTerminatedPath.push_back(L'\0');
	doubleNullTerminatedPath.push_back(L'\0');

	SHFILEOPSTRUCTW fileOperation;
	fileOperation.wFunc = FO_DELETE;
	fileOperation.pFrom = &doubleNullTerminatedPath[0];
	fileOperation.fFlags = FOF_NO_UI | FOF_NOCONFIRMATION | FOF_SILENT;

	return ::SHFileOperationW(&fileOperation);
}


NTSTATUS NTAPI KrkrDumperBase::ProcessAMV(IStream* Stream, LPCWSTR OutFileName)
{
	NTSTATUS            Status;
	INT                 State;
	BOOL                Success;
	GlobalData*         Handle;
	WCHAR               CmdLine[1000];
	wstring             Name, BaseName, OutPath, RemovedDir;
	STARTUPINFOW        StartupInfo;
	PROCESS_INFORMATION ProcessInfo;
	DWORD               WaitStatus;
	DWORD               ExitCode;
	vector<wstring>     FileList;
	
	Handle = GlobalData::GetGlobalData();
	Status = ProcessFile(Stream, OutFileName);
	if (NT_FAILED(Status))
		return Status;

	if (Handle->GetAmvFlag() == AMV_RAW)
		return Status;

	Status = CreateAmvExtractor();
	if (NT_FAILED(Status))
		return Status;

	RtlZeroMemory(CmdLine, sizeof(CmdLine));
	BaseName = GetPackageName(wstring(OutFileName));
	Name = BasePath + L"\\video\\" + BaseName;
	Success = CopyFileW(OutFileName, Name.c_str(), FALSE);
	if (!Success)
		return STATUS_NO_SUCH_FILE;

	wsprintfW(CmdLine, 
		L"\"%s\\\\%s\" \"-amvpath=%s\"", 
		BasePath.c_str(), 
		L"AlphaMovieDecoderFake.exe", 
		BaseName.c_str());

	RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
	StartupInfo.cb = sizeof(StartupInfo);
	RtlZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

	Success = Handle->StubCreateProcessInternalW(
		NULL,
		NULL,
		CmdLine,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&StartupInfo,
		&ProcessInfo,
		NULL
	);

	if (!Success)
		return STATUS_UNSUCCESSFUL;

	ExitCode = 0;
	WaitStatus = WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
	Success = GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
	if (!Success || ExitCode == STILL_ACTIVE)
	{
		Status = NtTerminateProcess(ProcessInfo.hProcess, 0);
	}

	CloseHandle(ProcessInfo.hProcess);
	CloseHandle(ProcessInfo.hThread);

	DeleteFileW(Name.c_str());
	FileList = GetPNGFiles(BasePath + L"\\video");
	if (!FileList.size())
	{
		CleanAmvExtractor();
		return STATUS_UNSUCCESSFUL;
	}

	OutPath = ReplaceFileNameExtension(wstring(OutFileName), L"");
	LOOP_ONCE
	{
		if (GetFileAttributesW(OutPath.c_str()) != INVALID_FILE_ATTRIBUTES)
			break;

		State = SHCreateDirectory(NULL, OutPath.c_str());
		if (State != ERROR_SUCCESS)
		{
			CleanAmvExtractor();
			return STATUS_UNSUCCESSFUL;
		}
	}

	switch (Handle->GetAmvFlag())
	{
	case AMV_DECODE_INFO::AMV_JPG:
		Status = ProcessAmvToJPG(FileList, OutPath);
		break;

	case AMV_DECODE_INFO::AMV_PNG:
		Status = ProcessAmvToPNG(FileList, OutPath);
		break;
	
	case AMV_DECODE_INFO::AMV_GIF:
		Status = ProcessAmvToGIF(FileList, GetPackageName(wstring(OutFileName)), OutPath);
		break;
	}

	CleanAmvExtractor();
	return Status;
}


IStream* LoadFromResource2(HMODULE Module, UINT nResID, LPCWSTR lpTyp);

NTSTATUS NTAPI KrkrDumperBase::CreateAmvExtractor()
{
	NTSTATUS    Status;
	HRESULT     hr;
	IStream*    Stream;
	GlobalData* Handle;
	STATSTG     Stat;
	DWORD       Size;
	ULONG       BytesRead;
	PBYTE       Buffer;
	WCHAR       CurrentDir[MAX_PATH];
	wstring     ExtractBasePath, Hex;
	HZIP        hZip;
	ZIPENTRY    ze;
	INT         Count;
	DWORD       Attribute;
	BYTE        GBuffer[128];

	Handle = GlobalData::GetGlobalData();
	Stream = LoadFromResource2(Handle->hSelfModule, IDR_ZIP1, L"ZIP");
	if (!Stream)
		return STATUS_UNSUCCESSFUL;

	Stream->Stat(&Stat, STATFLAG_DEFAULT);
	Size = Stat.cbSize.LowPart;
	Buffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
	if (!Buffer)
	{
		Stream->Release();
		return STATUS_NO_MEMORY;
	}

	hr = Stream->Read(Buffer, Size, &BytesRead);
	if (FAILED(hr)) 
	{
		Stream->Release();
		return STATUS_UNSUCCESSFUL;
	}


	Stream->Release();

	RtlZeroMemory(CurrentDir, sizeof(CurrentDir));
	GetCurrentDirectoryW(countof(CurrentDir) - 1, CurrentDir);
	

	hZip = OpenZip(Buffer, Size, NULL);
	ExtractBasePath = CurrentDir;
	ExtractBasePath += L"\\AMV_";
	RtlZeroMemory(GBuffer, sizeof(GBuffer));
	Status = BCryptGenRandom(
		NULL,
		GBuffer,
		sizeof(GBuffer),
		BCRYPT_USE_SYSTEM_PREFERRED_RNG);

	GenMD5Code(GBuffer, sizeof(GBuffer), Hex);
	ExtractBasePath += Hex;

	Attribute = GetFileAttributesW(ExtractBasePath.c_str());
	if (Attribute != INVALID_FILE_ATTRIBUTES) 
	{
		HeapFree(GetProcessHeap(), 0, Buffer);
		return STATUS_UNSUCCESSFUL;
	}

	if (ERROR_SUCCESS != SHCreateDirectory(NULL, ExtractBasePath.c_str()))
	{
		HeapFree(GetProcessHeap(), 0, Buffer);
		return STATUS_UNSUCCESSFUL;
	}
	
	BasePath = ExtractBasePath;
	SetUnzipBaseDir(hZip, ExtractBasePath.c_str());
	GetZipItem(hZip, -1, &ze);
	Count = ze.index;
	for (int zi = 0; zi < Count; zi++)
	{
		GetZipItem(hZip, zi, &ze);
		UnzipItem(hZip, zi, ze.name);
	}
	CloseZip(hZip);
	HeapFree(GetProcessHeap(), 0, Buffer);
	return STATUS_SUCCESS;
}


NTSTATUS NTAPI KrkrDumperBase::CleanAmvExtractor()
{
	BOOL Success;
	INT  State;

	Success = TRUE;
	State   = RemoveDirectoryR(BasePath);
	RemoveDirectoryW(BasePath.c_str());
	
	Success = State == 0;
	BasePath  = {};
	return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

