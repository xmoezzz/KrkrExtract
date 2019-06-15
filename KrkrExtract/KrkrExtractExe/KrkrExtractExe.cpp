#include <my.h>
#include "resource.h"

typedef
BOOL
(WINAPI
	*FuncCreateProcessInternalW)(
		HANDLE                  hToken,
		LPCWSTR                 lpApplicationName,
		LPWSTR                  lpCommandLine,
		LPSECURITY_ATTRIBUTES   lpProcessAttributes,
		LPSECURITY_ATTRIBUTES   lpThreadAttributes,
		BOOL                    bInheritHandles,
		ULONG                   dwCreationFlags,
		LPVOID                  lpEnvironment,
		LPCWSTR                 lpCurrentDirectory,
		LPSTARTUPINFOW          lpStartupInfo,
		LPPROCESS_INFORMATION   lpProcessInformation,
		PHANDLE                 phNewToken
		);

BOOL
(WINAPI
	*StubCreateProcessInternalW)(
		HANDLE                  hToken,
		LPCWSTR                 lpApplicationName,
		LPWSTR                  lpCommandLine,
		LPSECURITY_ATTRIBUTES   lpProcessAttributes,
		LPSECURITY_ATTRIBUTES   lpThreadAttributes,
		BOOL                    bInheritHandles,
		ULONG                   dwCreationFlags,
		LPVOID                  lpEnvironment,
		LPCWSTR                 lpCurrentDirectory,
		LPSTARTUPINFOW          lpStartupInfo,
		LPPROCESS_INFORMATION   lpProcessInformation,
		PHANDLE                 phNewToken
		);



BOOL
WINAPI
VMeCreateProcess(
	HANDLE                  hToken,
	LPCWSTR                 lpApplicationName,
	LPWSTR                  lpCommandLine,
	LPCWSTR                 lpDllPath,
	LPSECURITY_ATTRIBUTES   lpProcessAttributes,
	LPSECURITY_ATTRIBUTES   lpThreadAttributes,
	BOOL                    bInheritHandles,
	ULONG                   dwCreationFlags,
	LPVOID                  lpEnvironment,
	LPCWSTR                 lpCurrentDirectory,
	LPSTARTUPINFOW          lpStartupInfo,
	LPPROCESS_INFORMATION   lpProcessInformation,
	PHANDLE                 phNewToken
)
{
	BOOL             Result, IsSuspended;
	UNICODE_STRING   FullDllPath;

	RtlInitUnicodeString(&FullDllPath, (PWSTR)lpDllPath);

	IsSuspended = !!(dwCreationFlags & CREATE_SUSPENDED);
	dwCreationFlags |= CREATE_SUSPENDED;
	Result = StubCreateProcessInternalW(
		hToken,
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation,
		phNewToken);

	if (!Result)
		return Result;

	InjectDllToRemoteProcess(
		lpProcessInformation->hProcess,
		lpProcessInformation->hThread,
		&FullDllPath,
		IsSuspended
	);

	NtResumeThread(lpProcessInformation->hThread, NULL);

	return TRUE;
}

BOOL NTAPI CreateProcessInternalWithDll(LPCWSTR ProcessName)
{
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	return VMeCreateProcess(NULL, ProcessName, NULL, L"KrkrExtract.dll", NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi, NULL);
}


BOOL CheckDll()
{
	DWORD  Attribute;

	Attribute = GetFileAttributesW(L"KrkrExtract.dll");
	return Attribute != 0xFFFFFFFF;
}


LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		HICON hIcon;

		hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL),
			MAKEINTRESOURCE(IDI_ICON1),
			IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			0);

		if (hIcon)
		{
			SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		}
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

	default:
		break;
	}
	return 0;
}


int CreateMainWindow()
{
	HWND MainWindow;
	MSG  msg;

	MainWindow = CreateDialogParamW(
		GetModuleHandleW(NULL),
		MAKEINTRESOURCEW(IDD_DIALOG_MAIN),
		NULL,
		(DLGPROC)DlgProc,
		WM_INITDIALOG);

	if (!MainWindow)
		return -1;

	DragAcceptFiles(MainWindow, TRUE);
	ShowWindow(MainWindow, SW_SHOW);
	UpdateWindow(MainWindow);

	while (GetMessageW(&msg, NULL, NULL, NULL))
	{
		if (!IsDialogMessageW(MainWindow, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else
		{
			SendMessageW(MainWindow, msg.message, msg.wParam, msg.lParam);
		}
	}
	return 0;
}


int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR    lpCmdLine,
	int       nShowCmd
)
{
	PWSTR*              Argv;
	INT                 Argc;
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;
	BOOL                CreateResult;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);
	
	Argv = CommandLineToArgvW(Nt_CurrentPeb()->ProcessParameters->CommandLine.Buffer, &Argc);
	if (Argv == NULL || Argc < 2)
	{
		LocalFree(Argv);
		return CreateMainWindow();
	}

	StubCreateProcessInternalW = (FuncCreateProcessInternalW)EATLookupRoutineByHashPNoFix(GetKernel32Handle(), KERNEL32_CreateProcessInternalW);

	CreateResult = CreateProcessInternalWithDll(Argv[1]);
	if (!CreateResult)
	{
		MessageBoxW(NULL, L"Couldn't Launch KrkrExtract", L"KrkrExtract", MB_OK | MB_ICONERROR);
	}

	LocalFree(Argv);
	return 0;
}




