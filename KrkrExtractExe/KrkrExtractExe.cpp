#pragma comment(linker, "/SECTION:.text,ERW /MERGE:.rdata=.text /MERGE:.data=.text")
#pragma comment(linker, "/SECTION:.Xmoe,ERW /MERGE:.text=.Xmoe")

#include "KrkrExtractExe.h"
#include <Psapi.h>
#include "Shellapi.h"
#include <string>
#include <Shlobj.h>
#include "WinFile.h"


#pragma comment(lib, "Psapi.lib")

using std::wstring;

#define wClassName  L"KrkrExtractTool"
#define wClassTitle L"[X'moe]KrkrExtractLauncher"

BOOL Inited = FALSE;
BOOL isRunning = FALSE;

HWND   MainWindow = nullptr;
HANDLE hThread = (HANDLE)-1;
DWORD  ThreadId = 0;

DWORD WINAPI DoExtraction(WCHAR* ExtractFileName);


typedef BOOL(WINAPI* Proc_CreateProcessW)(LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation);

typedef HMODULE(WINAPI* Func_LoadLibraryW)(LPCWSTR lpLibFileName);


BYTE* mov_eax_xx(BYTE* lpCurAddres, DWORD eax)
{
	*lpCurAddres = 0xB8;
	*(DWORD*)(lpCurAddres + 1) = eax;
	return lpCurAddres + 5;
}

BYTE* mov_ebx_xx(BYTE* lpCurAddres, DWORD ebx)
{
	*lpCurAddres = 0xBB;
	*(DWORD*)(lpCurAddres + 1) = ebx;
	return lpCurAddres + 5;
}

BYTE* mov_ecx_xx(BYTE* lpCurAddres, DWORD ecx)
{
	*lpCurAddres = 0xB9;
	*(DWORD*)(lpCurAddres + 1) = ecx;
	return lpCurAddres + 5;
}

BYTE* mov_edx_xx(BYTE* lpCurAddres, DWORD edx)
{
	*lpCurAddres = 0xBA;
	*(DWORD*)(lpCurAddres + 1) = edx;
	return lpCurAddres + 5;
}

BYTE* mov_esi_xx(BYTE* lpCurAddres, DWORD esi)
{
	*lpCurAddres = 0xBE;
	*(DWORD*)(lpCurAddres + 1) = esi;
	return lpCurAddres + 5;
}

BYTE* mov_edi_xx(BYTE* lpCurAddres, DWORD edi)
{
	*lpCurAddres = 0xBF;
	*(DWORD*)(lpCurAddres + 1) = edi;
	return lpCurAddres + 5;
}

BYTE* mov_ebp_xx(BYTE* lpCurAddres, DWORD ebp)
{
	*lpCurAddres = 0xBD;
	*(DWORD*)(lpCurAddres + 1) = ebp;
	return lpCurAddres + 5;
}

BYTE* mov_esp_xx(BYTE* lpCurAddres, DWORD esp)
{
	*lpCurAddres = 0xBC;
	*(DWORD*)(lpCurAddres + 1) = esp;
	return lpCurAddres + 5;
}

BYTE* mov_eip_xx(BYTE* lpCurAddres, DWORD eip, DWORD newEip)
{
	if (!newEip)
	{
		newEip = (DWORD)lpCurAddres;
	}

	*lpCurAddres = 0xE9;
	*(DWORD*)(lpCurAddres + 1) = eip - (newEip + 5);
	return lpCurAddres + 5;
}

BYTE* push_xx(BYTE* lpCurAddres, DWORD dwAdress)
{

	*lpCurAddres = 0x68;
	*(DWORD*)(lpCurAddres + 1) = dwAdress;

	return lpCurAddres + 5;
}

BYTE* Call_xx(BYTE* lpCurAddres, DWORD eip, DWORD newEip)
{
	if (!newEip)
	{
		newEip = (DWORD)lpCurAddres;
	}

	*lpCurAddres = 0xE8;
	*(DWORD*)(lpCurAddres + 1) = eip - (newEip + 5);
	return lpCurAddres + 5;
}

BOOL SuspendTidAndInjectCode(HANDLE hProcess, HANDLE hThread, DWORD dwFuncAdress, const BYTE * lpShellCode, size_t uCodeSize)
{
	SIZE_T NumberOfBytesWritten = 0;
	BYTE ShellCodeBuf[0x480];
	CONTEXT Context;
	DWORD flOldProtect = 0;
	LPBYTE lpCurESPAddress = NULL;
	LPBYTE lpCurBufAdress = NULL;
	BOOL bResult = FALSE;

	SuspendThread(hThread);

	memset(&Context, 0, sizeof(Context));
	Context.ContextFlags = CONTEXT_FULL;

	if (GetThreadContext(hThread, &Context))
	{
		lpCurESPAddress = (LPBYTE)((Context.Esp - 0x480) & 0xFFFFFFE0);

		lpCurBufAdress = &ShellCodeBuf[0];

		if (lpShellCode)
		{
			memcpy(ShellCodeBuf + 128, lpShellCode, uCodeSize);
			lpCurBufAdress = push_xx(lpCurBufAdress, (DWORD)lpCurESPAddress + 128); // push
			lpCurBufAdress = Call_xx(lpCurBufAdress, dwFuncAdress, (DWORD)lpCurESPAddress + (DWORD)lpCurBufAdress - (DWORD)&ShellCodeBuf); //Call
		}

		lpCurBufAdress = mov_eax_xx(lpCurBufAdress, Context.Eax);
		lpCurBufAdress = mov_ebx_xx(lpCurBufAdress, Context.Ebx);
		lpCurBufAdress = mov_ecx_xx(lpCurBufAdress, Context.Ecx);
		lpCurBufAdress = mov_edx_xx(lpCurBufAdress, Context.Edx);
		lpCurBufAdress = mov_esi_xx(lpCurBufAdress, Context.Esi);
		lpCurBufAdress = mov_edi_xx(lpCurBufAdress, Context.Edi);
		lpCurBufAdress = mov_ebp_xx(lpCurBufAdress, Context.Ebp);
		lpCurBufAdress = mov_esp_xx(lpCurBufAdress, Context.Esp);
		lpCurBufAdress = mov_eip_xx(lpCurBufAdress, Context.Eip, (DWORD)lpCurESPAddress + (DWORD)lpCurBufAdress - (DWORD)&ShellCodeBuf);
		Context.Esp = (DWORD)(lpCurESPAddress - 4);
		Context.Eip = (DWORD)lpCurESPAddress;

		if (VirtualProtectEx(hProcess, lpCurESPAddress, 0x480, PAGE_EXECUTE_READWRITE, &flOldProtect)
			&& WriteProcessMemory(hProcess, lpCurESPAddress, &ShellCodeBuf, 0x480, &NumberOfBytesWritten)
			&& FlushInstructionCache(hProcess, lpCurESPAddress, 0x480)
			&& SetThreadContext(hThread, &Context))
		{
			bResult = TRUE;
		}

	}
	ResumeThread(hThread);

	return TRUE;
}


DWORD GetFuncAdress()
{
	return (DWORD)GetProcAddress(GetModuleHandleA("Kernel32"), "LoadLibraryW");
}

BOOL WINAPI CreateProcessWithDllW(LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	LPWSTR lpDllFullPath,
	Proc_CreateProcessW FuncAdress
	)
{
	BOOL bResult = FALSE;
	size_t uCodeSize = 0;
	DWORD dwCreaFlags;
	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if (FuncAdress == NULL)
	{
		FuncAdress = CreateProcessW;
	}

	dwCreaFlags = dwCreationFlags | CREATE_SUSPENDED;
	if (CreateProcessW(lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreaFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		&pi
		))
	{
		if (lpDllFullPath)
			uCodeSize = 2 * wcslen(lpDllFullPath) + 2;
		else
			uCodeSize = 0;

		DWORD dwLoadDllProc = GetFuncAdress();

		if (SuspendTidAndInjectCode(pi.hProcess, pi.hThread, dwLoadDllProc, (BYTE*)lpDllFullPath, uCodeSize))
		{
			if (lpProcessInformation)
				memcpy(lpProcessInformation, &pi, sizeof(PROCESS_INFORMATION));

			if (!(dwCreationFlags & CREATE_SUSPENDED))
				ResumeThread(pi.hThread);

			bResult = TRUE;
		}
	}
	return bResult;
}


BOOL WINAPI CheckDll()
{
	WinFile File;
	if (File.Open(L"KrkrExtract.dll", WinFile::FileRead) != S_OK)
	{
		return FALSE;
	}
	File.Release();
	return TRUE;
}

static WCHAR strFileName[MAX_PATH] = { 0 };

void WINAPI RunProc()
{
	WCHAR DllPath[MAX_PATH] = { 0 };
	WCHAR wszDll[] = L"KrkrExtract.dll";

	GetCurrentDirectoryW(MAX_PATH, DllPath);
	lstrcatW(DllPath, L"\\");
	lstrcatW(DllPath, wszDll);

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	if (!CheckDll())
	{
		MessageBoxW(NULL, L"Please make sure KrkrExtract.dll and KrkrExtract.exe are all in\n"
			L"game root path!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		RtlZeroMemory(strFileName, MAX_PATH * 2);
		return;
	}

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	BOOL result = CreateProcessWithDllW(NULL, strFileName, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi, DllPath, NULL);

	if (result == FALSE)
	{
		MessageBoxW(NULL, L"Failed to launch!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		RtlZeroMemory(strFileName, MAX_PATH * 2);
		return;
	}

	ResumeThread(pi.hThread);
	PostQuitMessage(0);
}

LRESULT CALLBACK WindowProc(HWND   hwnd,
	UINT   msg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc;
		BeginPaint(hwnd, &ps);
		hdc = GetDC(hwnd);
		static WCHAR test[MAX_PATH] = L"Drop a Krkr Exe Here>_<";
		::TextOutW(hdc, 240, 350, test, lstrlenW(test));
		EndPaint(hwnd, &ps);

	} break;

	case WM_CLOSE:
	{
		PostQuitMessage(0);
	}break;

	case WM_DROPFILES:
	{
		HDROP hDrop = (HDROP)wParam;

		UINT nFileNum = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
		if (nFileNum == 1)
		{
			InvalidateRect(hwnd, NULL, TRUE);
			RtlZeroMemory(strFileName, MAX_PATH * 2);
			DragQueryFileW(hDrop, 0, strFileName, MAX_PATH);
			DragFinish(hDrop);

			RunProc();
		}
		else
		{
			MessageBoxW(hwnd, L"Please Drop one exe on this window", L"XP3Extract", MB_OK);
			DragFinish(hDrop);
		}

	}break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}


typedef BOOL(WINAPI* pfSetProcessDPIAware)();
HRESULT WINAPI DisableHighDPI()
{
	HMODULE hDll = LoadLibraryW(L"User32.dll");
	if (hDll)
	{
		pfSetProcessDPIAware StubSetProcessDPIAware = GetProcAddress(hDll, "SetProcessDPIAware");
		if (StubSetProcessDPIAware)
		{
			StubSetProcessDPIAware();
			FreeLibrary(hDll);
			return S_OK;
		}
	}
	return S_FALSE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR  lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	DisableHighDPI();

	if (__argc == 1)
	{
		isRunning = FALSE;
		ThreadId = 0;
		hThread = INVALID_HANDLE_VALUE;
		MainWindow = nullptr;

		WNDCLASSEXW winclass;

		winclass.cbSize = sizeof(WNDCLASSEX);
		winclass.style = CS_HREDRAW | CS_VREDRAW;
		winclass.lpfnWndProc = WindowProc;
		winclass.cbClsExtra = 0;
		winclass.cbWndExtra = 0;
		winclass.hInstance = nullptr;
		winclass.hIcon = LoadIconW(NULL, IDI_APPLICATION);
		winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		winclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		winclass.lpszMenuName = NULL;
		winclass.lpszClassName = wClassName;
		winclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		if (!RegisterClassExW(&winclass))
		{
			MessageBoxW(NULL, L"Cannot create a the main window!", L"Error", 0);

			return S_FALSE;
		}


		MainWindow = CreateWindowExW(NULL,
			wClassName,
			wClassTitle,
			WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX,
			0,
			0,
			700,
			480,
			NULL,
			NULL,
			NULL,
			NULL);

		if (!MainWindow)
		{
			MessageBoxW(NULL, L"Cannot create a the main window!", L"Error!", 0);
			return S_FALSE;
		}
		DragAcceptFiles(MainWindow, TRUE);
		ShowWindow(MainWindow, SW_SHOW);
		UpdateWindow(MainWindow);

		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return (INT)msg.wParam;
	}

	if (__argc != 2)
	{
		return -1;
	}



	WCHAR* wszPath = __wargv[1];
	WCHAR DllPath[MAX_PATH] = { 0 };
	WCHAR wszDll[] = L"KrkrExtract.dll";

	GetCurrentDirectoryW(MAX_PATH, DllPath);
	lstrcatW(DllPath, L"\\");
	lstrcatW(DllPath, wszDll);

	if (!CheckDll())
	{
		MessageBoxW(NULL, L"Please make sure KrkrExtract.dll and KrkrExtract.exe are all in\n"
			L"game root path!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		return -1;
	}

	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	BOOL result = CreateProcessWithDllW(NULL, wszPath, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi, DllPath, NULL);

	if (result == FALSE)
	{
		MessageBoxW(NULL, L"Failed to launch!", L"KrkrExtract", MB_OK | MB_ICONERROR);
		return -1;
	}

	ResumeThread(pi.hThread);
	return 0;
}




