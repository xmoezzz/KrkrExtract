#include "WellknownPlugin.h"
#include <my.h>
#include <RpcUIDefine.h>
#include <atomic>
#include <vector>
#include "gif.h"
#include "Helper.h"
#include "Decoder.h"
#include <png.h>
#include "zlib.h"
#include "unzip.h"
#include <Shlobj.h>
#include <shlobj_core.h>
#include <Shlwapi.h>
#include "magic_enum.hpp"
#include "SafeMemory.h"
#include "resource.h"


#pragma comment(lib, "Bcrypt.lib")

CAmvPlugin::CAmvPlugin() :
	m_Registered(FALSE),
	m_Reference(1)
{
}

CAmvPlugin::~CAmvPlugin()
{
	m_Proxyer = nullptr;
}

HRESULT STDMETHODCALLTYPE CAmvPlugin::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	if (!ppvObject)
		return E_INVALIDARG;

	*ppvObject = NULL;
	if (riid == IID_IUnknown)
	{
		*ppvObject = (LPVOID)this;
		AddRef();
		return NOERROR;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAmvPlugin::AddRef()
{
	m_Reference++;
	return m_Reference;
}

ULONG STDMETHODCALLTYPE CAmvPlugin::Release()
{
	m_Reference--;
	if (m_Reference == 0)
	{
		delete this;
	}

	return m_Reference;
}

HRESULT STDMETHODCALLTYPE CAmvPlugin::Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count)
{
	if (!Extensions || !Count || Proxyer)
		return E_INVALIDARG;

	static PCWSTR ExtensionList[] = 
	{
		L"AMV",
		NULL
	};
	
	m_Handle  = Handle;
	m_Proxyer = Proxyer;
	*Extensions = ExtensionList;
	*Count = _countof(ExtensionList) - 1;
	return NOERROR;
}



//video
std::vector<std::wstring> GetPNGFiles(std::wstring BasePath)
{
	HANDLE               hFile;
	std::wstring         Pattern;
	WIN32_FIND_DATAW     Info;
	std::vector<std::wstring> FileList;


	Pattern = BasePath + L"\\*.png";
	hFile = FindFirstFileW(Pattern.c_str(), &Info);
	if (hFile == INVALID_HANDLE_VALUE)
		return FileList;

	do
	{
		if (Info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		auto FileName = BasePath + L"\\";
		FileName += Info.cFileName;
		FileList.push_back(FileName);

	} while (FindNextFileW(hFile, &Info));

	FindClose(hFile);
	return FileList;
}


NTSTATUS ProcessAmvToPNG(std::vector<std::wstring>& FileList, std::wstring DestDir)
{
	BOOL          Success;
	std::wstring  NewPath;

	for (auto& FileName : FileList)
	{
		NewPath = DestDir + L"\\" + GetPackageName(FileName);
		Success = Io::CopyFileW(FileName.c_str(), NewPath.c_str(), FALSE);
	}

	return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


NTSTATUS NTAPI ProcessAmvToJPG(std::vector<std::wstring>& FileList, std::wstring DestDir)
{
	NTSTATUS Status;

	for (auto& FileName : FileList)
	{
		NtFileDisk      File;
		ULONG_PTR       Size;
		std::wstring    NewPath;

		Status = File.Open(FileName.c_str());
		if (NT_FAILED(Status))
			continue;

		Size = File.GetSize32();
		if (Size == 0)
		{
			File.Close();
			continue;
		}

		auto Buffer = AllocateMemorySafeP<BYTE>(Size);
		if (!Buffer)
		{
			File.Close();
			continue;
		}

		Status = File.Read(Buffer.get(), Size);
		if (NT_FAILED(Status))
		{
			File.Close();
			continue;
		}

		NewPath = ReplaceFileNameExtension(DestDir + L"\\" + GetPackageName(FileName), L".jpg");
		Image2JPG(Buffer.get(), Size, NewPath.c_str());
		File.Close();
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

	DWORD width = png_get_image_width(png, info);
	DWORD height = png_get_image_height(png, info);
	BYTE  color_type = png_get_color_type(png, info);
	BYTE  bit_depth = png_get_bit_depth(png, info);

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
	for (DWORD y = 0; y < height; y++) {
		row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
	}

	png_read_image(png, row_pointers);
	fclose(fp);

	PBYTE Data = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, width * height * 4);
	for (DWORD y = 0; y < height; y++) {
		RtlCopyMemory(Data + y * width * 4, row_pointers[y], png_get_rowbytes(png, info));
		free(row_pointers[y]);
		row_pointers[y] = nullptr;
	}

	free(row_pointers);
	row_pointers = nullptr;
	png_destroy_read_struct(&png, &info, NULL);

	return Data;
}

NTSTATUS ProcessAmvToGIF(std::vector<std::wstring>& FileList, std::wstring OriName, std::wstring DestDir)
{
	NTSTATUS       Status;
	std::wstring   NewPath;
	BOOL           IsFirst;
	GifWriter      Gif;
	DWORD          Width, Height;
	PBYTE          Data;

	IsFirst = TRUE;
	Width = 0;
	Height = 0;
	Data = NULL;
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


INT RemoveDirectoryR(std::wstring Dir)
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


NTSTATUS CAmvPlugin::CleanAmvExtractor()
{
	BOOL Success;
	INT  State;

	Success = TRUE;
	State = RemoveDirectoryR(m_BasePath);
	RemoveDirectoryW(m_BasePath.c_str());

	Success = State == 0;
	m_BasePath = {};
	return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


NTSTATUS CAmvPlugin::CreateAmvExtractor()
{
	NTSTATUS         Status;
	HRESULT          hr;
	IStream*         Stream;
	STATSTG          Stat;
	DWORD            Size;
	ULONG            BytesRead;
	WCHAR            CurrentDir[MAX_PATH];
	wstring          ExtractBasePath, Hex;
	HZIP             hZip;
	ZIPENTRY         ze;
	INT              Count;
	DWORD            Attribute;
	BYTE            GBuffer[128];

	Stream = LoadFromResource(m_Proxyer->GetSelfModule(), IDR_ZIP1, L"ZIP", WALK_RESOURCE_BUFFER_DUMMY);
	if (!Stream)
		return STATUS_UNSUCCESSFUL;

	Stream->Stat(&Stat, STATFLAG_DEFAULT);
	Size = Stat.cbSize.LowPart;
	auto Buffer = AllocateMemorySafeP<BYTE>(Size);
	if (!Buffer)
	{
		Stream->Release();
		return STATUS_NO_MEMORY;
	}

	hr = Stream->Read(Buffer.get(), Size, &BytesRead);
	if (FAILED(hr))
	{
		Stream->Release();
		return STATUS_UNSUCCESSFUL;
	}

	Stream->Release();

	RtlZeroMemory(CurrentDir, sizeof(CurrentDir));
	GetCurrentDirectoryW(countof(CurrentDir) - 1, CurrentDir);

	hZip = OpenZip(Buffer.get(), Size, NULL);
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
		return STATUS_UNSUCCESSFUL;

	if (ERROR_SUCCESS != SHCreateDirectory(NULL, ExtractBasePath.c_str()))
		return STATUS_UNSUCCESSFUL;

	m_BasePath = ExtractBasePath;
	SetUnzipBaseDir(hZip, ExtractBasePath.c_str());
	GetZipItem(hZip, -1, &ze);
	Count = ze.index;
	for (int zi = 0; zi < Count; zi++)
	{
		GetZipItem(hZip, zi, &ze);
		UnzipItem(hZip, zi, ze.name);
	}
	CloseZip(hZip);
	return STATUS_SUCCESS;
}


HRESULT STDMETHODCALLTYPE CAmvPlugin::Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream)
{
	NTSTATUS            Status;
	INT                 State;
	BOOL                Success;
	WCHAR               CmdLine[1000];
	wstring             Name, BaseName, OutPath, RemovedDir;
	STARTUPINFOW        StartupInfo;
	PROCESS_INFORMATION ProcessInfo;
	DWORD               WaitStatus;
	DWORD               ExitCode;
	vector<wstring>     FileList;
	KrkrAmvMode         AmvMode;

	//
	// chromium style
	//

	if (KrkrAmvMode::AMV_RAW < KrkrAmvMode::AMV_JPG)
		return E_FAIL;

	AmvMode = (KrkrAmvMode)m_Proxyer->GetAmvFlag();

	switch (AmvMode)
	{
	case KrkrAmvMode::AMV_JPG:
	case KrkrAmvMode::AMV_PNG:
	case KrkrAmvMode::AMV_GIF:
		break;

	case KrkrAmvMode::AMV_RAW:
		return E_ABORT;

	default:
		m_Proxyer->TellServerLogOutput(
			LogLevel::LOG_WARN,
			L"CAmvPlugin::Unpack : Unknown flag = %d",
			AmvMode
		);

		return E_FAIL;
	}

	Status = CreateAmvExtractor();
	if (NT_FAILED(Status))
		return NtStatusToHResult(Status);

	RtlZeroMemory(CmdLine, sizeof(CmdLine));
	BaseName = GetPackageName(wstring(FilePath));
	Name = m_BasePath + L"\\video\\" + BaseName;
	Success = Io::CopyFileW(FilePath, Name.c_str(), FALSE);
	if (!Success)
		return STATUS_NO_SUCH_FILE;

	wsprintfW(CmdLine,
		L"\"%s\\\\%s\" \"-amvpath=%s\"",
		m_BasePath.c_str(),
		L"AlphaMovieDecoderFake.exe",
		BaseName.c_str());

	RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
	StartupInfo.cb = sizeof(StartupInfo);
	RtlZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

	Success = m_Proxyer->CreateProcessBypass(
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
	{
		ExitCode = GetLastError();
		if (ExitCode != NO_ERROR)
			return HRESULT_FROM_WIN32(ExitCode);

		return E_FAIL;
	}

	ExitCode = 0;
	WaitStatus = WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
	Success = GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
	if (!Success || ExitCode == STILL_ACTIVE)
	{
		Status = NtTerminateProcess(ProcessInfo.hProcess, 0);
	}

	CloseHandle(ProcessInfo.hProcess);
	CloseHandle(ProcessInfo.hThread);

	Io::DeleteFileW(Name.c_str());
	FileList = GetPNGFiles(m_BasePath + L"\\video");
	if (!FileList.size())
	{
		CleanAmvExtractor();
		return E_FAIL;
	}

	OutPath = ReplaceFileNameExtension(wstring(FilePath), L"");
	LOOP_ONCE
	{
		if (GetFileAttributesW(OutPath.c_str()) != INVALID_FILE_ATTRIBUTES)
			break;

		State = SHCreateDirectory(NULL, OutPath.c_str());
		if (State != ERROR_SUCCESS)
		{
			CleanAmvExtractor();
			return E_FAIL;
		}
	}

	switch (AmvMode)
	{
	case KrkrAmvMode::AMV_JPG:
		Status = ProcessAmvToJPG(FileList, OutPath);
		break;

	case KrkrAmvMode::AMV_PNG:
		Status = ProcessAmvToPNG(FileList, OutPath);
		break;

	case KrkrAmvMode::AMV_GIF:
		Status = ProcessAmvToGIF(FileList, GetPackageName(wstring(FilePath)), OutPath);
		break;
	}

	CleanAmvExtractor();
	return NtStatusToHResult(Status);
}

