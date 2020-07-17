#include "PluginHook.h"
#include "resource.h"
#include <WindowsX.h>

/********************************************/

PluginHook::PluginHook() :
Packer(nullptr)
{
	RawExtract = FALSE;
	PngButton = nullptr;
	RtlZeroMemory(M2ChunkMagic, 4);
	RtlZeroMemory(wFileName, MAX_PATH * 2);
	Gui = nullptr;
	CountFile = 0;
	Inited = FALSE;
	hThread = INVALID_HANDLE_VALUE;
	hExtractThread = INVALID_HANDLE_VALUE;
	isM2Format = FALSE;
}


PluginHook::~PluginHook()
{
	if (hThread != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hThread);
	}
	if (Gui)
	{
		DestroyWindow(Gui);
	}
}

PluginHook* PluginHook::GetInstance()
{
	if (Handle == NULL)
	{
		Handle = new PluginHook;
	}
	return Handle;
}


BOOLEAN TVPGetXP3ArchiveOffset(HANDLE st, ULONG64& offset)
{
	tjs_uint8 mark[11 + 1];
	DWORD nRet = 0;

	SetFilePointer(st, 0, NULL, FILE_BEGIN);
	mark[0] = 0;
	ReadFile(st, mark, 11, &nRet, NULL);
	if (11 == RtlCompareMemory(StaticXP3V2Magic, mark, 11))
	{
		offset = 0;
	}
	else
	{
		MessageBoxW(NULL, L"Invaild XP3 Package!", L"KrkrExtract", MB_OK);
		return FALSE;
	}
	return TRUE;
}


/*****************************************************************/

PluginHook* PluginHook::Handle = nullptr;


static PWCHAR VersionString = L"[X'moe]XP3-core-krkr2-krkrz_sig_Hash";

extern "C" const PWCHAR WINAPI GetToolVersion()
{
	return VersionString;
}


typedef HRESULT(_stdcall *tlink)(iTVPFunctionExporter *);
PVOID pV2Link = NULL;
HRESULT WINAPI WarppedV2Link(iTVPFunctionExporter *exporter)
{
	InitExporter(exporter);
	WCHAR Info[200] = { 0 };
	wsprintfW(Info, L"exporter : [0x%08x]\n", (DWORD)exporter);
	OutputString(Info);
	GlobalData::GetGlobalData()->TVPFunctionExporter = exporter;

	static char funcname233[] = "IStream * ::TVPCreateIStream(const ttstr &,tjs_uint32)";

	PVOID FuncStub = nullptr;
	__try
	{
		FuncStub = TVPGetImportFuncPtr(funcname233);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
	
	if (FuncStub)
	{
		wsprintfW(Info, L"Stub Addr : [0x%08x]\n", FuncStub);
		OutputString(Info);
	}
	else
	{
		OutputString(L"Stub Addr : nullptr\n");
	}

	InitLayer();
	return ((tlink)pV2Link)(exporter);
}


DWORD WINAPI InitWindowThread(LPVOID lpParam)
{
	OutputString(L"Try to init window\n");
	PluginHook* o = (PluginHook*)lpParam;
	o->InitWindow();
	return 0;
}

HRESULT WINAPI PluginHook::InitHookNull()
{
	if (Inited == TRUE)
	{
		return S_OK;
	}
	Inited = TRUE;
	//return InitWindow();
	DWORD id;
	CreateThread(NULL, NULL, InitWindowThread, this, NULL, &id);
	return S_OK;
}


HRESULT WINAPI PluginHook::InitHook(const char* ModuleName, HMODULE ImageBase)
{
	if (Inited == TRUE)
	{
		return S_OK;
	}
	if (ImageBase == NULL)
	{
		return S_FALSE;
	}
	unsigned int Strlen = lstrlenA(ModuleName);
	if (Strlen <= 3)
	{
		return S_FALSE;
	}
	Strlen--;
	if ((ModuleName[Strlen] != 'm' || ModuleName[Strlen - 1] != 'p' || ModuleName[Strlen - 2] != 't') &&
		(ModuleName[Strlen] != 'l' || ModuleName[Strlen - 1] != 'l' || ModuleName[Strlen - 2] != 'd'))
	{
		return S_FALSE;
	}
	tlink v2link = (tlink)GetProcAddress(ImageBase, "V2Link");
	if (v2link == NULL)
	{
		return S_FALSE;
	}
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	pV2Link = DetourFindFunction(ModuleName, "V2Link");
	DetourAttach(&pV2Link, WarppedV2Link);
	DetourTransactionCommit();

	WCHAR ImageW[MAX_PATH] = { 0 };
	RtlMultiByteToUnicodeN(ImageW, MAX_PATH, nullptr, ModuleName, lstrlenA(ModuleName));
	WCHAR Info[512] = { 0 };
	wsprintfW(Info, L"[Proc]%s\n", ImageW);
	OutputString(Info);

	Inited = TRUE;
	//return InitWindow();
	DWORD id;
	CreateThread(NULL, NULL, InitWindowThread, this, NULL, &id);
	return S_OK;
}


HRESULT WINAPI PluginHook::InitHook(const wchar_t* ModuleName, HMODULE ImageBase)
{
#if 0
	if (Inited == TRUE)
	{
		return S_OK;
	}
	if (ImageBase == NULL)
	{
		return S_FALSE;
	}
	unsigned int Strlen = lstrlenW(ModuleName);
	if (Strlen <= 3)
	{
		return S_FALSE;
	}
	Strlen--;
	if ((ModuleName[Strlen] != L'm' || ModuleName[Strlen - 1] != L'p' || ModuleName[Strlen - 2] != L't') &&
		(ModuleName[Strlen] != L'l' || ModuleName[Strlen - 1] != L'l' || ModuleName[Strlen - 2] != L'd'))
	{
		return S_FALSE;
	}
	tlink v2link = (tlink)GetProcAddress(ImageBase, "V2Link");
	if (v2link == NULL)
	{
		return S_FALSE;
	}
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	//pV2Link = DetourFindFunction(ModuleName, "V2Link");
	DetourAttach(&pV2Link, WarppedV2Link);
	DetourTransactionCommit();

	WCHAR Info[400] = { 0 };
	wsprintfW(Info, L"[Proc]%s\n", ModuleName);
	OutputString(Info);

	Inited = TRUE;
	DWORD id;
	CreateThread(NULL, NULL, InitWindowThread, this, NULL, &id);

#endif
	return S_OK;
}



HRESULT WINAPI PluginHook::ParseXP3File(PWCHAR lpFileName)
{
	WCHAR Info[512] = { 0 };
	wsprintfW(Info, L"Parsing at [%s]\n", lpFileName);
	OutputString(Info);
	HANDLE hFile = INVALID_HANDLE_VALUE;
	hFile = CreateFileW(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == NULL)
	{
		RtlZeroMemory(Info, 1024);
		wsprintfW(Info, L"Couldn't open [%s]\n", lpFileName);
		OutputString(Info);
		return S_FALSE;
	}
	if (S_OK != this->ProcessXP3Archive(wstring(lpFileName), hFile))
	{
		CloseHandle(hFile);
		return S_OK;
	}
	CloseHandle(hFile);
	return S_OK;
}


VOID WINAPI PluginHook::AddPath(const wchar_t* FileName)
{
	int iPos = 0;
	while (FileName[iPos] != NULL)
	{
		if (FileName[iPos] == L'/' || FileName[iPos] == L'\\')
		{
			wchar_t iPath[260] = { 0 };
			wchar_t  CurrPath[260] = { 0 };
			GetCurrentDirectoryW(260, CurrPath);
			lstrcpynW(iPath, FileName, iPos + 1);
			wstring CreateP(CurrPath);
			CreateP += L"\\";
			CreateP += iPath;
			CreateDirectoryW(CreateP.c_str(), NULL);
		}
		iPos++;
	}
}

VOID WINAPI PluginHook::SetFile(const wchar_t* lpFile)
{
	RtlZeroMemory(wFileName, MAX_PATH * 2);
	lstrcpyW(wFileName, lpFile);
}


/**********************************/
//ThreadProc
DWORD WINAPI ExtractThread(LPVOID lParam)
{
	PluginHook* This = static_cast<PluginHook*>(lParam);
	return static_cast<DWORD>(This->StartDumper());
}


/*************************************/
//GUI

#define GetItemX(x)    GetDlgItem(hWnd, x)
#define EnableX(x)     EnableWindow(x, TRUE)
#define DisableX(x)    EnableWindow(x, FALSE)
#define CheckItem(x)   SendMessageW(x, BM_SETCHECK, BST_UNCHECKED, 0)
#define UnCheckItem(x) SendMessageW(x, BM_SETCHECK, BST_CHECKED, 0)

HRESULT WINAPI PluginHook::EnableAll(HWND hWnd)
{
	HWND hPNGRaw = GetItemX(IDC_PNG_RAW);
	HWND hPNGSys = GetItemX(IDC_PNG_SYSTEM);

	HWND hPSBRaw   = GetItemX(IDC_PSB_RAW);
	HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
	HWND hPSBText  = GetItemX(IDC_PSB_TEXT);
	HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
	HWND hPSBAnm   = GetItemX(IDC_PSB_ANIM);
	HWND hPSBAll   = GetItemX(IDC_PSB_ALL);

	HWND hTXTRaw  = GetItemX(IDC_TEXT_RAW);
	HWND hTXTText = GetItemX(IDC_TEXT_TEXT);

	HWND hTLGRaw     = GetItemX(IDC_TLG_RAW);
	HWND hTLGBuildin = GetItemX(IDC_TLG_BUILDIN);
	HWND hTLGSys     = GetItemX(IDC_TLG_SYSTEM);
	HWND hTLGPng     = GetItemX(IDC_TLG_PNG);

	HWND hFolderEdit  = GetItemX(IDC_PACK_EDIT_FOLDER);
	HWND hOriPackEdit = GetItemX(IDC_PACK_EDIT_ORIPACK);
	HWND hOutPackEdit = GetItemX(IDC_PACK_EDIT_OUTPACK);

	HWND hFolderSel  = GetItemX(IDC_PACK_BUTTON_FOLDER);
	HWND hOriPackSel = GetItemX(IDC_PACK_BUTTON_ORIPACK);
	HWND hOutPackSel = GetItemX(IDC_PACK_BUTTON_OUTPACK);

	HWND hDoPack  = GetItemX(IDC_PACK_BUTTON_MAKE);
	HWND hProcess = GetItemX(IDC_PROGRESS1);
	HWND hDbg     = GetItemX(IDC_BUTTON_DEBUGGER);

	EnableX(hPNGRaw);
	EnableX(hPNGSys);

	EnableX(hPSBRaw);
	EnableX(hPSBDecom);
	EnableX(hPSBText);
	EnableX(hPSBImage);
	EnableX(hPSBAnm);
	EnableX(hPSBAll);

	EnableX(hTXTRaw);
	EnableX(hTXTText);

	EnableX(hTLGRaw);
	EnableX(hTLGBuildin);
	EnableX(hTLGSys);
	EnableX(hTLGPng);

	EnableX(hFolderEdit);
	EnableX(hOriPackEdit);
	EnableX(hOutPackEdit);

	EnableX(hFolderSel);
	EnableX(hOriPackSel);
	EnableX(hOutPackSel);

	EnableX(hDoPack);
	return S_OK;
}

HRESULT WINAPI PluginHook::DisableAll(HWND hWnd)
{
	HWND hPNGRaw = GetItemX(IDC_PNG_RAW);
	HWND hPNGSys = GetItemX(IDC_PNG_SYSTEM);

	HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
	HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
	HWND hPSBText = GetItemX(IDC_PSB_TEXT);
	HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
	HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
	HWND hPSBAll = GetItemX(IDC_PSB_ALL);

	HWND hTXTRaw = GetItemX(IDC_TEXT_RAW);
	HWND hTXTText = GetItemX(IDC_TEXT_TEXT);

	HWND hTLGRaw = GetItemX(IDC_TLG_RAW);
	HWND hTLGBuildin = GetItemX(IDC_TLG_BUILDIN);
	HWND hTLGSys = GetItemX(IDC_TLG_SYSTEM);
	HWND hTLGPng = GetItemX(IDC_TLG_PNG);

	HWND hFolderEdit = GetItemX(IDC_PACK_EDIT_FOLDER);
	HWND hOriPackEdit = GetItemX(IDC_PACK_EDIT_ORIPACK);
	HWND hOutPackEdit = GetItemX(IDC_PACK_EDIT_OUTPACK);

	HWND hFolderSel = GetItemX(IDC_PACK_BUTTON_FOLDER);
	HWND hOriPackSel = GetItemX(IDC_PACK_BUTTON_ORIPACK);
	HWND hOutPackSel = GetItemX(IDC_PACK_BUTTON_OUTPACK);

	HWND hDoPack = GetItemX(IDC_PACK_BUTTON_MAKE);
	HWND hProcess = GetItemX(IDC_PROGRESS1);
	HWND hDbg = GetItemX(IDC_BUTTON_DEBUGGER);

	DisableX(hPNGRaw);
	DisableX(hPNGSys);

	DisableX(hPSBRaw);
	DisableX(hPSBDecom);
	DisableX(hPSBText);
	DisableX(hPSBImage);
	DisableX(hPSBAnm);
	DisableX(hPSBAll);

	DisableX(hTXTRaw);
	DisableX(hTXTText);

	DisableX(hTLGRaw);
	DisableX(hTLGBuildin);
	DisableX(hTLGSys);
	DisableX(hTLGPng);

	DisableX(hFolderEdit);
	DisableX(hOriPackEdit);
	DisableX(hOutPackEdit);

	DisableX(hFolderSel);
	DisableX(hOriPackSel);
	DisableX(hOutPackSel);

	DisableX(hDoPack);
	return S_OK;
}

#define LANG_CNS 0
#define LANG_CNT 1
#define LANG_JP  2
#define LANG_EN  3

static WCHAR* LangDropList[4] = 
{
	L"请拖放xp3文件或者文件夹>_<",
	L"請拖放xp3檔案或者資料夾>_<",
	L"xp3ファイル（フォルダ）をこっちに置いてください>_<",
	L"Drop a xp3 file or folder here>_<"
};

static WCHAR* LangGroupPNGList[4] = 
{
	L"PNG 设定",
	L"PNG 設定",
	L"PNG 設定",
	L"PNG Setting"
};

static WCHAR* LangALLRaw[4] = 
{
	L"原始",
	L"原始",
	L"ない",
	L"Raw"
};

static WCHAR* LangPNGSystem[4] = 
{
	L"系统编码器",
	L"系統編碼器",
	L"システムエンコーダー",
	L"System Decoder"
};

static WCHAR* LangGroupPSBList[4] =
{
	L"PSB封包",
	L"PSB封包",
	L"PSB Package",
	L"PSB Package"
};

static WCHAR* LangPSBDecompile[4] =
{
	L"反编译",
	L"反編譯",
	L"Decompile Script",
	L"Decompile Script"
};

static WCHAR* LangPSBImage[4] =
{
	L"解包图片文件",
	L"解包圖片檔案",
	L"Unapck Image",
	L"Unapck Image"
};

static WCHAR* LangPSBAnm[4] =
{
	L"解包动画文件",
	L"解包動畫檔案",
	L"Unapck Animation",
	L"Unapck Animation"
};

static WCHAR* LangPSBText[4] =
{
	L"提取文本",
	L"提取文本",
	L"Dump Text",
	L"Dump Text"
};

static WCHAR* LangPSBALL[4] =
{
	L"解包全部",
	L"解包全部",
	L"Full Unpack",
	L"Full Unpack"
};

static WCHAR* LangTEXTDecode[4] =
{
	L"解密",
	L"解密",
	L"復号",
	L"Text"
};

static WCHAR* LangGroupPackList[4] =
{
	L"封包设定",
	L"封包設定",
	L"Pack Setting",
	L"Pack Setting"
};

static WCHAR* LangPackFolder[4] =
{
	L"封包目录",
	L"封包目錄",
	L"Folder",
	L"Folder"
};

static WCHAR* LangPackOri[4] =
{
	L"原始封包",
	L"原始封包",
	L"Ori Pack",
	L"Ori Pack"
};

static WCHAR* LangPackOut[4] =
{
	L"输出封包",
	L"輸出封包",
	L"Out Pack",
	L"Out Pack"
};

static WCHAR* LangPackSelect[4] =
{
	L"选择",
	L"選擇",
	L"選択",
	L"Select"
};

static WCHAR* LangPackMake[4] =
{
	L"开始封包",
	L"開始封包",
	L"Make Package",
	L"Make Package"
};

static WCHAR* LangGroupTLGList[4] =
{
	L"TLG图片",
	L"TLG圖片",
	L"TLG画像",
	L"TLG Image"
};

static WCHAR* LangTLGBuildIn[4] =
{
	L"内置编码器",
	L"內置編碼器",
	L"内蔵エンコーダ",
	L"Build-in Decoder"
};

static WCHAR* LangTLGSystem[4] =
{
	L"系统(PNG)",
	L"系統(PNG)",
	L"システム",
	L"System(PNG)"
};

static WCHAR* LangTLGPNG[4] =
{
	L"PNG",
	L"PNG",
	L"PNG",
	L"PNG"
};

static WCHAR* LangGroupProcessList[4] =
{
	L"进度",
	L"進度",
	L"進度",
	L"Process"
};

static WCHAR* LangOpenDbg[4] =
{
	L"开启调试",
	L"開啟調試",
	L"Open Debugger",
	L"Open Debugger"
};

static WCHAR* LangCloseDbg[4] =
{
	L"关闭调试",
	L"關閉調試",
	L"Close Debugger",
	L"Close Debugger"
};

#define ReturnCheck(x, minVar, maxVar) return (minVar > x || x > maxVar) ?  maxVar : x

ULONG WINAPI GetLangIndex()
{
	ULONG CodePage = GetACP();
	
	if (CodePage == 936)
	{
		ReturnCheck(LANG_CNS, 0, 3);
	}
	else if (CodePage == 932)
	{
		ReturnCheck(LANG_JP, 0, 3);
	}
	else if (CodePage == 950)
	{
		ReturnCheck(LANG_CNT, 0, 3);
	}
	else
	{
		ReturnCheck(LANG_EN, 0, 3);
	}
}


//无视==
HRESULT WINAPI PluginHook::AdjustCP(HWND hWnd)
{
	HWND hStaticInfo = GetItemX(IDC_INFO_TEXT);
	SetWindowTextW(hStaticInfo, LangDropList[GetLangIndex()]);

	HWND hStaticGroupPNG = GetItemX(IDC_G_PNG);
	SetWindowTextW(hStaticGroupPNG, LangGroupPNGList[GetLangIndex()]);

	HWND hStaticPNGRaw = GetItemX(IDC_PNG_RAW);
	SetWindowTextW(hStaticPNGRaw, LangALLRaw[GetLangIndex()]);

	HWND hStaticPNGSystem = GetItemX(IDC_PNG_SYSTEM);
	SetWindowTextW(hStaticPNGSystem, LangPNGSystem[GetLangIndex()]);

	HWND hStaticGroupTLG = GetItemX(IDC_G_TLG);
	SetWindowTextW(hStaticGroupTLG, LangGroupTLGList[GetLangIndex()]);

	HWND hStaticTLGBuildIn = GetItemX(IDC_TLG_BUILDIN);
	SetWindowTextW(hStaticTLGBuildIn, LangTLGBuildIn[GetLangIndex()]);

	HWND hStaticTLGRaw = GetItemX(IDC_TLG_RAW);
	SetWindowTextW(hStaticTLGRaw, LangALLRaw[GetLangIndex()]);

	HWND hStaticTLGSys = GetItemX(IDC_TLG_SYSTEM);
	SetWindowTextW(hStaticTLGSys, LangTLGSystem[GetLangIndex()]);

	HWND hStaticTLGPng = GetItemX(IDC_TLG_PNG);
	SetWindowTextW(hStaticTLGPng, LangTLGPNG[GetLangIndex()]);

	HWND hStaticGroupPSB = GetItemX(IDC_G_PSB);
	SetWindowTextW(hStaticGroupPSB, LangGroupPSBList[GetLangIndex()]);

	HWND hStaticPSBRaw = GetItemX(IDC_PSB_RAW);
	SetWindowTextW(hStaticPSBRaw, LangALLRaw[GetLangIndex()]);

	HWND hStaticPSBDcom = GetItemX(IDC_PSB_SCRIPT);
	SetWindowTextW(hStaticPSBDcom, LangPSBDecompile[GetLangIndex()]);

	HWND hStaticPSBImage = GetItemX(IDC_PSB_IMAGE);
	SetWindowTextW(hStaticPSBImage, LangPSBImage[GetLangIndex()]);

	HWND hStaticPSBAnm = GetItemX(IDC_PSB_ANIM);
	SetWindowTextW(hStaticPSBAnm, LangPSBAnm[GetLangIndex()]);

	HWND hStaticPSBText = GetItemX(IDC_PSB_TEXT);
	SetWindowTextW(hStaticPSBText, LangPSBText[GetLangIndex()]);

	HWND hStaticPSBAll = GetItemX(IDC_PSB_ALL);
	SetWindowTextW(hStaticPSBAll, LangPSBALL[GetLangIndex()]);

	HWND hStaticGroupText = GetItemX(IDC_G_TEXT);
	//SetWindowTextW(hStaticPSBAll, LangPSBALL[GetLangIndex()]);

	HWND hStaticTextRaw = GetItemX(IDC_TEXT_RAW);
	SetWindowTextW(hStaticTextRaw, LangALLRaw[GetLangIndex()]);

	HWND hStaticTextDecode = GetItemX(IDC_TEXT_TEXT);
	SetWindowTextW(hStaticTextDecode, LangTEXTDecode[GetLangIndex()]);

	HWND hStaticGroupPack = GetItemX(IDC_G_PACK);
	SetWindowTextW(hStaticGroupPack, LangGroupPackList[GetLangIndex()]);

	HWND hStaticPackOri = GetItemX(IDC_PACK_T_ORIPACK);
	SetWindowTextW(hStaticPackOri, LangPackFolder[GetLangIndex()]);

	HWND hStaticPackFolder = GetItemX(IDC_PACK_T_FOLDER);
	SetWindowTextW(hStaticPackFolder, LangPackOri[GetLangIndex()]);

	HWND hStaticPackOut = GetItemX(IDC_PACK_T_OUTPACK);
	SetWindowTextW(hStaticPackOut, LangPackOut[GetLangIndex()]);

	HWND hStaticButtonPackOri = GetItemX(IDC_PACK_BUTTON_ORIPACK);
	SetWindowTextW(hStaticButtonPackOri, LangPackSelect[GetLangIndex()]);

	HWND hStaticButtonPackOut = GetItemX(IDC_PACK_BUTTON_OUTPACK);
	SetWindowTextW(hStaticButtonPackOut, LangPackSelect[GetLangIndex()]);

	HWND hStaticButtonPackFolder = GetItemX(IDC_PACK_BUTTON_FOLDER);
	SetWindowTextW(hStaticButtonPackFolder, LangPackSelect[GetLangIndex()]);

	HWND hStaticGroupProcess = GetItemX(IDC_G_PROCESS);
	SetWindowTextW(hStaticGroupProcess, LangGroupProcessList[GetLangIndex()]);

	HWND hStaticDbg = GetItemX(IDC_BUTTON_DEBUGGER);
	SetWindowTextW(hStaticGroupProcess, LangOpenDbg[GetLangIndex()]);


	HWND hStaticMake = GetItemX(IDC_PACK_BUTTON_MAKE);
	SetWindowTextW(hStaticMake, LangPackMake[GetLangIndex()]);

	return S_OK;
}

HRESULT WINAPI PluginHook::SetProcess(HWND hWnd, ULONG Value)
{
	HWND hProcess = GetItemX(IDC_PROGRESS1);
	SendMessageW(hProcess, PBM_SETPOS, Value, 0);
	return S_OK;
}

static WCHAR strFileName[MAX_PATH] = { 0 };

LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT Painter;

	UNREFERENCED_PARAMETER(Painter);

	switch (msg)
	{
	case WM_CREATE:
	{
	}
	break;

	case WM_INITDIALOG:
	{
		HWND hPNGRaw = GetItemX(IDC_PNG_RAW);
		HWND hPNGSys = GetItemX(IDC_PNG_SYSTEM);

		HWND hPSBRaw   = GetItemX(IDC_PSB_RAW);
		HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
		HWND hPSBText  = GetItemX(IDC_PSB_TEXT);
		HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
		HWND hPSBAnm   = GetItemX(IDC_PSB_ANIM);
		HWND hPSBAll   = GetItemX(IDC_PSB_ALL);

		HWND hTXTRaw  = GetItemX(IDC_TEXT_RAW);
		HWND hTXTText = GetItemX(IDC_TEXT_TEXT);

		HWND hTLGRaw     = GetItemX(IDC_TLG_RAW);
		HWND hTLGBuildin = GetItemX(IDC_TLG_BUILDIN);
		HWND hTLGSys     = GetItemX(IDC_TLG_SYSTEM);
		HWND hTLGPng     = GetItemX(IDC_TLG_PNG);

		HWND hFolderEdit  = GetItemX(IDC_PACK_EDIT_FOLDER);
		HWND hOriPackEdit = GetItemX(IDC_PACK_EDIT_ORIPACK);
		HWND hOutPackEdit = GetItemX(IDC_PACK_EDIT_OUTPACK);

		HWND hFolderSel  = GetItemX(IDC_PACK_BUTTON_FOLDER);
		HWND hOriPackSel = GetItemX(IDC_PACK_BUTTON_ORIPACK);
		HWND hOutPackSel = GetItemX(IDC_PACK_BUTTON_OUTPACK);

		HWND hDoPack  = GetItemX(IDC_PACK_BUTTON_MAKE);
		HWND hProcess = GetItemX(IDC_PROGRESS1);
		HWND hDbg     = GetItemX(IDC_BUTTON_DEBUGGER);

		EnableX(hPNGRaw);
		EnableX(hPNGSys);

		EnableX(hPSBRaw);
		EnableX(hPSBDecom);
		EnableX(hPSBText);
		EnableX(hPSBImage);
		EnableX(hPSBAnm);
		EnableX(hPSBAll);

		EnableX(hTXTRaw);
		EnableX(hTXTText);

		EnableX(hTLGRaw);
		EnableX(hTLGBuildin);
		EnableX(hTLGSys);
		EnableX(hTLGPng);

		EnableX(hFolderEdit);
		EnableX(hOriPackEdit);
		EnableX(hOutPackEdit);

		EnableX(hFolderSel);
		EnableX(hOriPackSel);
		EnableX(hOutPackSel);

		EnableX(hDoPack);
		EnableX(hProcess);
		EnableX(hDbg);

		PluginHook::GetInstance()->AdjustCP(hWnd);

		Button_SetCheck(hPNGRaw,     BST_CHECKED);
		Button_SetCheck(hPNGSys,     BST_UNCHECKED);
		GlobalData::GetGlobalData()->SetPngFlag(PNG_RAW);

		Button_SetCheck(hPSBRaw,     BST_CHECKED);
		Button_SetCheck(hPSBDecom,   BST_UNCHECKED);
		Button_SetCheck(hPSBText,    BST_UNCHECKED);
		Button_SetCheck(hPSBImage,   BST_UNCHECKED);
		Button_SetCheck(hPSBAnm,     BST_UNCHECKED);
		Button_SetCheck(hPSBAll,     BST_UNCHECKED);
		GlobalData::GetGlobalData()->AddPsbFlag(PSB_RAW);

		Button_SetCheck(hTXTRaw,     BST_CHECKED);
		Button_SetCheck(hTXTText,    BST_UNCHECKED);
		GlobalData::GetGlobalData()->SetTextFlag(TEXT_RAW);

		Button_SetCheck(hTLGRaw,     BST_CHECKED);
		Button_SetCheck(hTLGBuildin, BST_UNCHECKED);
		Button_SetCheck(hTLGSys,     BST_UNCHECKED);
		Button_SetCheck(hTLGPng,     BST_UNCHECKED);
		GlobalData::GetGlobalData()->SetTlgFlag(TLG_RAW);

		WCHAR   DataName[MAX_PATH] = { 0 };
		WinFile File;
		
		GetCurrentDirectoryW(MAX_PATH, DataName);
		lstrcatW(DataName, L"\\data.xp3");
		if (FAILED(File.Open(DataName, WinFile::FileRead)))
		{
		}
		else
		{
			GlobalData::GetGlobalData()->SetGuessPack(wstring(DataName));
			SetWindowTextW(hOriPackEdit, DataName);
		}
		SendMessageW(hProcess, PBM_SETRANGE, (WPARAM)0, (LPARAM)(MAKELPARAM(0, 100)));
		SendMessageW(hProcess, PBM_SETSTEP,  1, 0);
	}
	break;

	case WM_SYSCOMMAND:
	{
		switch (wParam)
		{
		case SC_CLOSE:
			ExitProcess(0);
			break;
		default:
			break;
		}
	}
	break;

	case WM_COMMAND:
	{
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDC_PACK_EDIT_FOLDER:
		{
			switch (wmEvent)
			{
			case EN_CHANGE:
			{
				HWND hFolderEdit = GetItemX(IDC_PACK_EDIT_FOLDER);
				WCHAR lpString[1024] = { 0 };
				GetWindowTextW(hFolderEdit, lpString, 1024);
				GlobalData::GetGlobalData()->SetFolder(wstring(lpString));
			}
			break;
			default:
				break;
			}
		}
		break;

		case IDC_PACK_EDIT_ORIPACK:
		{
			switch (wmEvent)
			{
			case EN_CHANGE:
			{
				HWND hOriPack = GetItemX(IDC_PACK_EDIT_ORIPACK);
				WCHAR lpString[1024] = { 0 };
				GetWindowTextW(hOriPack, lpString, 1024);
				GlobalData::GetGlobalData()->SetGuessPack(wstring(lpString));
			}
			break;
			default:
				break;
			}
		}
		break;

		case IDC_PACK_EDIT_OUTPACK:
		{
			switch (wmEvent)
			{
			case EN_CHANGE:
			{
				HWND hOutPack = GetItemX(IDC_PACK_EDIT_OUTPACK);
				WCHAR lpString[1024] = { 0 };
				GetWindowTextW(hOutPack, lpString, 1024);
				GlobalData::GetGlobalData()->SetOutputPack(wstring(lpString));
			}
			break;
			default:
				break;
			}
		}
		break;

		//PNG Raw
		case IDC_PNG_RAW:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hPNGRaw = GetItemX(IDC_PNG_RAW);
				BOOL Result;
				Result = SendMessageW(hPNGRaw, BM_GETCHECK, 0, 0);
				if (Result == TRUE)
				{
					GlobalData::GetGlobalData()->SetPngFlag(PNG_RAW);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//PNG System
		case IDC_PNG_SYSTEM:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hPNGSys = GetItemX(IDC_PNG_SYSTEM);
				BOOL Result;
				Result = SendMessageW(hPNGSys, BM_GETCHECK, 0, 0);
				if (Result == TRUE)
				{
					GlobalData::GetGlobalData()->SetPngFlag(PNG_SYS);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//must select one mode
		//Psb Raw
		case IDC_PSB_RAW:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hPSBRaw = GetItemX(IDC_PSB_RAW);

				BOOL Result;
				Result = SendMessageW(hPSBRaw, BM_GETCHECK, 0, 0);
				if (Result == FALSE)
				{
					BOOL SubResult[5] = { FALSE };
					HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
					HWND hPSBText  = GetItemX(IDC_PSB_TEXT);
					HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
					HWND hPSBAnm   = GetItemX(IDC_PSB_ANIM);
					HWND hPSBAll   = GetItemX(IDC_PSB_ALL);

					SubResult[0] = SendMessageW(hPSBDecom, BM_GETCHECK, 0, 0);
					SubResult[1] = SendMessageW(hPSBText,  BM_GETCHECK, 0, 0);
					SubResult[2] = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
					SubResult[3] = SendMessageW(hPSBAnm,   BM_GETCHECK, 0, 0);
					SubResult[4] = SendMessageW(hPSBAll,   BM_GETCHECK, 0, 0);

					if ((SubResult[0] + SubResult[1] + SubResult[2] + SubResult[3] + SubResult[4]) == 0)
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
						Button_SetCheck(hPSBDecom, BST_UNCHECKED);
						Button_SetCheck(hPSBText,  BST_UNCHECKED);
						Button_SetCheck(hPSBImage, BST_UNCHECKED);
						Button_SetCheck(hPSBAnm,   BST_UNCHECKED);
						Button_SetCheck(hPSBAll,   BST_UNCHECKED);

						MessageBoxW(hWnd, L"You must set one flag for this operation", L"KrkrExtract", MB_OK);
						Button_SetCheck(hPSBRaw,   BST_CHECKED);
						GlobalData::GetGlobalData()->AddPsbFlag(PSB_RAW);
					}
					else
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_RAW);
					}
				}
				else
				{
					GlobalData::GetGlobalData()->AddPsbFlag(PSB_RAW);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//Psb decompile
		case IDC_PSB_SCRIPT:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);

				BOOL Result;
				Result = SendMessageW(hPSBDecom, BM_GETCHECK, 0, 0);
				if (Result == FALSE)
				{
					BOOL SubResult[5] = { FALSE };
					HWND hPSBRaw   = GetItemX(IDC_PSB_RAW);
					HWND hPSBText  = GetItemX(IDC_PSB_TEXT);
					HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
					HWND hPSBAnm   = GetItemX(IDC_PSB_ANIM);
					HWND hPSBAll   = GetItemX(IDC_PSB_ALL);

					SubResult[0] = SendMessageW(hPSBRaw,   BM_GETCHECK, 0, 0);
					SubResult[1] = SendMessageW(hPSBText,  BM_GETCHECK, 0, 0);
					SubResult[2] = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
					SubResult[3] = SendMessageW(hPSBAnm,   BM_GETCHECK, 0, 0);
					SubResult[4] = SendMessageW(hPSBAll,   BM_GETCHECK, 0, 0);

					if ((SubResult[0] + SubResult[1] + SubResult[2] + SubResult[3] + SubResult[4]) == 0)
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
						Button_SetCheck(hPSBDecom, BST_UNCHECKED);
						Button_SetCheck(hPSBText,  BST_UNCHECKED);
						Button_SetCheck(hPSBImage, BST_UNCHECKED);
						Button_SetCheck(hPSBAnm,   BST_UNCHECKED);
						Button_SetCheck(hPSBAll,   BST_UNCHECKED);

						MessageBoxW(hWnd, L"You must set one flag for this operation", L"KrkrExtract", MB_OK);
						Button_SetCheck(hPSBRaw, BST_CHECKED);
						GlobalData::GetGlobalData()->AddPsbFlag(PSB_RAW);
					}
					else
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_DECOM);
					}
				}
				else
				{
					GlobalData::GetGlobalData()->AddPsbFlag(PSB_DECOM);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//Psb Image
		case IDC_PSB_IMAGE:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);

				BOOL Result;
				Result = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
				if (Result == FALSE)
				{
					BOOL SubResult[5] = { FALSE };
					HWND hPSBRaw   = GetItemX(IDC_PSB_RAW);
					HWND hPSBText  = GetItemX(IDC_PSB_TEXT);
					HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
					HWND hPSBAnm   = GetItemX(IDC_PSB_ANIM);
					HWND hPSBAll   = GetItemX(IDC_PSB_ALL);

					SubResult[0] = SendMessageW(hPSBRaw,   BM_GETCHECK, 0, 0);
					SubResult[1] = SendMessageW(hPSBText,  BM_GETCHECK, 0, 0);
					SubResult[2] = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
					SubResult[3] = SendMessageW(hPSBAnm,   BM_GETCHECK, 0, 0);
					SubResult[4] = SendMessageW(hPSBAll,   BM_GETCHECK, 0, 0);

					if ((SubResult[0] + SubResult[1] + SubResult[2] + SubResult[3] + SubResult[4]) == 0)
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
						Button_SetCheck(hPSBDecom, BST_UNCHECKED);
						Button_SetCheck(hPSBText,  BST_UNCHECKED);
						Button_SetCheck(hPSBImage, BST_UNCHECKED);
						Button_SetCheck(hPSBAnm,   BST_UNCHECKED);
						Button_SetCheck(hPSBAll,   BST_UNCHECKED);

						MessageBoxW(hWnd, L"You must set one flag for this operation", L"KrkrExtract", MB_OK);
						Button_SetCheck(hPSBRaw, BST_CHECKED);
						GlobalData::GetGlobalData()->AddPsbFlag(PSB_RAW);
					}
					else
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_IMAGE);
					}
				}
				else
				{
					GlobalData::GetGlobalData()->AddPsbFlag(PSB_IMAGE);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		case IDC_PSB_TEXT:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hPSBText = GetItemX(IDC_PSB_TEXT);

				BOOL Result;
				Result = SendMessageW(hPSBText, BM_GETCHECK, 0, 0);
				if (Result == FALSE)
				{
					BOOL SubResult[5] = { FALSE };
					HWND hPSBRaw   = GetItemX(IDC_PSB_RAW);
					HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
					HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
					HWND hPSBAnm   = GetItemX(IDC_PSB_ANIM);
					HWND hPSBAll   = GetItemX(IDC_PSB_ALL);

					SubResult[0] = SendMessageW(hPSBRaw,   BM_GETCHECK, 0, 0);
					SubResult[1] = SendMessageW(hPSBText,  BM_GETCHECK, 0, 0);
					SubResult[2] = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
					SubResult[3] = SendMessageW(hPSBAnm,   BM_GETCHECK, 0, 0);
					SubResult[4] = SendMessageW(hPSBAll,   BM_GETCHECK, 0, 0);

					if ((SubResult[0] + SubResult[1] + SubResult[2] + SubResult[3] + SubResult[4]) == 0)
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
						Button_SetCheck(hPSBDecom, BST_UNCHECKED);
						Button_SetCheck(hPSBText,  BST_UNCHECKED);
						Button_SetCheck(hPSBImage, BST_UNCHECKED);
						Button_SetCheck(hPSBAnm,   BST_UNCHECKED);
						Button_SetCheck(hPSBAll,   BST_UNCHECKED);

						MessageBoxW(hWnd, L"You must set one flag for this operation", L"KrkrExtract", MB_OK);
						Button_SetCheck(hPSBRaw, BST_CHECKED);
						GlobalData::GetGlobalData()->AddPsbFlag(PSB_RAW);
					}
					else
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_TEXT);
					}
				}
				else
				{
					GlobalData::GetGlobalData()->AddPsbFlag(PSB_TEXT);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//Psb Anm
		case IDC_PSB_ANIM:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);

				BOOL Result;
				Result = SendMessageW(hPSBAnm, BM_GETCHECK, 0, 0);
				if (Result == FALSE)
				{
					BOOL SubResult[5] = { FALSE };
					HWND hPSBRaw   = GetItemX(IDC_PSB_RAW);
					HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
					HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
					HWND hPSBText  = GetItemX(IDC_PSB_TEXT);
					HWND hPSBAll   = GetItemX(IDC_PSB_ALL);

					SubResult[0] = SendMessageW(hPSBRaw,   BM_GETCHECK, 0, 0);
					SubResult[1] = SendMessageW(hPSBText,  BM_GETCHECK, 0, 0);
					SubResult[2] = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
					SubResult[3] = SendMessageW(hPSBAnm,   BM_GETCHECK, 0, 0);
					SubResult[4] = SendMessageW(hPSBAll,   BM_GETCHECK, 0, 0);

					if ((SubResult[0] + SubResult[1] + SubResult[2] + SubResult[3] + SubResult[4]) == 0)
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
						Button_SetCheck(hPSBDecom, BST_UNCHECKED);
						Button_SetCheck(hPSBText,  BST_UNCHECKED);
						Button_SetCheck(hPSBImage, BST_UNCHECKED);
						Button_SetCheck(hPSBAnm,   BST_UNCHECKED);
						Button_SetCheck(hPSBAll,   BST_UNCHECKED);

						MessageBoxW(hWnd, L"You must set one flag for this operation", L"KrkrExtract", MB_OK);
						Button_SetCheck(hPSBRaw, BST_CHECKED);
						GlobalData::GetGlobalData()->AddPsbFlag(PSB_RAW);
					}
					else
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ANM);
					}
				}
				else
				{
					GlobalData::GetGlobalData()->AddPsbFlag(PSB_ANM);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//Psb All
		case IDC_PSB_ALL:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hPSBRaw   = GetItemX(IDC_PSB_RAW);
				HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
				HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
				HWND hPSBText  = GetItemX(IDC_PSB_TEXT);
				HWND hPSBAnm   = GetItemX(IDC_PSB_ANIM);
				HWND hPSBAll   = GetItemX(IDC_PSB_ALL);

				BOOL Result;
				Result = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);

				//单独处理
				if (Result == TRUE)
				{
					Button_SetCheck(hPSBDecom, BST_CHECKED);
					Button_SetCheck(hPSBText,  BST_CHECKED);
					Button_SetCheck(hPSBImage, BST_CHECKED);
					Button_SetCheck(hPSBAnm,   BST_CHECKED);
					Button_SetCheck(hPSBAll,   BST_CHECKED);
					Button_SetCheck(hPSBRaw,   BST_CHECKED);
					GlobalData::GetGlobalData()->AddPsbFlag(PSB_ALL);
				}
				else
				{
					GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
					Button_SetCheck(hPSBDecom, BST_UNCHECKED);
					Button_SetCheck(hPSBText,  BST_UNCHECKED);
					Button_SetCheck(hPSBImage, BST_UNCHECKED);
					Button_SetCheck(hPSBAnm,   BST_UNCHECKED);
					Button_SetCheck(hPSBAll,   BST_UNCHECKED);

					Button_SetCheck(hPSBRaw,   BST_CHECKED);
					GlobalData::GetGlobalData()->AddPsbFlag(PSB_RAW);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//Text Raw
		case IDC_TEXT_RAW:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hTXTRaw = GetItemX(IDC_TEXT_RAW);
				BOOL Result;
				Result = SendMessageW(hTXTRaw, BM_GETCHECK, 0, 0);
				if (Result == TRUE)
				{
					GlobalData::GetGlobalData()->SetTextFlag(TEXT_RAW);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//Decode Text
		case IDC_TEXT_TEXT:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hTXTText = GetItemX(IDC_TEXT_TEXT);
				BOOL Result;
				Result = SendMessageW(hTXTText, BM_GETCHECK, 0, 0);
				if (Result == TRUE)
				{
					GlobalData::GetGlobalData()->SetTextFlag(TEXT_DECODE);
				}
			}
			break;
			default:
				break;
			}
		}
		break;
		
		//Tlg Raw
		case IDC_TLG_RAW:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hTLGRaw = GetItemX(IDC_TLG_RAW);
				BOOL Result;
				Result = SendMessageW(hTLGRaw, BM_GETCHECK, 0, 0);
				if (Result == TRUE)
				{
					GlobalData::GetGlobalData()->SetTlgFlag(TLG_RAW);
				}

			}
			break;
			default:
				break;
			}
		}
		break;

		//Tlg Build-in
		case IDC_TLG_BUILDIN:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hTLGBuildin = GetItemX(IDC_TLG_BUILDIN);

				BOOL Result;
				Result = SendMessageW(hTLGBuildin, BM_GETCHECK, 0, 0);
				if (Result == TRUE)
				{
					GlobalData::GetGlobalData()->SetTlgFlag(TLG_BUILDIN);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//System(Support all)
		case IDC_TLG_SYSTEM:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hTLGSys = GetItemX(IDC_TLG_SYSTEM);
				BOOL Result;
				Result = SendMessageW(hTLGSys, BM_GETCHECK, 0, 0);
				if (Result == TRUE)
				{
					GlobalData::GetGlobalData()->SetTlgFlag(TLG_SYS);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		case IDC_TLG_PNG:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hTLGSys = GetItemX(IDC_TLG_PNG);
				BOOL Result;
				Result = SendMessageW(hTLGSys, BM_GETCHECK, 0, 0);
				if (Result == TRUE)
				{
					GlobalData::GetGlobalData()->SetTlgFlag(TLG_PNG);
				}
			}
			break;
			default:
				break;
			}
		}
		break;
		
		//Folder select
		case IDC_PACK_BUTTON_FOLDER:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hFolderSel = GetItemX(IDC_PACK_BUTTON_FOLDER);

				WCHAR lpString[MAX_PATH];
				BROWSEINFOW FolderInfo = { 0 };
				FolderInfo.hwndOwner = hWnd;
				FolderInfo.lpszTitle = L"Select a folder to pack";
				FolderInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_UAHINT | BIF_NONEWFOLDERBUTTON;
				LPITEMIDLIST Dlist;
				Dlist = SHBrowseForFolderW(&FolderInfo);
				if (Dlist != nullptr)
				{
					SHGetPathFromIDListW(Dlist, lpString);
					GlobalData::GetGlobalData()->SetFolder(wstring(lpString));
					HWND hSubWnd = GetDlgItem(hWnd, IDC_PACK_EDIT_FOLDER);
					SetWindowTextW(hSubWnd, lpString);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//Ori Pack(Guess)
		case IDC_PACK_BUTTON_ORIPACK:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hOriPackSel = GetItemX(IDC_PACK_BUTTON_ORIPACK);

				WCHAR lpString[MAX_PATH] = { 0 };
				WCHAR CurDir[MAX_PATH] = { 0 };
				OPENFILENAMEW OpenInfo = { { sizeof(OPENFILENAMEW) } };
				OpenInfo.hwndOwner = hWnd;
				OpenInfo.lpstrFilter = L"XP3 File\0*.xp3";
				OpenInfo.lpstrFile = lpString;
				OpenInfo.nMaxFile = sizeof(lpString)/2;
				OpenInfo.lpstrTitle = L"Select an original xp3 file";

				GetCurrentDirectoryW(MAX_PATH, CurDir);
				OpenInfo.lpstrInitialDir = CurDir;
				OpenInfo.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
				BOOL Result = GetOpenFileNameW(&OpenInfo);
				if (Result)
				{
					GlobalData::GetGlobalData()->SetGuessPack(wstring(lpString));
					HWND hSubWnd = GetDlgItem(hWnd, IDC_PACK_EDIT_ORIPACK);
					SetWindowTextW(hSubWnd, lpString);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//Output Pack
		case IDC_PACK_BUTTON_OUTPACK:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hOutPackSel = GetItemX(IDC_PACK_BUTTON_OUTPACK);

				WCHAR lpString[MAX_PATH] = { 0 };
				WCHAR CurDir[MAX_PATH] = { 0 };
				OPENFILENAMEW OpenInfo = { { sizeof(OPENFILENAMEW) } };
				OpenInfo.hwndOwner = hWnd;
				OpenInfo.lpstrFilter = L"XP3 File\0*.xp3";
				OpenInfo.lpstrFile = lpString;
				OpenInfo.nMaxFile = sizeof(lpString)/2;
				OpenInfo.lpstrTitle = L"Output xp3 file";

				GetCurrentDirectoryW(MAX_PATH, CurDir);
				OpenInfo.lpstrInitialDir = CurDir;
				OpenInfo.Flags = OFN_EXPLORER;
				BOOL Result = GetOpenFileNameW(&OpenInfo);
				if (Result)
				{
					GlobalData::GetGlobalData()->SetOutputPack(wstring(lpString));
					HWND hSubWnd = GetDlgItem(hWnd, IDC_PACK_EDIT_OUTPACK);
					SetWindowTextW(hSubWnd, lpString);
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		//Make Package
		case IDC_PACK_BUTTON_MAKE:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hDoPack = GetItemX(IDC_BUTTON4);
				PluginHook::GetInstance()->StartPacker();
			}
			break;
			default:
				break;
			}
		}
		break;

		case IDC_BUTTON_DEBUGGER:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hDbg = GetItemX(IDC_BUTTON_DEBUGGER);
				if (GlobalData::GetGlobalData()->HasConsole)
				{
					if (FreeConsole())
					{
						SetWindowTextW(hDbg, LangOpenDbg[GetLangIndex()]);
						GlobalData::GetGlobalData()->HasConsole = FALSE;
					}
				}
				else
				{
					if (AllocConsole())
					{
						SetWindowTextW(hDbg, LangCloseDbg[GetLangIndex()]);
						GlobalData::GetGlobalData()->HasConsole = TRUE;
					}
				}
			}
			break;
			default:
				break;
			}
		}
		break;

		default:
			break;
		}
	}
	break;

	case WM_DROPFILES:
	{
		if (GlobalData::GetGlobalData()->isRunning == TRUE)
		{
			MessageBoxW(hWnd, L"Background tasks are under processing...\nPlease wait for a while", L"KrkrExtract", MB_OK);
			return TRUE;
		}

		HDROP hDrop = (HDROP)wParam;

		UINT nFileNum = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
		//Folder or File
		if (nFileNum == 1)
		{
			InvalidateRect(hWnd, NULL, TRUE);
			RtlZeroMemory(strFileName, MAX_PATH * 2);
			DragQueryFileW(hDrop, 0, strFileName, MAX_PATH);
			DragFinish(hDrop);

			ULONG FileAttr = GetFileAttributesW(strFileName);
			if (FileAttr == INVALID_FILE_ATTRIBUTES)
			{
				DragFinish(hDrop);
			}
			else
			{
				if (FileAttr & FILE_ATTRIBUTE_DIRECTORY)
				{
					HWND hFolderEdit = GetItemX(IDC_PACK_EDIT_FOLDER);
					SetWindowTextW(hFolderEdit, strFileName);
					GlobalData::GetGlobalData()->SetFolder(wstring(strFileName));

					HWND hOutEdit = GetItemX(IDC_PACK_EDIT_OUTPACK);
					wstring OutEditName = strFileName;
					OutEditName += L".xp3";
					SetWindowTextW(hOutEdit, OutEditName.c_str());
					GlobalData::GetGlobalData()->SetOutputPack(wstring(OutEditName));
				}
				else
				{
					PluginHook::GetInstance()->SetFile(strFileName);
					PluginHook::GetInstance()->BeginThread();
				}
			}
		}
		else
		{
			MessageBoxW(hWnd, L"Please Drop one file on this window", L"XP3Extract", MB_OK);
			DragFinish(hDrop);
		}
	}
	break;
	
	case WM_DESTROY:
		ExitProcess(0);
		break;
	break;
	}
	return 0;
}

HRESULT WINAPI PluginHook::InitWindow()
{
	WCHAR FullApplicationTitle[512] = { 0 };
	wsprintfW(FullApplicationTitle, this->szApplicationName, 
		_XP3ExtractVersion_, 
		GlobalData::GetGlobalData()->GetCompiledDate());

	WinText = FullApplicationTitle;

	this->Gui = CreateDialogParamW(
		GlobalData::GetGlobalData()->hSelfModule, 
		MAKEINTRESOURCEW(IDD_KrkrMainDlg),
		NULL, 
		(DLGPROC)DlgProc, 
		WM_INITDIALOG);

	if (!this->Gui)
	{
		MessageBoxW(NULL, L"Cannot create a the main window!", L"Error!", 0);
		return S_FALSE;
	}

	WinText = FullApplicationTitle;
	SetWindowTextW(this->Gui, FullApplicationTitle);
	DragAcceptFiles(this->Gui, TRUE);
	ShowWindow(this->Gui, SW_SHOW);
	UpdateWindow(this->Gui);

	MSG msg;
	while (GetMessageW(&msg, NULL, NULL, NULL))
	{
		if (msg.message == WM_KEYDOWN)
		{
			SendMessageW(this->Gui, msg.message, msg.wParam, msg.lParam);
		}
		else if (!IsDialogMessageW(this->Gui, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	return S_OK;
}



HRESULT WINAPI PluginHook::BeginThread()
{
	if (hExtractThread != INVALID_HANDLE_VALUE)
	{
		MessageBoxW(NULL, L"Extraction Thread is Not Free Now!", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}
	DWORD ThreadId = 0;
	hExtractThread = CreateThread(NULL, NULL, ExtractThread, this, NULL, &ThreadId);
	//ExtractThread(this);
	if (hExtractThread == INVALID_HANDLE_VALUE)
	{
		MessageBoxW(NULL, L"Failed to Start Extraction Thread!", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}
	return S_OK;
}

/***************************/
//Dumper

HRESULT WINAPI PluginHook::StartDumper()
{
	DisableAll(Gui);
	OutputString(L"Starting Dumper\n"); 
	ParseXP3File(this->wFileName);

	return DumpFile();
}


VOID WINAPI PluginHook::SetCurFile(DWORD iPos)
{
	WCHAR wBuf[MAX_PATH] = { 0 };

	wsprintfW(wBuf, L"Processing : [%d / %d]", iPos, CountFile);
	SetWindowTextW(this->Gui, wBuf);
	SetProcess(Gui, ((float)iPos / (float)CountFile) * 100);
}

VOID WINAPI PluginHook::SetCount(DWORD Count)
{
	this->CountFile = Count;
}


HRESULT WINAPI PluginHook::Reset()
{
	WCHAR FullApplicationTitle[512] = { 0 };

	RawExtract = FALSE;

	if (hExtractThread != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(hExtractThread, 500);
		TerminateThread(hThread, 0);
	}
	CloseHandle(hExtractThread);
	hExtractThread = INVALID_HANDLE_VALUE;

	ItemVector.clear();
	ItemNormalVector.clear();
	CountFile = 0;
	isM2Format = FALSE;
	RtlZeroMemory(wFileName, MAX_PATH * 2);

	
	wsprintfW(FullApplicationTitle, this->szApplicationName,
		_XP3ExtractVersion_, 
		GlobalData::GetGlobalData()->GetCompiledDate());

	SetWindowTextW(Gui, FullApplicationTitle);
	EnableAll(Gui);
	SetProcess(Gui, 0);
	return S_OK;
}


