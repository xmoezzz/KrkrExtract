#include "MyHook.h"
#include "MinHook.h"
#include <Psapi.h>

#ifdef  _M_X64
#pragma comment(lib, "libMinHook.x64.lib")
#else
#pragma comment(lib, "libMinHook.x86.lib")
#endif

BOOL NTAPI IATPatch32(PVOID hMod, LPCSTR szDllName, LPVOID pfnOrg, LPVOID pfnNew)
{
	LPCSTR                     szLibName;
	PIMAGE_IMPORT_DESCRIPTOR   pImportDesc;
	PIMAGE_THUNK_DATA          pThunk;
	ULONG                      dwOldProtect, dwRVA;
	PBYTE                      pAddr;

	pAddr = (PBYTE)hMod;
	pAddr += *((DWORD*)&pAddr[0x3C]);
	dwRVA = *((DWORD*)&pAddr[0x80]);

	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)hMod + dwRVA);

	for (; pImportDesc->Name; pImportDesc++)
	{
		szLibName = (LPCSTR)((DWORD)hMod + pImportDesc->Name);
		if (!lstrcmpiA(szLibName, szDllName))
		{
			pThunk = (PIMAGE_THUNK_DATA)((DWORD)hMod + pImportDesc->FirstThunk);
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

BOOL NTAPI IATPatch64(HMODULE hMod, LPCSTR DllName, LPVOID orgProc, LPVOID newProc)
{
	IMAGE_DOS_HEADER*                 pDosHeader;
	IMAGE_OPTIONAL_HEADER64*          pOptHeader;
	IMAGE_IMPORT_DESCRIPTOR*          pImportDesc;
	IMAGE_THUNK_DATA*                 pThunk;
	PULONG_PTR                        lpAddr;
	DWORD                             dwOldProtect;

	pDosHeader = (IMAGE_DOS_HEADER*)hMod;
	pOptHeader = (IMAGE_OPTIONAL_HEADER64 *)((BYTE*)hMod + pDosHeader->e_lfanew + 24);
	pImportDesc = (IMAGE_IMPORT_DESCRIPTOR*)((BYTE*)hMod + pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (pImportDesc->FirstThunk)
	{
		LPCSTR pszDllName = (LPCSTR)((PBYTE)hMod + pImportDesc->Name);

		if (lstrcmpiA(pszDllName, DllName) == 0)
			break;

		pImportDesc++;
	}

	if (pImportDesc->FirstThunk)
	{
		pThunk = (IMAGE_THUNK_DATA*)((BYTE*)hMod + pImportDesc->FirstThunk);

		while (pThunk->u1.Function)
		{
			lpAddr = (PULONG_PTR)&(pThunk->u1.Function);
			if (*lpAddr == (ULONG_PTR)orgProc)
			{
				VirtualProtect(lpAddr, sizeof(LPVOID), PAGE_EXECUTE_READWRITE, &dwOldProtect);
				*lpAddr = (ULONG_PTR)newProc;
				return TRUE;
			}
			pThunk++;
		}
	}
	return FALSE;
}

BOOL NTAPI EATPatch32(LPCSTR ModName, LPCSTR FunName, ULONG64 ProxyFunAddr)
{
	DWORD                         Addr;
	DWORD                         Index;
	HMODULE                       hMod;
	DWORD                         Protect;
	PIMAGE_DOS_HEADER             DosHeader;
	PIMAGE_OPTIONAL_HEADER        Opthdr;
	PIMAGE_EXPORT_DIRECTORY       Export;
	PULONG                        pAddressOfFunctions;
	PULONG                        pAddressOfNames;
	PUSHORT                       pAddressOfNameOrdinals;
	BOOL                          Result;

	hMod = GetModuleHandleA(ModName);
	DosHeader = (PIMAGE_DOS_HEADER)hMod;
	Opthdr = (PIMAGE_OPTIONAL_HEADER)((DWORD)hMod + DosHeader->e_lfanew + 24);
	Export = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)DosHeader + Opthdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	pAddressOfFunctions = (ULONG*)((BYTE*)hMod + Export->AddressOfFunctions);
	pAddressOfNames = (ULONG*)((BYTE*)hMod + Export->AddressOfNames);
	pAddressOfNameOrdinals = (USHORT*)((BYTE*)hMod + Export->AddressOfNameOrdinals);
	Result = FALSE;

	for (ULONG_PTR i = 0; i <Export->NumberOfNames; i++)
	{
		Index = pAddressOfNameOrdinals[i];
		LPCSTR pFuncName = (LPCSTR)((BYTE*)hMod + pAddressOfNames[i]);

		if (lstrcmpiA((LPCSTR)pFuncName, FunName) == 0)
		{
			Addr = pAddressOfFunctions[Index];
			VirtualProtect(&pAddressOfFunctions[Index], 0x1000, PAGE_READWRITE, &Protect);
			pAddressOfFunctions[Index] = (DWORD)ProxyFunAddr - (DWORD)hMod;
			Result = TRUE;
			break;
		}
	}
	return Result;
}

BOOL NTAPI EATPatch64(LPCSTR ModName, LPCSTR FunName, ULONG64 ProxyFunAddr)
{
	HANDLE                      hMod;
	PVOID                       BaseAddress = NULL;
	IMAGE_DOS_HEADER*           pDosHeader;
	IMAGE_OPTIONAL_HEADER64*    OptHdr;
	PIMAGE_EXPORT_DIRECTORY     Exports;
	USHORT                      Index;
	DWORD                       OldProtect;
	ULONG                       Addr;
	PUCHAR                      pFuncName;
	PULONG                      pAddressOfFunctions, pAddressOfNames;
	PUSHORT                     pAddressOfNameOrdinals;
	MODULEINFO                  ModuleInfo;
	BOOL                        Result;

	Result = FALSE;
	BaseAddress = GetModuleHandleA(ModName);

	RtlZeroMemory(&ModuleInfo, sizeof(ModuleInfo));
	GetModuleInformation(GetCurrentProcess(), (HMODULE)BaseAddress, &ModuleInfo, sizeof(MODULEINFO));
	VirtualProtect(BaseAddress, ModuleInfo.SizeOfImage, PAGE_EXECUTE_READWRITE, &OldProtect);

	hMod = BaseAddress;
	pDosHeader = (IMAGE_DOS_HEADER *)hMod;
	OptHdr = (IMAGE_OPTIONAL_HEADER64 *)((BYTE*)hMod + pDosHeader->e_lfanew + 24);
	Exports = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)pDosHeader + OptHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	pAddressOfFunctions = (ULONG*)((BYTE*)hMod + Exports->AddressOfFunctions);
	pAddressOfNames = (ULONG*)((BYTE*)hMod + Exports->AddressOfNames);
	pAddressOfNameOrdinals = (USHORT*)((BYTE*)hMod + Exports->AddressOfNameOrdinals);


	for (ULONG_PTR i = 0; i < Exports->NumberOfNames; i++)
	{
		Index = pAddressOfNameOrdinals[i];
		Addr = pAddressOfFunctions[Index];
		pFuncName = (PUCHAR)((BYTE*)hMod + pAddressOfNames[i]);
		Addr = pAddressOfFunctions[Index];

		if (!lstrcmpA((LPCSTR)pFuncName, FunName))
		{
			pAddressOfFunctions[Index] = (ULONG)((ULONG64)ProxyFunAddr - (ULONG64)hMod);
			Result = TRUE;
			break;
		}
	}
	return Result;
}


FORCEINLINE NTSTATUS PatchCode(LPVOID lpStartAddress, LPVOID lpCode, ULONG_PTR Size)
{
	NTSTATUS    Status;
	DWORD       OldProtect;

	auto BOOL_TO_NTSTATUS = [](NTSTATUS& nStatus, BOOL bVar)->NTSTATUS
	{
		nStatus = bVar ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
		return nStatus;
	};

	LOOP_ONCE
	{

		if (!lpStartAddress || !lpCode || !Size)
		{
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		if (!IsStatusSuccess(
			BOOL_TO_NTSTATUS(Status,
			VirtualProtect(lpStartAddress, Size, PAGE_EXECUTE_READWRITE, &OldProtect))))
			return Status;

		RtlCopyMemory(lpStartAddress, lpCode, Size);

		if (!IsStatusSuccess(
			BOOL_TO_NTSTATUS(Status,
			VirtualProtect(lpStartAddress, Size, OldProtect, &OldProtect))))
			return Status;
	}
	Status = STATUS_SUCCESS;

	return Status;
}


static BOOL g_InlineInited = FALSE;

NTSTATUS NTAPI InlinePatchMemory(PINLINE_PATCH_DATA Data, ULONG_PTR Size)
{
	NTSTATUS        Status;
	ULONG           MinStatus;


	MinStatus = MH_OK;
	if (!g_InlineInited)
	{
		MinStatus = MH_Initialize();
		g_InlineInited = TRUE;
	}

	if (!Data || !Size)
		return STATUS_INVALID_PARAMETER;

	auto MH_STATUS_TRANSLATION = [](NTSTATUS& Status, ULONG MinStatus)->LONG
	{
		switch (MinStatus)
		{
		case MH_UNKNOWN:
			Status = STATUS_UNSUCCESSFUL;
			break;

		case MH_OK:
			Status = STATUS_SUCCESS;
			break;

		case MH_ERROR_ALREADY_INITIALIZED:
			Status = STATUS_ALREADY_REGISTERED;
			break;

			//ok
		case MH_ERROR_NOT_INITIALIZED:
			Status = STATUS_REGISTRY_RECOVERED;
			break;

		case MH_ERROR_ALREADY_CREATED:
			Status = STATUS_ALREADY_COMMITTED;
			break;

		case MH_ERROR_NOT_CREATED:
		case MH_ERROR_MODULE_NOT_FOUND:
		case MH_ERROR_ENABLED:
		case MH_ERROR_DISABLED:
			Status = STATUS_UNSUCCESSFUL;
			break;


		case MH_ERROR_NOT_EXECUTABLE:
			Status = STATUS_ILLEGAL_INSTRUCTION;
			break;

		case MH_ERROR_UNSUPPORTED_FUNCTION:
			Status = STATUS_ILLEGAL_FUNCTION;
			break;

		case MH_ERROR_MEMORY_ALLOC:
			Status = STATUS_MEMORY_NOT_ALLOCATED;
			break;

		case MH_ERROR_MEMORY_PROTECT:
			Status = STATUS_ACCESS_DENIED;
			break;

		case MH_ERROR_FUNCTION_NOT_FOUND:
			Status = STATUS_ILLEGAL_FUNCTION;
			break;
		}
		return Status;
	};

	//PrintConsoleA("Inline Status : %d\n", MinStatus);

	if (!IsStatusSuccess(MH_STATUS_TRANSLATION(Status, MinStatus)))
		return Status;


	for (ULONG_PTR i = 0; i < Size; i++)
	{
		if (!IsStatusSuccess(
			MH_STATUS_TRANSLATION(Status,
			MH_CreateHook(Data[i].pTarget, Data[i].pHook, Data[i].pDetour))))
			return Status;
	}

	if (!IsStatusSuccess(MH_STATUS_TRANSLATION(Status, MH_EnableHook(MH_ALL_HOOKS))))
	{
		Status = STATUS_INVALID_HANDLE;
		return Status;
	}

	return Status;
}

NTSTATUS NTAPI IATPatchMemory(PIAT_PATCH_DATA Data, ULONG_PTR Size)
{
	NTSTATUS        Status;

	if (!Data || !Size)
		return STATUS_INVALID_PARAMETER;

	auto BOOL_TO_NTSTATUS = [](NTSTATUS& nStatus, BOOL bVar)->NTSTATUS
	{
		nStatus = bVar ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
		return nStatus;
	};

	for (ULONG_PTR i = 0; i < Size; i++)
	{
		if (!IsStatusSuccess(
			BOOL_TO_NTSTATUS(
			Status,
#ifdef  _M_X64
			IATPatch64(
#else
			IATPatch32(
#endif
			Data[i].hModule,
			Data[i].DllName,
			Data[i].SourceFunction,
			Data[i].DestFunction))))
		{
			Status = STATUS_UNSUCCESSFUL;
		}
	}
	return Status;
}

NTSTATUS NTAPI EATPatchMemory(PEAT_PATCH_DATA Data, ULONG_PTR Size)
{
	NTSTATUS        Status;

	if (!Data || !Size)
		return STATUS_INVALID_PARAMETER;

	auto BOOL_TO_NTSTATUS = [](NTSTATUS& nStatus, BOOL bVar)->NTSTATUS
	{
		nStatus = bVar ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
		return nStatus;
	};

	for (ULONG_PTR i = 0; i < Size; i++)
	{
		if (!IsStatusSuccess(
			BOOL_TO_NTSTATUS(
			Status,
#ifdef  _M_X64
			EATPatch64(
#else
			EATPatch32(
#endif
			Data[i].ModName,
			Data[i].FunName,
			Data[i].ProxyFunAddr))))
			return Status;
	}
	return Status;
}


NTSTATUS NTAPI CodePatchMemory(PCODE_PATCH_DATA Data, ULONG_PTR Size)
{
	NTSTATUS  Status;

	if (!Data || !Size)
		return STATUS_INVALID_PARAMETER;

	auto GET_STATUS = [](NTSTATUS& nStatus, NTSTATUS ParamStatus)->NTSTATUS
	{
		nStatus = ParamStatus;
		return nStatus;
	};

	for (ULONG_PTR i = 0; i < Size; i++)
	{
		if (!IsStatusSuccess(GET_STATUS(Status, PatchCode(Data[i].lpAddress, Data[i].pCode, Data[i].CodeSize))))
			return Status;
	}

	return Status;
}


NTSTATUS NTAPI SetNopCode(LPBYTE Code, ULONG_PTR Size)
{
	DWORD      OldProtect;
	NTSTATUS   Status;
	BOOL       Success;

	Status = STATUS_UNSUCCESSFUL;
	Success = VirtualProtect(Code, Size, PAGE_EXECUTE_READWRITE, &OldProtect);
	if (!Success)
		return Status;

	RtlFillMemory(Code, Size, 0x90);

	Success = VirtualProtect(Code, Size, OldProtect, &OldProtect);
	if (!Success)
		return Status;

	Status = STATUS_SUCCESS;
	return Status;
}

