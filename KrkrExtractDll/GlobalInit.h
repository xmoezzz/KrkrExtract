#pragma once

#include "tp_stub.h"
#include <string>
#include <ctime>

using std::wstring;

#define SET_FLAG(_V, _F)     ((_V) |= (_F))
#define CLEAR_FLAG(_V, _F)   ((_V) &= ~(_F))
#define FLAG_ON(_V, _F)      (!!((_V) & (_F)))
#define FLAG_OFF(_V, _F)     (!FLAG_ON(_V, _F))

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

typedef void(__stdcall * XP3Filter)(tTVPXP3ArchiveExtractionFilter);

class GlobalData
{
	GlobalData();
	static GlobalData* Handle;

public:
	static GlobalData* GetGlobalData(HMODULE hSelf = nullptr);

public:
	BOOL                            InitedByModule;
	iTVPFunctionExporter *          TVPFunctionExporter;
	tTVPXP3ArchiveExtractionFilter  pfGlobalXP3Filter;
	HMODULE                         hImageModule;
	HMODULE                         hSelfModule;
	HMODULE                         hHostModule;
	ULONG                           ModuleType;
	BOOL                            HasConsole;
	BOOL                            isRunning;

private:
	ULONG                           PngFlag;
	ULONG                           TlgFlag;
	ULONG                           TextFlag;
	ULONG                           PsbFlag;

	wstring                         Folder;
	wstring                         GuessPack;
	wstring                         OutPack;

public:
	WCHAR                           SelfPath[MAX_PATH];

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
	VOID      DebugPsbFlag();

	VOID      SetFolder(wstring& Name);
	VOID      SetGuessPack(wstring& Name);
	VOID      SetOutputPack(wstring& Name);

	wstring   GetFolder();
	wstring   GetGuessPack();
	wstring   GetOutputPack();

	bool      FindCodeSlow(const PCHAR start, ULONG size, const PCHAR Pattern, ULONG PatternLen);
	ULONG     GetFileLen(LPVOID pBaseaddr, LPVOID pReadBuf);
	WCHAR*    GetCompiledDate();

};

//Init once
HRESULT WINAPI InitExporter(iTVPFunctionExporter *exporter);
HRESULT WINAPI InitExporterByExeModule();
