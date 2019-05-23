#include "KrkrExtract.h"
#include "resource.h"
#include "FakePNG.h"
#include "Worker.h"
#include "hook.h"
#include "KrkrUniversalPatch.h"
#include "KrkrUniversalDumper.h"
#include <Shlobj.h>
#include <WindowsX.h>
#include "MultiThread.h"
#include "DebuggerHandler.h"

BOOL GlobalData::WindowIsInited = FALSE;

PVOID GetTVPCreateStreamCall();

NTSTATUS NTAPI InitExporter(iTVPFunctionExporter *exporter)
{
	BOOL                  Result;
	ULONG64               Crc;
	PVOID                 EncodedExporter;
	PVOID                 CreateStreamAddress;
	PBYTE                 PointRva;
	PBYTE                 ExecuteHandle;
	NtFileDisk            File;
	NTSTATUS              Status;


	LOOP_ONCE
	{
		Result = TVPInitImportStub(exporter);
		if (!Result)
			break;

		ExecuteHandle = (PBYTE)GetModuleHandleW(NULL);
		PointRva      = (PBYTE)exporter - (ULONG_PTR)ExecuteHandle;
		Crc = GenerationCRC64(0, (LPCBYTE)&PointRva, sizeof(PVOID));
		EncodedExporter = Nt_EncodePointer(PointRva, LoDword(Crc));
		CreateStreamAddress = (PBYTE)GetTVPCreateStreamCall() - (ULONG_PTR)ExecuteHandle;
		CreateStreamAddress = Nt_EncodePointer(CreateStreamAddress, HiDword(Crc));


		//lookup Image Entries to make sure CurrentFile is 

		Status = File.Create(L"KrkrExtract.db");
		if (NT_SUCCESS(Status))
		{
			File.Write(&Crc, sizeof(Crc));
			File.Write(&EncodedExporter, sizeof(EncodedExporter));
			File.Write(&CreateStreamAddress, sizeof(CreateStreamAddress));
			
			File.Close();
		}

		GlobalData::GetGlobalData()->InitedByModule = InitedByDllModule;
	}
	return Result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


HRESULT WINAPI InitExporterByExeModule(iTVPFunctionExporter *Exporter)
{
	NTSTATUS                    Status;
	
	LOOP_ONCE
	{
		Status  = STATUS_UNSUCCESSFUL;
		if (!Exporter)
			break;

		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"[exporter:exe]%p\n", Exporter);
		

		Status = InitExporter(Exporter);
		if (NT_FAILED(Status))
			break;

		GlobalData::GetGlobalData()->InitedByModule = InitedByExeModule;
		Status = STATUS_SUCCESS;
	}
	return Status;
}


//============================================

GlobalData* GlobalData::Handle = nullptr;

//============================================
//GlobalData

GlobalData::GlobalData() :
	PngFlag(PNG_RAW),
	TlgFlag(TLG_RAW),
	TextFlag(TEXT_RAW),
	PsbFlag(PSB_RAW),
	TjsFlag(TJS2_RAW),
	AmvFlag(AMV_RAW),
	//hImageModule(nullptr),
	InitedByModule(InitedByDllModule),
	hSelfModule(nullptr), //At DllMain
	hHostModule(nullptr),
	pfGlobalXP3Filter(nullptr),
	TVPFunctionExporter(nullptr),
	StubTVPCreateStream(NULL),
	IStreamAdapterVtable(NULL),
	StubHostAlloc(NULL),
	HasConsole(FALSE),
	isRunning(FALSE),
	DebugOn(FALSE),
	FakePngWorkerInited(FALSE),
	IsProtection(FALSE),
	InheritIcon(FALSE)
{
	hHostModule = (HMODULE)GetModuleHandleW(NULL);
	Inited = FALSE;

	RtlZeroMemory(GuessPack,    countof(GuessPack)    * sizeof(WCHAR));
	RtlZeroMemory(OutPack,      countof(OutPack)      * sizeof(WCHAR));
	RtlZeroMemory(Folder,       countof(Folder)       * sizeof(WCHAR));
	RtlZeroMemory(DragFileName, countof(DragFileName) * sizeof(WCHAR));
	RtlZeroMemory(WinText,      countof(WinText)      * sizeof(WCHAR));
	RtlZeroMemory(CurFileName,  countof(CurFileName)  * sizeof(WCHAR));
	
	StubGetProcAddress = NULL;
	StubCreateProcessInternalW = NULL;
	OriginalReturnAddress = NULL;
	FirstSectionAddress = NULL;
	SizeOfImage = NULL;
	StubV2Link = NULL;

	MainWindow = NULL;
	IsM2Format = FALSE;
	M2ChunkMagic = NULL;
	RawExtract = NULL;
	CountFile = 0;
	ItemVector.clear();
	WorkerThread = INVALID_HANDLE_VALUE;
	CurrentTempHandle = INVALID_HANDLE_VALUE;
	FakePngWorkerInited = FALSE;
	IsAllPackReaded = FALSE;

	IsSpcialChunkEncrypted = FALSE;
	SpecialChunkDecoder = NULL;
}


GlobalData::~GlobalData()
{
	ExitKrkr();
}


VOID NTAPI GlobalData::ExitKrkr()
{
	if (MainWindow)
		DestroyWindow(MainWindow);
	
	MainWindow = NULL;

	for (auto& Entry : SpecialChunkMap)
		if (Entry.second.Buffer)
			HeapFree(GetProcessHeap(), 0, Entry.second.Buffer);

	SpecialChunkMap.clear();
}

GlobalData* GlobalData::GetGlobalData()
{
	if (Handle == nullptr)
		Handle = new GlobalData;

	return Handle;
}

VOID NTAPI GlobalData::SetDllModule(HMODULE hModule)
{
	Handle->hSelfModule = hModule;

	RtlZeroMemory(SelfPath, MAX_PATH * 2);
	GetModuleFileNameW(hModule, SelfPath, MAX_PATH);
}

ULONG GlobalData::SetTextFlag(ULONG Flag)
{
	ULONG OldFlag  = Flag;
	TextFlag       = Flag;
	return OldFlag;
}

ULONG GlobalData::GetTextFlag()
{
	return TextFlag;
}

ULONG GlobalData::SetTlgFlag(ULONG Flag)
{
	ULONG OldFlag = TlgFlag;
	TlgFlag       = Flag;
	return OldFlag;
}

ULONG GlobalData::GetTlgFlag()
{
	return this->TlgFlag;
}

ULONG GlobalData::SetAmvFlag(ULONG Flag)
{
	ULONG OldFlag = AmvFlag;
	AmvFlag = Flag;
	return OldFlag;
}


ULONG GlobalData::GetAmvFlag()
{
	return this->AmvFlag;
}

ULONG GlobalData::SetTjsFlag(ULONG Flag)
{
	ULONG OldFlag = TjsFlag;
	TjsFlag = Flag;
	return OldFlag;
}

ULONG GlobalData::GetTjsFlag()
{
	return this->TjsFlag;
}


ULONG GlobalData::SetPngFlag(ULONG Flag)
{
	ULONG OldFlag = Flag;
	PngFlag       = Flag;
	return OldFlag;
}

ULONG GlobalData::GetPngFlag()
{
	return PngFlag;
}

ULONG GlobalData::AddPsbFlag(ULONG Flag)
{
	ULONG OldFlag = PsbFlag;
	SET_FLAG(PsbFlag, Flag);
	return OldFlag;
}

ULONG GlobalData::DeletePsbFlag(ULONG Flag)
{
	ULONG OldFlag = PsbFlag;
	CLEAR_FLAG(PsbFlag, Flag);
	return OldFlag;
}

BOOL GlobalData::PsbFlagOn(ULONG Flag)
{
	if (Flag == PSB_ALL)
		return FLAG_ON(this->PsbFlag, PSB_RAW)   &&
			   FLAG_ON(this->PsbFlag, PSB_TEXT)  &&
			   FLAG_ON(this->PsbFlag, PSB_DECOM) &&
			   FLAG_ON(this->PsbFlag, PSB_IMAGE) &&
			   FLAG_ON(this->PsbFlag, PSB_JSON)  &&
			   FLAG_ON(this->PsbFlag, PSB_ANM);
	else
		return FLAG_ON(this->PsbFlag, Flag);
}
;
VOID GlobalData::DebugPsbFlag()
{
	static WCHAR f[] = L"false";
	static WCHAR t[] = L"true";

	if (DebugOn)
		PrintConsoleW(L"Raw: %s, Text: %s, Decom : %s, Image : %s, Anm : %s\n",
		FLAG_ON(PsbFlag, PSB_RAW)   ? t : f,
		FLAG_ON(PsbFlag, PSB_TEXT)  ? t : f,
		FLAG_ON(PsbFlag, PSB_DECOM) ? t : f,
		FLAG_ON(PsbFlag, PSB_IMAGE) ? t : f,
		FLAG_ON(PsbFlag, PSB_ANM)   ? t : f);
}

VOID GlobalData::SetFolder(LPCWSTR Name)
{
	RtlZeroMemory(Folder, countof(Folder) * sizeof(WCHAR));
	lstrcpyW(Folder, Name);
}

VOID GlobalData::SetGuessPack(LPCWSTR Name)
{
	RtlZeroMemory(GuessPack, countof(GuessPack) * sizeof(WCHAR));
	lstrcpyW(GuessPack, Name);
}

VOID GlobalData::SetOutputPack(LPCWSTR Name)
{
	RtlZeroMemory(OutPack, countof(OutPack) * sizeof(WCHAR));
	lstrcpyW(OutPack, Name);
}

VOID GlobalData::GetFolder(LPWSTR Name, ULONG BufferMaxLength)
{
	RtlZeroMemory(Name, BufferMaxLength * sizeof(WCHAR));
	lstrcpyW(Name, Folder);
}

VOID GlobalData::GetGuessPack(LPWSTR Name, ULONG BufferMaxLength)
{
	RtlZeroMemory(Name, BufferMaxLength * sizeof(WCHAR));
	lstrcpyW(Name, GuessPack);
}

VOID GlobalData::GetOutputPack(LPWSTR Name, ULONG BufferMaxLength)
{
	RtlZeroMemory(Name, BufferMaxLength * sizeof(WCHAR));
	lstrcpyW(Name, OutPack);
}


BOOL GlobalData::FindCodeSlow(PCCHAR Start, ULONG Size, PCCHAR Pattern, ULONG PatternLen)
{
	return KMP(Start, Size, Pattern, PatternLen) ? TRUE : FALSE;
	return TRUE;
}

tTJSCriticalSection  CS; //Entry CS

VOID GlobalData::AddFileEntry(PCWSTR FileName, ULONG Length)
{
	BOOL                 FindDot;

	if (!FileName || !Length)
		return;

	//section holder, krkr may initialize package by multi-thread in the future.
	tTJSCriticalSectionHolder CsHolder(CS);
	FindDot = FALSE;
	for (ULONG i = 0; i < Length; i++)
	{
		if (FileName[i] == L'.')
		{
			FindDot = TRUE;
			break;
		}
	}

	if (FindDot)
		FileNameList.push_back(std::wstring(FileName, Length));
}

LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

VOID HookMessageKeyDown(MSG* msg)
{
	HWND  hCmd;
	WCHAR CmdLine[1000];

	if (msg->message != WM_KEYDOWN)
		return;

	hCmd = GetDlgItem(GlobalData::GetGlobalData()->MainWindow, IDC_CMD);
	if (hCmd == NULL || hCmd != msg->hwnd)
		return;

	if (msg->wParam == VK_RETURN)
	{
		RtlZeroMemory(CmdLine, sizeof(CmdLine));
		GetWindowTextW(hCmd, CmdLine, countof(CmdLine));
		SetWindowTextW(hCmd, L"");
		ParseCommand(CmdLine);
	}
}


NTSTATUS NTAPI GlobalData::InitWindow()
{
	NTSTATUS       Status;
	WCHAR          FullApplicationTitle[512];
	MSG            msg;

	RtlZeroMemory(FullApplicationTitle, countof(FullApplicationTitle) * sizeof(WCHAR));
	RtlZeroMemory(WinText, countof(WinText) * sizeof(WCHAR));

	wsprintfW(FullApplicationTitle, szApplicationName, _XP3ExtractVersion_, MAKE_WSTRING(__DATE__) L" " MAKE_WSTRING(__TIME__));
	lstrcpyW(WinText, FullApplicationTitle);

	LOOP_ONCE
	{
		Status = STATUS_UNSUCCESSFUL;

		MainWindow = CreateDialogParamW(
		hSelfModule,
		MAKEINTRESOURCEW(IDD_KrkrMainDlg),
		NULL,
		(DLGPROC)DlgProc,
		WM_INITDIALOG);

		if (!MainWindow)
		{
			MessageBoxW(NULL, L"Cannot create a the main window!", L"Error!", 0);
			break;
		}

		if (DebugOn)
			PrintConsoleW(L"Init Main Window...\n");

		SetWindowTextW(MainWindow, FullApplicationTitle);
		DragAcceptFiles(MainWindow, TRUE);
		ShowWindow(MainWindow, SW_SHOW);
		UpdateWindow(MainWindow);

		InterlockedExchange((PLONG)&GlobalData::WindowIsInited, TRUE);
		VirtualConsolePrint(
			L"[+] Virtual Console initializated, type `help` to see list of commands");

		Status = STATUS_SUCCESS;

		while (GetMessageW(&msg, NULL, NULL, NULL))
		{
			if (!IsDialogMessageW(MainWindow, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
			else
			{
				if (msg.message == WM_KEYDOWN)
				{
					HookMessageKeyDown(&msg);
				}
				SendMessageW(MainWindow, msg.message, msg.wParam, msg.lParam);
			}
		}
	}
	return Status;
}


HRESULT WINAPI HookV2Link(iTVPFunctionExporter *exporter)
{
	GlobalData*    Handle;

	Handle = GlobalData::GetGlobalData();
	InitExporter(exporter);
	ImageWorkerV2Link(exporter);
	Handle->TVPFunctionExporter = exporter;

	if (!Handle->FakePngWorkerInited)
	{
		InitLayer();
		Handle->FakePngWorkerInited = TRUE;
	}
	return Handle->StubV2Link(exporter);
}


DWORD WINAPI InitWindowThread(LPVOID lpParam)
{
	GlobalData* o = (GlobalData*)lpParam;
	return o->InitWindow();
}

NTSTATUS NTAPI GlobalData::InitHookNull()
{
	NTSTATUS Status;

	if (Inited == TRUE)
		return STATUS_SUCCESS;

	LOOP_ONCE
	{
		if (DebugOn)
			PrintConsoleW(L"Init hook null...\n");
		Status = Nt_CreateThread(InitWindowThread, this);
		if (NT_FAILED(Status))
			break;

		Inited = TRUE;
	}
	return Status;
}

NTSTATUS NTAPI GlobalData::InitHook(LPCWSTR ModuleName, PVOID ImageBase)
{
	NTSTATUS    Status;
	ULONG_PTR   Length;
	ULONG64     Extension;
	PVOID       pV2Link;

	LOOP_ONCE
	{
		Status = STATUS_ALREADY_REGISTERED;
		if (Inited == TRUE)
			break;

		Status = STATUS_UNSUCCESSFUL;
		if (ImageBase == NULL)
			break;

		Length = lstrlenW(ModuleName);
		if (Length <= 4)
			break;

		Extension = *(PULONG64)&ModuleName[Length - 4];
		
		if (Extension != TAG4W('.dll') && Extension != TAG4W('.tpm'))
			break;

		if (Nt_GetProcAddress(ImageBase, "FlushInstructionCache"))
			break;

		pV2Link = Nt_GetProcAddress(ImageBase, "V2Link");
		if (pV2Link == NULL)
			break;

		Mp::PATCH_MEMORY_DATA f[] =
		{
			Mp::FunctionJumpVa(pV2Link, HookV2Link, &StubV2Link)
		};

		Status = Mp::PatchMemory(f, countof(f));

		Status = Nt_CreateThread(InitWindowThread, this);
		Inited = TRUE;
	}
	return Status;
}

VOID NTAPI GlobalData::SetCurFile(DWORD iPos)
{
	WCHAR         TextBuffer[MAX_PATH];

	RtlZeroMemory(TextBuffer, countof(TextBuffer) * sizeof(WCHAR));

	wsprintfW(TextBuffer, L"Processing : [%d / %d]", iPos, CountFile);
	SetWindowTextW(MainWindow, TextBuffer);
	SetProcess(MainWindow, (ULONG)(((float)iPos / (float)CountFile) * 100.0));
}

VOID NTAPI GlobalData::SetCount(DWORD Count)
{
	CountFile = Count;
}


VOID NTAPI GlobalData::Reset()
{
	WCHAR FullApplicationTitle[512];

	RtlZeroMemory(FullApplicationTitle, countof(FullApplicationTitle) * sizeof(WCHAR));
	ItemVector.clear();
	CountFile = 0;
	IsM2Format = FALSE;
	RtlZeroMemory(CurFileName, countof(CurFileName) * sizeof(WCHAR));

	wsprintfW(FullApplicationTitle, szApplicationName, _XP3ExtractVersion_, MAKE_WSTRING(__DATE__) L" " MAKE_WSTRING(__TIME__));

	SetWindowTextW(MainWindow, FullApplicationTitle);
	EnableAll(MainWindow);
	SetProcess(MainWindow, 0);
	isRunning = FALSE;
}

//在每一次进行操作之前
VOID NTAPI GlobalData::ForceReset()
{
	DWORD         ExitCode;
	WCHAR         FullApplicationTitle[512];
	LARGE_INTEGER Timeout;

	RtlZeroMemory(FullApplicationTitle, countof(FullApplicationTitle) * sizeof(WCHAR));

	RawExtract = FALSE;

	if (WorkerThread!= INVALID_HANDLE_VALUE)
	{
		ExitCode = STILL_ACTIVE;
		if (GetExitCodeThread(WorkerThread, &ExitCode) == STILL_ACTIVE)
		{
			Timeout.QuadPart = 500;
			NtWaitForSingleObject(WorkerThread, TRUE, &Timeout);
			if (GetExitCodeThread(WorkerThread, &ExitCode) == STILL_ACTIVE)
				TerminateThread(WorkerThread, 0);
		}
	}
	NtClose(WorkerThread);
	WorkerThread = INVALID_HANDLE_VALUE;

	ItemVector.clear();
	CountFile = 0;
	IsM2Format = FALSE;
	RtlZeroMemory(CurFileName, countof(CurFileName) * sizeof(WCHAR));

	wsprintfW(FullApplicationTitle, szApplicationName, _XP3ExtractVersion_, MAKE_WSTRING(__DATE__) L" " MAKE_WSTRING(__TIME__));

	SetWindowTextW(MainWindow, FullApplicationTitle);
	EnableAll(MainWindow);
	SetProcess(MainWindow, 0);
	isRunning = FALSE;
}



/*************************************/
//GUI

#define GetItemX(x)    GetDlgItem(hWnd, x)
#define EnableX(x)     EnableWindow(x, TRUE)
#define DisableX(x)    EnableWindow(x, FALSE)
#define CheckItem(x)   SendMessageW(x, BM_SETCHECK, BST_UNCHECKED, 0)
#define UnCheckItem(x) SendMessageW(x, BM_SETCHECK, BST_CHECKED, 0)

VOID WINAPI GlobalData::EnableAll(HWND hWnd)
{
	HWND hPNGRaw = GetItemX(IDC_PNG_RAW);
	HWND hPNGSys = GetItemX(IDC_PNG_SYSTEM);

	HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
	HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
	HWND hPSBText = GetItemX(IDC_PSB_TEXT);
	HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
	HWND hPSBJson = GetItemX(IDC_PSB_JSON);
	HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
	HWND hPSBAll = GetItemX(IDC_PSB_ALL);

	HWND hTXTRaw = GetItemX(IDC_TEXT_RAW);
	HWND hTXTText = GetItemX(IDC_TEXT_TEXT);

	HWND hTLGRaw = GetItemX(IDC_TLG_RAW);
	HWND hTLGBuildin = GetItemX(IDC_TLG_BUILDIN);
	HWND hTLGSys = GetItemX(IDC_TLG_SYSTEM);
	HWND hTLGPng = GetItemX(IDC_TLG_PNG);
	HWND hTLGJpg = GetItemX(IDC_TLG_JPG);

	HWND hIcon = GetItemX(IDC_PACK_SAME_ICON);
	HWND hProtect = GetItemX(IDC_PACK_ADD_PROTECT);

	HWND hFolderEdit = GetItemX(IDC_PACK_EDIT_FOLDER);
	HWND hOriPackEdit = GetItemX(IDC_PACK_EDIT_ORIPACK);
	HWND hOutPackEdit = GetItemX(IDC_PACK_EDIT_OUTPACK);

	HWND hFolderSel = GetItemX(IDC_PACK_BUTTON_FOLDER);
	HWND hOriPackSel = GetItemX(IDC_PACK_BUTTON_ORIPACK);
	HWND hOutPackSel = GetItemX(IDC_PACK_BUTTON_OUTPACK);

	HWND hDoPack = GetItemX(IDC_PACK_BUTTON_MAKE);
	HWND hDoUPatch = GetItemX(IDC_PACK_BUTTON_UMAKE);
	HWND hProcess = GetItemX(IDC_PROGRESS1);
	HWND hDbg = GetItemX(IDC_BUTTON_DEBUGGER);

	HWND hTJS2Raw = GetItemX(IDC_TJS2_RAW);
	HWND hTJS2Decom = GetItemX(IDC_TJS2_Decomp);

	HWND hAMVJpg = GetItemX(IDC_AMV_FRAME);
	HWND hAMVPng = GetItemX(IDC_AMV_PNG);
	HWND hAMVMng = GetItemX(IDC_AMV_MNG);

	EnableX(hAMVJpg);
	EnableX(hAMVPng);
	EnableX(hAMVMng);

	EnableX(hTJS2Raw);
	EnableX(hTJS2Decom);

	EnableX(hIcon);
	EnableX(hProtect);

	EnableX(hPNGRaw);
	EnableX(hPNGSys);

	EnableX(hDoUPatch);

	EnableX(hPSBRaw);
	EnableX(hPSBDecom);
	EnableX(hPSBText);
	EnableX(hPSBImage);
	EnableX(hPSBJson);
	EnableX(hPSBAnm);
	EnableX(hPSBAll);

	EnableX(hTXTRaw);
	EnableX(hTXTText);

	EnableX(hTLGRaw);
	EnableX(hTLGBuildin);
	EnableX(hTLGSys);
	EnableX(hTLGPng);
	EnableX(hTLGJpg);

	EnableX(hFolderEdit);
	EnableX(hOriPackEdit);
	EnableX(hOutPackEdit);

	EnableX(hFolderSel);
	EnableX(hOriPackSel);
	EnableX(hOutPackSel);

	EnableX(hDoPack);

	HWND hUButton = GetItemX(IDC_BUTTON_UUPCK);
	if (GlobalData::GetGlobalData()->IsAllPackReaded)
		EnableX(hUButton);
	else
		DisableX(hUButton);
}

VOID NTAPI GlobalData::DisableAll(HWND hWnd)
{
	HWND hPNGRaw = GetItemX(IDC_PNG_RAW);
	HWND hPNGSys = GetItemX(IDC_PNG_SYSTEM);

	HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
	HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
	HWND hPSBText = GetItemX(IDC_PSB_TEXT);
	HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
	HWND hPSBJson = GetItemX(IDC_PSB_JSON);
	HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
	HWND hPSBAll = GetItemX(IDC_PSB_ALL);

	HWND hTXTRaw = GetItemX(IDC_TEXT_RAW);
	HWND hTXTText = GetItemX(IDC_TEXT_TEXT);

	HWND hTLGRaw = GetItemX(IDC_TLG_RAW);
	HWND hTLGBuildin = GetItemX(IDC_TLG_BUILDIN);
	HWND hTLGSys = GetItemX(IDC_TLG_SYSTEM);
	HWND hTLGPng = GetItemX(IDC_TLG_PNG);
	HWND hTLGJpg = GetItemX(IDC_TLG_JPG);

	HWND hFolderEdit = GetItemX(IDC_PACK_EDIT_FOLDER);
	HWND hOriPackEdit = GetItemX(IDC_PACK_EDIT_ORIPACK);
	HWND hOutPackEdit = GetItemX(IDC_PACK_EDIT_OUTPACK);

	HWND hFolderSel = GetItemX(IDC_PACK_BUTTON_FOLDER);
	HWND hOriPackSel = GetItemX(IDC_PACK_BUTTON_ORIPACK);
	HWND hOutPackSel = GetItemX(IDC_PACK_BUTTON_OUTPACK);

	HWND hDoPack = GetItemX(IDC_PACK_BUTTON_MAKE);
	HWND hDoUPatch = GetItemX(IDC_PACK_BUTTON_UMAKE);
	HWND hProcess = GetItemX(IDC_PROGRESS1);
	HWND hDbg = GetItemX(IDC_BUTTON_DEBUGGER);


	HWND hIcon = GetItemX(IDC_PACK_SAME_ICON);
	HWND hProtect = GetItemX(IDC_PACK_ADD_PROTECT);


	HWND hTJS2Raw = GetItemX(IDC_TJS2_RAW);
	HWND hTJS2Decom = GetItemX(IDC_TJS2_Decomp);


	HWND hAMVJpg = GetItemX(IDC_AMV_FRAME);
	HWND hAMVPng = GetItemX(IDC_AMV_PNG);
	HWND hAMVMng = GetItemX(IDC_AMV_MNG);

	DisableX(hAMVJpg);
	DisableX(hAMVPng);
	DisableX(hAMVMng);

	DisableX(hTJS2Raw);
	DisableX(hTJS2Decom);


	DisableX(hIcon);
	DisableX(hProcess);

	DisableX(hPNGRaw);
	DisableX(hPNGSys); 

	DisableX(hDoUPatch);

	DisableX(hPSBRaw);
	DisableX(hPSBDecom);
	DisableX(hPSBText);
	DisableX(hPSBImage);
	DisableX(hPSBJson);
	DisableX(hPSBAnm);
	DisableX(hPSBAll);

	DisableX(hTXTRaw);
	DisableX(hTXTText);

	DisableX(hTLGRaw);
	DisableX(hTLGBuildin);
	DisableX(hTLGSys);
	DisableX(hTLGPng);
	DisableX(hTLGJpg);

	DisableX(hFolderEdit);
	DisableX(hOriPackEdit);
	DisableX(hOutPackEdit);

	DisableX(hFolderSel);
	DisableX(hOriPackSel);
	DisableX(hOutPackSel);

	DisableX(hDoPack);

	HWND hUButton = GetItemX(IDC_BUTTON_UUPCK);
	DisableX(hUButton);
}



VOID WINAPI GlobalData::AdjustCP(HWND hWnd)
{
}

VOID WINAPI GlobalData::SetProcess(HWND hWnd, ULONG Value)
{
	HWND hProcess = GetItemX(IDC_PROGRESS1);
	SendMessageW(hProcess, PBM_SETPOS, Value, 0);
}

VOID GlobalData::VirtualConsolePrint(PCWSTR Format, ...)
{
	ULONG       Length;
	WCHAR       Buffer[0xF00 / 2];
	va_list     Args;
	HWND        VirtualConsole;

	if (MainWindow == NULL)
		return;

	va_start(Args, Format);
	Length = _vsnwprintf(Buffer, countof(Buffer) - 1, Format, Args);
	if (Length == -1)
		return;
	
	VirtualConsole = GetDlgItem(MainWindow, IDC_VCONSOLE);
	if (VirtualConsole)
		ListBox_AddString(VirtualConsole, Buffer);
}

LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int         wmId, wmEvent;
	PAINTSTRUCT Painter;

	UNREFERENCED_PARAMETER(Painter);

	switch (msg)
	{
	case WM_INITDIALOG:
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


		HWND hIcon = GetItemX(IDC_PACK_SAME_ICON);
		HWND hProtect = GetItemX(IDC_PACK_ADD_PROTECT);

		HWND hTLGRaw = GetItemX(IDC_TLG_RAW);
		HWND hTLGBuildin = GetItemX(IDC_TLG_BUILDIN);
		HWND hTLGSys = GetItemX(IDC_TLG_SYSTEM);
		HWND hTLGPng = GetItemX(IDC_TLG_PNG);
		HWND hTLGJpg = GetItemX(IDC_TLG_JPG);

		HWND hTJSRaw = GetItemX(IDC_TJS2_RAW);
		HWND hTJSDecom = GetItemX(IDC_TJS2_Decomp);

		HWND hAmvRaw = GetItemX(IDC_AMV_RAW);
		HWND hAmvPNG = GetItemX(IDC_AMV_PNG);
		HWND hAmvJPG = GetItemX(IDC_AMV_FRAME);
		HWND hAmvMNG = GetItemX(IDC_AMV_MNG);

		HWND hFolderEdit = GetItemX(IDC_PACK_EDIT_FOLDER);
		HWND hOriPackEdit = GetItemX(IDC_PACK_EDIT_ORIPACK);
		HWND hOutPackEdit = GetItemX(IDC_PACK_EDIT_OUTPACK);

		HWND hFolderSel = GetItemX(IDC_PACK_BUTTON_FOLDER);
		HWND hOriPackSel = GetItemX(IDC_PACK_BUTTON_ORIPACK);
		HWND hOutPackSel = GetItemX(IDC_PACK_BUTTON_OUTPACK);

		HWND hDoPack = GetItemX(IDC_PACK_BUTTON_MAKE);
		HWND hDoUPatch = GetItemX(IDC_PACK_BUTTON_UMAKE);
		HWND hProcess = GetItemX(IDC_PROGRESS1);
		HWND hDbg = GetItemX(IDC_BUTTON_DEBUGGER);

		EnableX(hIcon);
		EnableX(hProtect);

		EnableX(hPNGRaw);
		EnableX(hPNGSys);

		EnableX(hDoUPatch);

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
		EnableX(hTLGJpg);

		EnableX(hFolderEdit);
		EnableX(hOriPackEdit);
		EnableX(hOutPackEdit);

		EnableX(hFolderSel);
		EnableX(hOriPackSel);
		EnableX(hOutPackSel);

		EnableX(hDoPack);
		EnableX(hProcess);
		EnableX(hDbg);

		GlobalData::GetGlobalData()->AdjustCP(hWnd);

		Button_SetCheck(hPNGRaw, BST_CHECKED);
		Button_SetCheck(hPNGSys, BST_UNCHECKED);
		GlobalData::GetGlobalData()->SetPngFlag(PNG_RAW);

		Button_SetCheck(hPSBRaw, BST_CHECKED);
		Button_SetCheck(hPSBDecom, BST_UNCHECKED);
		Button_SetCheck(hPSBText, BST_UNCHECKED);
		Button_SetCheck(hPSBImage, BST_UNCHECKED);
		Button_SetCheck(hPSBAnm, BST_UNCHECKED);
		Button_SetCheck(hPSBAll, BST_UNCHECKED);
		GlobalData::GetGlobalData()->AddPsbFlag(PSB_RAW);

		Button_SetCheck(hTXTRaw, BST_CHECKED);
		Button_SetCheck(hTXTText, BST_UNCHECKED);
		GlobalData::GetGlobalData()->SetTextFlag(TEXT_RAW);

		Button_SetCheck(hTLGRaw, BST_CHECKED);
		Button_SetCheck(hTLGBuildin, BST_UNCHECKED);
		Button_SetCheck(hTLGSys, BST_UNCHECKED);
		Button_SetCheck(hTLGPng, BST_UNCHECKED);
		Button_SetCheck(hTLGJpg, BST_UNCHECKED);
		GlobalData::GetGlobalData()->SetTlgFlag(TLG_RAW);

		Button_SetCheck(hTJSRaw,   BST_CHECKED);
		Button_SetCheck(hTJSDecom, BST_UNCHECKED);
		GlobalData::GetGlobalData()->SetTjsFlag(TJS2_RAW);


		Button_SetCheck(hAmvRaw,   BST_CHECKED);
		Button_SetCheck(hAmvPNG, BST_UNCHECKED);
		Button_SetCheck(hAmvJPG, BST_UNCHECKED);
		Button_SetCheck(hAmvMNG, BST_UNCHECKED);
		GlobalData::GetGlobalData()->SetAmvFlag(AMV_RAW);

		HWND hUButton = GetItemX(IDC_BUTTON_UUPCK);
		if (GlobalData::GetGlobalData()->IsAllPackReaded)
			EnableX(hUButton);
		else
			DisableX(hUButton);


		WCHAR      DataName[MAX_PATH];
		NtFileDisk File;

		RtlZeroMemory(DataName, countof(DataName) * sizeof(WCHAR));
		GetCurrentDirectoryW(MAX_PATH, DataName);
		lstrcatW(DataName, L"\\data.xp3");
		if (NT_SUCCESS(File.Open(DataName)))
		{
			GlobalData::GetGlobalData()->SetGuessPack(DataName);
			SetWindowTextW(hOriPackEdit, DataName);
			File.Close();
		}
		SendMessageW(hProcess, PBM_SETRANGE, (WPARAM)0, (LPARAM)(MAKELPARAM(0, 100)));
		SendMessageW(hProcess, PBM_SETSTEP, 1, 0);
	}
	break;

	case WM_SYSCOMMAND:
	{
		switch (wParam)
		{
		case SC_CLOSE:
			ExitProcess(0);
			break;
		}
	}
	break;

	case WM_UUPAK_OK:
	{
		HWND hButton = GetItemX(IDC_BUTTON_UUPCK);
		if (hButton)
			EnableX(hButton);
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
				WCHAR lpString[1024];
				RtlZeroMemory(lpString, sizeof(lpString));
				GetWindowTextW(hFolderEdit, lpString, 1024);
				GlobalData::GetGlobalData()->SetFolder(lpString);
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
				WCHAR lpString[1024];
				RtlZeroMemory(lpString, sizeof(lpString));
				GetWindowTextW(hOriPack, lpString, 1024);
				GlobalData::GetGlobalData()->SetGuessPack(lpString);
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
				WCHAR lpString[1024];
				RtlZeroMemory(lpString, sizeof(lpString));
				GetWindowTextW(hOutPack, lpString, 1024);
				GlobalData::GetGlobalData()->SetOutputPack(lpString);
			}
			break;
			default:
				break;
			}
		}
		break;

		case IDC_BUTTON_UUPCK:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				if (GlobalData::GetGlobalData()->IsAllPackReaded)
				{
					GlobalData::GetGlobalData()->WorkerThread = StartUniversalDumper();
				}
			}
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
					BOOL SubResult[7];

					RtlZeroMemory(SubResult, sizeof(SubResult));
					HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
					HWND hPSBText = GetItemX(IDC_PSB_TEXT);
					HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
					HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
					HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
					HWND hPSBAll = GetItemX(IDC_PSB_ALL);
					HWND hPSBJosn = GetItemX(IDC_PSB_JSON);

					SubResult[0] = SendMessageW(hPSBRaw, BM_GETCHECK, 0, 0);
					SubResult[1] = SendMessageW(hPSBText, BM_GETCHECK, 0, 0);
					SubResult[2] = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
					SubResult[3] = SendMessageW(hPSBAnm, BM_GETCHECK, 0, 0);
					SubResult[4] = SendMessageW(hPSBJosn, BM_GETCHECK, 0, 0);
					SubResult[5] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);
					SubResult[6] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);


					if ((SubResult[0] + SubResult[1] + SubResult[2] + SubResult[3] + SubResult[4] + SubResult[5] + SubResult[6]) == 0)
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
						Button_SetCheck(hPSBRaw, BST_UNCHECKED);
						Button_SetCheck(hPSBText, BST_UNCHECKED);
						Button_SetCheck(hPSBImage, BST_UNCHECKED);
						Button_SetCheck(hPSBAnm, BST_UNCHECKED);
						Button_SetCheck(hPSBAll, BST_UNCHECKED);
						Button_SetCheck(hPSBJosn, BST_UNCHECKED);

						MessageBoxW(hWnd, L"You must set one flag for this operation", L"KrkrExtract", MB_OK);
						Button_SetCheck(hPSBRaw, BST_CHECKED);
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
					BOOL SubResult[7];

					RtlZeroMemory(SubResult, sizeof(SubResult));
					HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
					HWND hPSBText = GetItemX(IDC_PSB_TEXT);
					HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
					HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
					HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
					HWND hPSBAll = GetItemX(IDC_PSB_ALL);
					HWND hPSBJosn = GetItemX(IDC_PSB_JSON);

					SubResult[0] = SendMessageW(hPSBRaw, BM_GETCHECK, 0, 0);
					SubResult[1] = SendMessageW(hPSBText, BM_GETCHECK, 0, 0);
					SubResult[2] = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
					SubResult[3] = SendMessageW(hPSBAnm, BM_GETCHECK, 0, 0);
					SubResult[4] = SendMessageW(hPSBJosn, BM_GETCHECK, 0, 0);
					SubResult[5] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);
					SubResult[6] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);


					if ((SubResult[0] + SubResult[1] + SubResult[2] + SubResult[3] + SubResult[4] + SubResult[5] + SubResult[6]) == 0)
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
						Button_SetCheck(hPSBRaw, BST_UNCHECKED);
						Button_SetCheck(hPSBText, BST_UNCHECKED);
						Button_SetCheck(hPSBImage, BST_UNCHECKED);
						Button_SetCheck(hPSBAnm, BST_UNCHECKED);
						Button_SetCheck(hPSBAll, BST_UNCHECKED);
						Button_SetCheck(hPSBJosn, BST_UNCHECKED);

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
			}
		}
		break;


		//psb json decompiler
		case IDC_PSB_JSON:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hPSBJosn = GetItemX(IDC_PSB_JSON);

				BOOL Result;
				Result = SendMessageW(hPSBJosn, BM_GETCHECK, 0, 0);
				if (Result == FALSE)
				{
					BOOL SubResult[7];

					RtlZeroMemory(SubResult, sizeof(SubResult));
					HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
					HWND hPSBText = GetItemX(IDC_PSB_TEXT);
					HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
					HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
					HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
					HWND hPSBAll = GetItemX(IDC_PSB_ALL);

					SubResult[0] = SendMessageW(hPSBRaw, BM_GETCHECK, 0, 0);
					SubResult[1] = SendMessageW(hPSBText, BM_GETCHECK, 0, 0);
					SubResult[2] = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
					SubResult[3] = SendMessageW(hPSBAnm, BM_GETCHECK, 0, 0);
					SubResult[4] = SendMessageW(hPSBJosn, BM_GETCHECK, 0, 0);
					SubResult[5] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);
					SubResult[6] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);


					if ((SubResult[0] + SubResult[1] + SubResult[2] + SubResult[3] + SubResult[4] + SubResult[5] + SubResult[6]) == 0)
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
						Button_SetCheck(hPSBRaw, BST_UNCHECKED);
						Button_SetCheck(hPSBText, BST_UNCHECKED);
						Button_SetCheck(hPSBImage, BST_UNCHECKED);
						Button_SetCheck(hPSBAnm, BST_UNCHECKED);
						Button_SetCheck(hPSBAll, BST_UNCHECKED);
						Button_SetCheck(hPSBJosn, BST_UNCHECKED);

						MessageBoxW(hWnd, L"You must set one flag for this operation", L"KrkrExtract", MB_OK);
						Button_SetCheck(hPSBRaw, BST_CHECKED);
						GlobalData::GetGlobalData()->AddPsbFlag(PSB_RAW);
					}
					else
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_JSON);
					}
				}
				else
				{
					GlobalData::GetGlobalData()->AddPsbFlag(PSB_JSON);
				}
			}
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
					BOOL SubResult[7];

					RtlZeroMemory(SubResult, sizeof(SubResult));
					HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
					HWND hPSBText = GetItemX(IDC_PSB_TEXT);
					HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
					HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
					HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
					HWND hPSBAll = GetItemX(IDC_PSB_ALL);
					HWND hPSBJosn = GetItemX(IDC_PSB_JSON);

					SubResult[0] = SendMessageW(hPSBRaw, BM_GETCHECK, 0, 0);
					SubResult[1] = SendMessageW(hPSBText, BM_GETCHECK, 0, 0);
					SubResult[2] = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
					SubResult[3] = SendMessageW(hPSBAnm, BM_GETCHECK, 0, 0);
					SubResult[4] = SendMessageW(hPSBJosn, BM_GETCHECK, 0, 0);
					SubResult[5] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);
					SubResult[6] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);


					if ((SubResult[0] + SubResult[1] + SubResult[2] + SubResult[3] + SubResult[4] + SubResult[5] + SubResult[6]) == 0)
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
						Button_SetCheck(hPSBRaw, BST_UNCHECKED);
						Button_SetCheck(hPSBText, BST_UNCHECKED);
						Button_SetCheck(hPSBImage, BST_UNCHECKED);
						Button_SetCheck(hPSBAnm, BST_UNCHECKED);
						Button_SetCheck(hPSBAll, BST_UNCHECKED);
						Button_SetCheck(hPSBJosn, BST_UNCHECKED);

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
					BOOL SubResult[7];

					RtlZeroMemory(SubResult, sizeof(SubResult));
					HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
					HWND hPSBText = GetItemX(IDC_PSB_TEXT);
					HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
					HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
					HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
					HWND hPSBAll = GetItemX(IDC_PSB_ALL);
					HWND hPSBJosn = GetItemX(IDC_PSB_JSON);

					SubResult[0] = SendMessageW(hPSBRaw, BM_GETCHECK, 0, 0);
					SubResult[1] = SendMessageW(hPSBText, BM_GETCHECK, 0, 0);
					SubResult[2] = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
					SubResult[3] = SendMessageW(hPSBAnm, BM_GETCHECK, 0, 0);
					SubResult[4] = SendMessageW(hPSBJosn, BM_GETCHECK, 0, 0);
					SubResult[5] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);
					SubResult[6] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);


					if ((SubResult[0] + SubResult[1] + SubResult[2] + SubResult[3] + SubResult[4] + SubResult[5] + SubResult[6]) == 0)
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
						Button_SetCheck(hPSBRaw,   BST_UNCHECKED);
						Button_SetCheck(hPSBText,  BST_UNCHECKED);
						Button_SetCheck(hPSBImage, BST_UNCHECKED);
						Button_SetCheck(hPSBAnm,   BST_UNCHECKED);
						Button_SetCheck(hPSBAll,   BST_UNCHECKED);
						Button_SetCheck(hPSBJosn,  BST_UNCHECKED);

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
					BOOL SubResult[7];

					RtlZeroMemory(SubResult, sizeof(SubResult));
					HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
					HWND hPSBText = GetItemX(IDC_PSB_TEXT);
					HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
					HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
					HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
					HWND hPSBAll = GetItemX(IDC_PSB_ALL);
					HWND hPSBJosn = GetItemX(IDC_PSB_JSON);

					SubResult[0] = SendMessageW(hPSBRaw, BM_GETCHECK, 0, 0);
					SubResult[1] = SendMessageW(hPSBText, BM_GETCHECK, 0, 0);
					SubResult[2] = SendMessageW(hPSBImage, BM_GETCHECK, 0, 0);
					SubResult[3] = SendMessageW(hPSBAnm, BM_GETCHECK, 0, 0);
					SubResult[4] = SendMessageW(hPSBJosn, BM_GETCHECK, 0, 0);
					SubResult[5] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);
					SubResult[6] = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);


					if ((SubResult[0] + SubResult[1] + SubResult[2] + SubResult[3] + SubResult[4] + SubResult[5] + SubResult[6]) == 0)
					{
						GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
						Button_SetCheck(hPSBRaw, BST_UNCHECKED);
						Button_SetCheck(hPSBText, BST_UNCHECKED);
						Button_SetCheck(hPSBImage, BST_UNCHECKED);
						Button_SetCheck(hPSBAnm, BST_UNCHECKED);
						Button_SetCheck(hPSBAll, BST_UNCHECKED);
						Button_SetCheck(hPSBJosn, BST_UNCHECKED);

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
				HWND hPSBRaw = GetItemX(IDC_PSB_RAW);
				HWND hPSBImage = GetItemX(IDC_PSB_IMAGE);
				HWND hPSBDecom = GetItemX(IDC_PSB_SCRIPT);
				HWND hPSBText = GetItemX(IDC_PSB_TEXT);
				HWND hPSBAnm = GetItemX(IDC_PSB_ANIM);
				HWND hPSBJson = GetItemX(IDC_PSB_JSON);
				HWND hPSBAll = GetItemX(IDC_PSB_ALL);

				BOOL Result;
				Result = SendMessageW(hPSBAll, BM_GETCHECK, 0, 0);

				//单独处理
				if (Result == TRUE)
				{
					Button_SetCheck(hPSBDecom, BST_CHECKED);
					Button_SetCheck(hPSBText, BST_CHECKED);
					Button_SetCheck(hPSBImage, BST_CHECKED);
					Button_SetCheck(hPSBJson, BST_CHECKED);
					Button_SetCheck(hPSBAnm, BST_CHECKED);
					Button_SetCheck(hPSBAll, BST_CHECKED);
					Button_SetCheck(hPSBRaw, BST_CHECKED);
					GlobalData::GetGlobalData()->AddPsbFlag(PSB_ALL);
				}
				else
				{
					GlobalData::GetGlobalData()->DeletePsbFlag(PSB_ALL);
					Button_SetCheck(hPSBDecom, BST_UNCHECKED);
					Button_SetCheck(hPSBText, BST_UNCHECKED);
					Button_SetCheck(hPSBImage, BST_UNCHECKED);
					Button_SetCheck(hPSBAnm, BST_UNCHECKED);
					Button_SetCheck(hPSBAll, BST_UNCHECKED);
					Button_SetCheck(hPSBJson, BST_UNCHECKED);

					Button_SetCheck(hPSBRaw, BST_CHECKED);
					GlobalData::GetGlobalData()->AddPsbFlag(PSB_RAW);
				}
			}
			break;
			}
		}
		break;

		//Icon
		case IDC_PACK_SAME_ICON:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hClick = GetItemX(IDC_PACK_SAME_ICON);

				BOOL Result;
				Result = SendMessageW(hClick, BM_GETCHECK, 0, 0);

				GlobalData::GetGlobalData()->InheritIcon = Result;
			}
			break;
			}
		}
		break;

		//Protection
		case IDC_PACK_ADD_PROTECT:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hClick = GetItemX(IDC_PACK_ADD_PROTECT);

				BOOL Result;
				Result = SendMessageW(hClick, BM_GETCHECK, 0, 0);

				GlobalData::GetGlobalData()->IsProtection = Result;
			}
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

		//tjs2 raw
		case IDC_TJS2_RAW:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hSubWnd = GetItemX(IDC_TJS2_RAW);
				BOOL Result;
				Result = SendMessageW(hSubWnd, BM_GETCHECK, 0, 0);
				if (Result == TRUE)
				{
					GlobalData::GetGlobalData()->SetTjsFlag(TJS2_RAW);
				}

			}
			break;
			}
		}
		break;

		//tjs2 decom
		case IDC_TJS2_Decomp:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hSubWnd = GetItemX(IDC_TJS2_Decomp);
				BOOL Result;
				Result = SendMessageW(hSubWnd, BM_GETCHECK, 0, 0);
				if (Result == TRUE)
				{
					GlobalData::GetGlobalData()->SetTjsFlag(TJS2_DECOM);
				}

			}
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

		case IDC_TLG_JPG:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				HWND hTLGSys = GetItemX(IDC_TLG_JPG);
				BOOL Result;
				Result = SendMessageW(hTLGSys, BM_GETCHECK, 0, 0);
				if (Result == TRUE)
				{
					GlobalData::GetGlobalData()->SetTlgFlag(TLG_JPG);
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
				BROWSEINFOW FolderInfo;

				RtlZeroMemory(&FolderInfo, sizeof(BROWSEINFOW));
				FolderInfo.hwndOwner = hWnd;
				FolderInfo.lpszTitle = L"Select a folder to pack";
				FolderInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_UAHINT | BIF_NONEWFOLDERBUTTON;
				LPITEMIDLIST Dlist;
				Dlist = SHBrowseForFolderW(&FolderInfo);
				if (Dlist != nullptr)
				{
					SHGetPathFromIDListW(Dlist, lpString);
					GlobalData::GetGlobalData()->SetFolder(lpString);
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

				WCHAR lpString[MAX_PATH];
				WCHAR CurDir[MAX_PATH];

				RtlZeroMemory(lpString, sizeof(lpString));
				RtlZeroMemory(CurDir, sizeof(CurDir));

				OPENFILENAMEW OpenInfo;

				RtlZeroMemory(&OpenInfo, sizeof(OPENFILENAMEW));
				OpenInfo.lStructSize = sizeof(OPENFILENAMEW);
				OpenInfo.hwndOwner = hWnd;
				OpenInfo.lpstrFilter = L"XP3 File\0*.xp3";
				OpenInfo.lpstrFile = lpString;
				OpenInfo.nMaxFile = sizeof(lpString) / 2;
				OpenInfo.lpstrTitle = L"Select an original xp3 file";

				GetCurrentDirectoryW(MAX_PATH, CurDir);
				OpenInfo.lpstrInitialDir = CurDir;
				OpenInfo.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
				BOOL Result = GetOpenFileNameW(&OpenInfo);
				if (Result)
				{
					GlobalData::GetGlobalData()->SetGuessPack(lpString);
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

				WCHAR lpString[MAX_PATH];
				WCHAR CurDir[MAX_PATH];

				RtlZeroMemory(lpString, sizeof(lpString));
				RtlZeroMemory(CurDir, sizeof(CurDir));

				OPENFILENAMEW OpenInfo;

				RtlZeroMemory(&OpenInfo, sizeof(OPENFILENAMEW));
				OpenInfo.lStructSize = sizeof(OPENFILENAMEW);
				OpenInfo.hwndOwner = hWnd;
				OpenInfo.lpstrFilter = L"XP3 File\0*.xp3";
				OpenInfo.lpstrFile = lpString;
				OpenInfo.nMaxFile = sizeof(lpString) / 2;
				OpenInfo.lpstrTitle = L"Output xp3 file";

				GetCurrentDirectoryW(MAX_PATH, CurDir);
				OpenInfo.lpstrInitialDir = CurDir;
				OpenInfo.Flags = OFN_EXPLORER;
				BOOL Result = GetOpenFileNameW(&OpenInfo);
				if (Result)
				{
					GlobalData::GetGlobalData()->SetOutputPack(lpString);
					HWND hSubWnd = GetDlgItem(hWnd, IDC_PACK_EDIT_OUTPACK);
					SetWindowTextW(hSubWnd, lpString);
				}
			}
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
				GlobalData::GetGlobalData()->WorkerThread = StartPacker();
			}
			break;
			default:
				break;
			}
		}
		break;

		//Make Universal Patch
		case IDC_PACK_BUTTON_UMAKE:
		{
			switch (wmEvent)
			{
			case BN_CLICKED:
			{
				MakePatch();
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
						SetWindowTextW(hDbg, L"Open Debugger");
						GlobalData::GetGlobalData()->HasConsole = FALSE;
					}
				}
				else
				{
					if (AllocConsole())
					{
						SetWindowTextW(hDbg, L"Close Debugger");
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
			RtlZeroMemory(GlobalData::GetGlobalData()->DragFileName, MAX_PATH * 2);
			DragQueryFileW(hDrop, 0, GlobalData::GetGlobalData()->DragFileName, MAX_PATH);
			

			ULONG FileAttr = GetFileAttributesW(GlobalData::GetGlobalData()->DragFileName);
			if (FileAttr != INVALID_FILE_ATTRIBUTES)
			{
				if (FileAttr & FILE_ATTRIBUTE_DIRECTORY)
				{
					HWND hFolderEdit = GetItemX(IDC_PACK_EDIT_FOLDER);
					SetWindowTextW(hFolderEdit, GlobalData::GetGlobalData()->DragFileName);
					GlobalData::GetGlobalData()->SetFolder(GlobalData::GetGlobalData()->DragFileName);

					HWND hOutEdit = GetItemX(IDC_PACK_EDIT_OUTPACK);
					wstring OutEditName = GlobalData::GetGlobalData()->DragFileName;
					OutEditName += L".xp3";
					SetWindowTextW(hOutEdit, OutEditName.c_str());
					GlobalData::GetGlobalData()->SetOutputPack(OutEditName.c_str());
				}
				else
				{
					auto GetExtensionName = [](PCWSTR Name)->std::wstring
					{
						std::wstring FileName(Name);
						auto Index = FileName.find_last_of(L'.');
						if (Index != std::wstring::npos)
							return FileName.substr(Index + 1, std::wstring::npos);
						
						return std::wstring(L"");
					};

					
					auto ExtName = GetExtensionName(GlobalData::GetGlobalData()->DragFileName);
					if (lstrcmpiW(ExtName.c_str(), L"xp3i") == 0)
						GlobalData::GetGlobalData()->WorkerThread = StartMiniDumper(GlobalData::GetGlobalData()->DragFileName);
					else
						GlobalData::GetGlobalData()->WorkerThread = StartDumper(GlobalData::GetGlobalData()->DragFileName);
					
				}
			}
		}
		else if(nFileNum > 1)
		{
			MessageBoxW(hWnd, L"Please Drop one file on this window", L"XP3Extract", MB_OK);
		}
	}
	DragFinish((HDROP)wParam);
	break;

	case WM_DESTROY:
		ExitProcess(0);
	break;
	}
	return 0;
}
