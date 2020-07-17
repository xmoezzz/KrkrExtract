#include "MiniLocale.h"

BOOL StartHookIAT(LPCSTR szDllName, PROC pfnOrg, PROC pfnNew)
{
	HMODULE hmod;
	LPCSTR szLibName;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
	PIMAGE_THUNK_DATA pThunk;
	DWORD dwOldProtect, dwRVA;
	PBYTE pAddr;
	hmod = GetModuleHandleW(NULL);
	pAddr = (PBYTE)hmod;
	pAddr += *((DWORD*)&pAddr[0x3C]);
	dwRVA = *((DWORD*)&pAddr[0x80]);

	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)hmod + dwRVA);
	for (; pImportDesc->Name; pImportDesc++)
	{
		szLibName = (LPCSTR)((DWORD)hmod + pImportDesc->Name);
		if (!stricmp(szLibName, szDllName))
		{
			pThunk = (PIMAGE_THUNK_DATA)((DWORD)hmod + pImportDesc->FirstThunk);
			for (; pThunk->u1.Function; pThunk++)
			{
				if (pThunk->u1.Function == (DWORD)pfnOrg)
				{
					VirtualProtect((LPVOID)&pThunk->u1.Function, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
					pThunk->u1.Function = (DWORD)pfnNew;
					VirtualProtect((LPVOID)&pThunk->u1.Function, 4, dwOldProtect, &dwOldProtect);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}


int WINAPI HookMultiByteToWideChar(
	UINT   CodePage,
	DWORD  dwFlags,
	LPCSTR lpMultiByteStr,
	int    cbMultiByte,
	LPWSTR lpWideCharStr,
	int    cchWideChar
	)
{
	if (CodePage == CP_ACP)
	{
		return
			MultiByteToWideChar(
			932,
			dwFlags,
			lpMultiByteStr,
			cbMultiByte,
			lpWideCharStr,
			cchWideChar
			);
	}
	else
	{
		return
			MultiByteToWideChar(
			CodePage,
			dwFlags,
			lpMultiByteStr,
			cbMultiByte,
			lpWideCharStr,
			cchWideChar
			);
	}
}

FARPROC pfMultiByteToWideChar = nullptr;

//无视Krkr2的转区
//krkr2:
//获取CreateTextStreamForRead的地址
//分析出第一个call
//Hook整个class 完事
//或者直接暴力加载LE
BOOLEAN WINAPI InitMiniLocale()
{
	pfMultiByteToWideChar = GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "MultiByteToWideChar");

	if (!StartHookIAT("Kernel32.dll", pfMultiByteToWideChar, (PROC)HookMultiByteToWideChar))
	{
		return FALSE;
	}
	return TRUE;
}
