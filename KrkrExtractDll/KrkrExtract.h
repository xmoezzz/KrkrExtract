#pragma once

#include "tp_stub.h"
#include "my.h"
#include "KrkrHeaders.h"


enum INIT_TYPE
{
	InitedByExeModule,
	InitedByDllModule
};

enum PNG_DECODE_INFO
{
	PNG_RAW,
	PNG_SYS,
	PNG_ALL
};

enum TEXT_DECODE_INFO
{
	TEXT_RAW,
	TEXT_DECODE,
	TEXT_ALL
};

enum PSB_DECODE_INFO
{
	PSB_RAW   = 1UL,
	PSB_TEXT  = 1UL << 1,
	PSB_DECOM = 1UL << 2,
	PSB_IMAGE = 1UL << 3,
	PSB_ANM   = 1UL << 4,
	PSB_ALL   = 0xFFFFFFFFUL
};

enum TLG_DECODE_INFO
{
	TLG_RAW,
	TLG_BUILDIN,
	TLG_SYS,
	TLG_PNG
};

enum ModuleVersion
{
	Krkr2,
	Krkrz, //M2
	Krkr3  //看了看svn上的进度和开坑时间，简直有生之年
};

typedef HRESULT(_stdcall *tV2Link)(iTVPFunctionExporter *);
typedef void(__stdcall * XP3Filter)(tTVPXP3ArchiveExtractionFilter);
typedef iTVPFunctionExporter*   (__stdcall *TVPGetFunctionExporterFunc)();
typedef VOID(WINAPI *TVPXP3ArchiveExtractionFilterFunc)(tTVPXP3ExtractionFilterInfo *info);
typedef tTJSBinaryStream* (FASTCALL * FuncCreateStream)(const ttstr &, tjs_uint32);
typedef PVOID (CDECL * FuncHostAlloc)(ULONG);

#define szApplicationName   L"[X'moe]Welcome to KrkrExtract(version : %s, built on : %s)"
#define szWindowClassName   L"XP3ExtractMainClass"
#define _XP3ExtractVersion_ L"Ver 2.0.2.3"

class GlobalData
{
	GlobalData();
	~GlobalData();

	Void NTAPI ExitKrkr();

	static GlobalData* Handle;

public:
	static GlobalData* GetGlobalData();
	
public:
	BOOL                            InitedByModule;
	iTVPFunctionExporter *          TVPFunctionExporter;
	tTVPXP3ArchiveExtractionFilter  pfGlobalXP3Filter;
	HMODULE                         hSelfModule;
	HMODULE                         hHostModule;
	ULONG                           ModuleType;
	BOOL                            HasConsole;
	BOOL                            isRunning;
	BOOL                            DebugOn;

	API_POINTER(GetProcAddress)         StubGetProcAddress;
	API_POINTER(CreateProcessInternalW) StubCreateProcessInternalW;
	ULONG_PTR                           OriginalReturnAddress, FirstSectionAddress;
	ULONG                               SizeOfImage;
	tV2Link                             StubV2Link;
	FuncCreateStream                    StubTVPCreateStream;
	FuncHostAlloc                       StubHostAlloc;
	ULONG_PTR						    IStreamAdapterVtable;

private:
	ULONG                           PngFlag;
	ULONG                           TlgFlag;
	ULONG                           TextFlag;
	ULONG                           PsbFlag;

	WCHAR                           Folder   [MAX_PATH];
	WCHAR                           GuessPack[MAX_PATH];
	WCHAR                           OutPack  [MAX_PATH];
	
public:
	WCHAR                           SelfPath[MAX_PATH];
	WCHAR                           DragFileName[MAX_PATH];

public:
	ULONG     SetPngFlag(ULONG Flag);
	ULONG     GetPngFlag();

	ULONG     SetTlgFlag(ULONG Flag);
	ULONG     GetTlgFlag();

	ULONG     SetTextFlag(ULONG Flag);
	ULONG     GetTextFlag();

	ULONG     AddPsbFlag(ULONG Flag);
	ULONG     DeletePsbFlag(ULONG Flag);
	BOOL      PsbFlagOn(ULONG Flag);
	Void      DebugPsbFlag();

	Void      SetFolder(LPCWSTR Name);
	Void      SetGuessPack(LPCWSTR Name);
	Void      SetOutputPack(LPCWSTR Name);

	Void      GetFolder(LPWSTR Name, ULONG BufferMaxLength);
	Void      GetGuessPack(LPWSTR Name, ULONG BufferMaxLength);
	Void      GetOutputPack(LPWSTR Name, ULONG BufferMaxLength);

	BOOL      FindCodeSlow(PCChar Start, ULONG Size, PCChar Pattern, ULONG PatternLen);

	NTSTATUS NTAPI InitWindow();
	NTSTATUS NTAPI InitHook(LPCWSTR ModuleName, PVoid ImageBase);
	NTSTATUS NTAPI InitHookNull();

	Void NTAPI EnableAll(HWND hWnd);
	Void NTAPI DisableAll(HWND hWnd);
	Void NTAPI AdjustCP(HWND hWnd);
	Void NTAPI SetProcess(HWND hWnd, ULONG Value);
	Void NTAPI SetCount(DWORD Count);
	Void NTAPI SetCurFile(DWORD iPos);
	Void NTAPI ForceReset();
	Void NTAPI Reset();
	Void NTAPI SetDllModule(HMODULE hModule);

	ForceInline static wstring FASTCALL GetUpperExtName(wstring& FileName)
	{
		auto Index = FileName.find_last_of(L'.');
		if (Index != std::wstring::npos)
		{
			wstring Result;
			for (auto& EachChar : FileName.substr(Index + 1, std::wstring::npos))
			{
				Result += CHAR_UPPER(EachChar);
			}
			return Result;
		}

		return nullptr;
	}

	HWND                        MainWindow;
	WCHAR                       WinText[1000];
	BOOL                        Inited;
	BOOL                        IsM2Format;
	DWORD                       M2ChunkMagic;
	BOOL                        RawExtract;
	ULONG_PTR                   CountFile;
	vector<XP3Index>            ItemVector;
	HANDLE                      WorkerThread;
	HANDLE                      CurrentTempHandle;
	wstring                     CurrentTempFileName;
	BOOL                        FakePngWorkerInited;
	WCHAR                       CurFileName[MAX_PATH];
};

//Init once
NTSTATUS NTAPI InitExporter(iTVPFunctionExporter *Exporter);
NTSTATUS NTAPI InitExporterByExeModule(iTVPFunctionExporter *Exporter);
