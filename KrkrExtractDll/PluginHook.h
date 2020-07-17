#ifndef _PluginHook_
#define _PluginHook_

#include <string>
#include "tp_stub.h"
#include <vector>
#include "zlib.h"
#include "GlobalInit.h"
#include "detours.h"
#include "Console.h"
#include "HookLoadLibary.h"
#include <time.h>
#include "XP3Info.h"
#include "XP3Offset.h"
#include "WinFile.h"
#include "PackerInfo.h"
#include "FakePNG.h"
#include "GlobalInit.h"
#include "TLGDecoder.h"
#include "PNGWorker.h"
#include "KrkrTextDecode.h"
#include "PsbWorker.h"
#include <Shlwapi.h> 
#include <algorithm>
#include <atlimage.h>

#define _XP3ExtractVersion_ L"Ver 1.0.3.1"

#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "detours.lib")
#pragma comment(lib, "Shlwapi.lib")

using std::wstring;
using std::vector;

class PluginHook
{
public:
	static PluginHook* GetInstance();
private:
	PluginHook();
	~PluginHook();

	BOOL Inited;
	HANDLE hThread;

	static PluginHook* Handle;
	WCHAR wFileName[MAX_PATH];

	LPCWSTR szApplicationName = L"[X'moe]Wellcome to KrkrExtract(version : %s, built on : %s)";
	LPCWSTR szWindowClassName = L"XP3ExtractMainClass";

public:
	HRESULT WINAPI InitHook(LPCSTR  ModuleName, HMODULE ImageBase);
	HRESULT WINAPI InitHook(LPCWSTR ModuleName, HMODULE ImageBase);
	HRESULT WINAPI InitHookNull();

//private:
	HRESULT WINAPI InitWindow();

public:
	HRESULT WINAPI StartDumper();
	HRESULT WINAPI BeginThread();

	HRESULT WINAPI StartPacker();

	VOID WINAPI SetCount(DWORD Count);
	VOID WINAPI SetCurFile(DWORD iPos);
	VOID WINAPI SetFile(LPCWSTR lpFile);
	HRESULT WINAPI AdjustCP(HWND hWnd);

private:
	VOID WINAPI AddPath(LPCWSTR lpPath);

	HRESULT WINAPI ParseXP3File(PWCHAR lpFileName);
	HRESULT WINAPI ProcessXP3Archive(std::wstring xp3, HANDLE st);
	HRESULT WINAPI DumpFile();
	HRESULT WINAPI Reset();
	HRESULT WINAPI EnableAll(HWND hWnd);
	HRESULT WINAPI DisableAll(HWND hWnd);
	HRESULT WINAPI SetProcess(HWND hWnd, ULONG Value);

	BOOL  WINAPI IsCompatXP3(PBYTE Data, ULONG Size, PBYTE pMagic = nullptr);

	//Compat Version
	BOOL  WINAPI InitIndexFileFirst (PBYTE pDecompress, ULONG Size);
	BOOL  WINAPI InitIndexFile_SenrenBanka(PBYTE pDecompress, ULONG Size, HANDLE hFile);
	//Ç§Áµ
	ULONG WINAPI DetectCompressedChunk(PBYTE pDecompress, ULONG Size);
	ULONG WINAPI FindChunkMagicFirst(PBYTE pDecompress, ULONG Size);

	BOOL  WINAPI InitIndexNew1(PBYTE pDecompress, ULONG Size);

	HWND        Gui;
	ULONG_PTR   CountFile;
	std::vector<XP3Index> ItemVector;
	std::vector<XP3Index> ItemNormalVector;

	BOOL    isM2Format;
	HANDLE  hExtractThread;
	BYTE    M2ChunkMagic[4];
	wstring WinText;
	BOOL    RawExtract;

public:
	PackInfo* Packer;
	HWND PackerButton;
	HWND PngButton;
};


#endif
