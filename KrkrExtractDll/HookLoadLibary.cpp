#include "HookLoadLibary.h"
#include "PluginHook.h"
#include "Console.h"
#include "GlobalInit.h"
#include "XP3FilterModify.h"
#include "Native.h"


BOOL FindIAT(HMODULE hmod, LPCSTR szDllName, LPCSTR szFunc)
{
	LPCSTR szLibName;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
	PIMAGE_THUNK_DATA pThunk;
	DWORD dwRVA;
	PBYTE pAddr;

	HMODULE hMod = GetModuleHandleA(szDllName);
	DWORD pfnOrg = (DWORD)GetProcAddress(hMod, szFunc);

	pAddr = (PBYTE)hmod;
	pAddr += *((DWORD*)&pAddr[0x3C]);
	dwRVA = *((DWORD*)&pAddr[0x80]);

	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)hmod + dwRVA);

	for (; pImportDesc->Name; pImportDesc++)
	{
		szLibName = (LPCSTR)((DWORD)hmod + pImportDesc->Name);
		if (!_stricmp(szLibName, szDllName))
		{
			pThunk = (PIMAGE_THUNK_DATA)((DWORD)hmod + pImportDesc->FirstThunk);

			for (; pThunk->u1.Function; pThunk++)
			{
				if (pThunk->u1.Function == (DWORD)pfnOrg)
				{
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}



HMODULE WINAPI HookLoadLibraryA(LPCSTR lpFileName)
{
	HMODULE result = LoadLibraryA(lpFileName);
	if (FindIAT(result, "kernel32.dll", "FlushInstructionCache"))
	{
		return result;
	}
	PluginHook::GetInstance()->InitHook(lpFileName, result);
	return result;
}


HMODULE WINAPI HookLoadLibraryW(LPCWSTR lpFileName)
{
	HMODULE result = LoadLibraryW(lpFileName);
	if (FindIAT(result, "kernel32.dll", "FlushInstructionCache"))
	{
		return result;
	}
	char* lpString = (char*)RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * 4);
	WideCharToMultiByte(CP_ACP, NULL, lpFileName, -1, lpString, MAX_PATH * 4, NULL, NULL);
	PluginHook::GetInstance()->InitHook(lpString, result);
	RtlFreeHeap(GetProcessHeap(), 0, lpString);
	return result;
}


BOOL StartHook(LPCSTR szDllName, PROC pfnOrg, PROC pfnNew)
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
		if (!_stricmp(szLibName, szDllName))
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


XP3Filter pfTVPSetXP3ArchiveExtractionFilter = nullptr;

void WINAPI HookTVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter filter)
{
	//_InterlockedExchangePointer((void* volatile *)&(GlobalData::pfGlobalXP3Filter), (void*)filter);

	SetRealFilter(filter);
	tTVPXP3ArchiveExtractionFilter FakeFilter = InitFakeFilter();
	_InterlockedExchangePointer((void* volatile *)&(GlobalData::GetGlobalData()->pfGlobalXP3Filter), (void*)FakeFilter);

	WCHAR Info[200] = { 0 };
	wsprintfW(Info, L"Filter ProcAddr : 0x%08x\n", filter);
	OutputString(Info);
	pfTVPSetXP3ArchiveExtractionFilter(filter);
}

typedef iTVPFunctionExporter*   (WINAPI *TVPGetFunctionExporterFunc)();
TVPGetFunctionExporterFunc pfTVPGetFunctionExporter = nullptr;
BOOL FakePngWorkerInited = FALSE;

//多次调用的
iTVPFunctionExporter* WINAPI HookTVPGetFunctionExporter()
{
	iTVPFunctionExporter* result = pfTVPGetFunctionExporter();
	GlobalData::GetGlobalData()->TVPFunctionExporter = result;
	InitExporter(result);

	if (!FakePngWorkerInited)
	{
		InitLayer();

		static char funcname[] = "void ::TVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter)";
		pfTVPSetXP3ArchiveExtractionFilter = (XP3Filter)TVPGetImportFuncPtr(funcname);

		if (pfTVPSetXP3ArchiveExtractionFilter == nullptr)
		{
			//Error 
		}
		else
		{
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourAttach((PVOID*)&pfTVPSetXP3ArchiveExtractionFilter, HookTVPSetXP3ArchiveExtractionFilter);
			DetourTransactionCommit();
		}

		PluginHook::GetInstance()->InitHookNull();
	}
	return result;
}

PVOID pGetProcAddress = NULL;
typedef FARPROC(WINAPI *PGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);
FARPROC WINAPI HookGetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	if (!strcmp(lpProcName, "GetSystemWow64DirectoryA"))
	{
		return NULL;
	}
	return ((PGetProcAddress)pGetProcAddress)(hModule, lpProcName);
}


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
HookCreateProcessInternalW(
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
)
{
	BOOL             Result, IsSuspended;
	UNICODE_STRING   FullDllPath;

	RtlInitUnicodeString(&FullDllPath, GlobalData::GetGlobalData()->SelfPath);
	
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

	return TRUE;
}

HRESULT WINAPI InitHook()
{
	FARPROC pLoadLibraryA = NULL;
	FARPROC pLoadLibraryW = NULL;

	HMODULE hModule = GetModuleHandle(NULL);
	*(FARPROC *)&pfTVPGetFunctionExporter = GetProcAddress(hModule, "TVPGetFunctionExporter");

	//Krkr2 & normal Krkrz Module
	if (pfTVPGetFunctionExporter != nullptr)
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach((PVOID*)&pfTVPGetFunctionExporter, HookTVPGetFunctionExporter);
		DetourTransactionCommit();
	}
	else
	{
		//一些有smc 会检验code

		pLoadLibraryA = GetProcAddress(GetModuleHandleW(L"KERNEL32.dll"), "LoadLibraryA");
		pLoadLibraryW = GetProcAddress(GetModuleHandleW(L"KERNEL32.dll"), "LoadLibraryW");

		StartHook("KERNEL32.dll", pLoadLibraryA, (PROC)HookLoadLibraryA);
		StartHook("KERNEL32.dll", pLoadLibraryW, (PROC)HookLoadLibraryW);
	}

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	//*(PULONG_PTR)StubCreateProcessInternalW = (ULONG_PTR)DetourFindFunction("Kernel32.dll", "CreateProcessInternalW");

	StubCreateProcessInternalW =
	(FuncCreateProcessInternalW)GetProcAddress(GetModuleHandleW(L"KERNEL32.dll"), "CreateProcessInternalW");
	DetourAttach((PVOID*)&StubCreateProcessInternalW, HookCreateProcessInternalW);
	DetourTransactionCommit();

	DWORD Ver = GetVersion();
	//if ((HIWORD(Ver) == 6 && LOWORD(Ver) >= 2) || (HIWORD(Ver) > 6))
	if (0)
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		pGetProcAddress = DetourFindFunction("Kernel32.dll", "GetProcAddress");
		DetourAttach(&pGetProcAddress, HookGetProcAddress);
		DetourTransactionCommit();
	}
	return S_OK;
}
