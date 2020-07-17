#include "XP3FilterModify.h"

LONG ExceptionFilterV2(LPEXCEPTION_POINTERS pExceptionPointers, HMODULE hModule, ULONG SizeOfImage)
{
	ULONG_PTR Address;

	switch (pExceptionPointers->ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		Address = pExceptionPointers->ExceptionRecord->ExceptionInformation[1];
		if (Address < (ULONG_PTR)hModule ||
			Address >(ULONG_PTR)hModule + SizeOfImage ||
			pExceptionPointers->ExceptionRecord->ExceptionInformation[0] != 1)
		{
		}
		else
		{
			break;
		}

	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}


TVPXP3ArchiveExtractionFilterFunc GetExtractionFilterV2()
{
	ULONG                               SizeOfImage, OldProtection;
	HMODULE                             hModule;
	PIMAGE_DOS_HEADER                   pDosHeader;
	PIMAGE_NT_HEADERS                   pNtHeader;
	PEXCEPTION_POINTERS                 pExceptionPointers;
	TVPXP3ArchiveExtractionFilterFunc   pfFilter;

	hModule = GetModuleHandleW(NULL);
	pDosHeader = (PIMAGE_DOS_HEADER)hModule;
	pNtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)hModule + pDosHeader->e_lfanew);
	SizeOfImage = pNtHeader->OptionalHeader.SizeOfImage;

	if (!VirtualProtect(hModule, SizeOfImage, PAGE_EXECUTE_READ, &OldProtection))
		return NULL;

	pExceptionPointers = NULL;

	__try
	{
		pfFilter = (TVPXP3ArchiveExtractionFilterFunc)0x23333333;
		TVPSetXP3ArchiveExtractionFilter(pfFilter);
	}
	__except(ExceptionFilterV2(pExceptionPointers = GetExceptionInformation(), hModule, SizeOfImage))
	{
		VirtualProtect(hModule, SizeOfImage, PAGE_EXECUTE_READWRITE, &OldProtection);
		*(PULONG_PTR)&pfFilter = *(PULONG_PTR)pExceptionPointers->ExceptionRecord->ExceptionInformation[1];
	}

	return pfFilter;
}


ULONG_PTR g_FakeReturnAddress = NULL;
TVPXP3ArchiveExtractionFilterFunc g_RealFilter = nullptr;
ULONG_PTR g_FakeExtractionFilter = NULL;
GLOBAL_INFO *g_pInfo = nullptr;


void WINAPI SetRealFilter(TVPXP3ArchiveExtractionFilterFunc Filter)
{
	g_RealFilter = Filter;
}

void __declspec(naked) WINAPI FakeExtractionFilterAsm(tTVPXP3ExtractionFilterInfo * /* info */)
{
	__asm
	{
		mov ecx, g_RealFilter;
		jecxz  NO_EXT_FILTER;
		mov eax, g_FakeReturnAddress;
		mov[esp], eax;
		jmp ecx;

		NO_EXT_FILTER:
		ret 4;
	}
}

void WINAPI FakeExtractionFilterWithException(tTVPXP3ExtractionFilterInfo *info)
{
	__try
	{
		FakeExtractionFilterAsm(info);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

void __declspec(naked) WINAPI FakeExtractionFilter(tTVPXP3ExtractionFilterInfo* /* info */)
{
	__asm
	{
		push[esp + 4];
		call g_FakeExtractionFilter;
		ret 4;
	}
}

ULONG_PTR FindReturnAddressWorker()
{
	ULONG State;
	PBYTE Buffer;
	GLOBAL_INFO *pInfo = g_pInfo;

	enum { FOUND_NONE, FOUND_C2, FOUND_04, FOUND_00 };

	Buffer = (PBYTE)pInfo->FirstSectionAddress;

	State = FOUND_NONE;
	for (ULONG SizeOfImage = pInfo->SizeOfImage; SizeOfImage; ++Buffer, --SizeOfImage)
	{
		ULONG b = Buffer[0];

		switch (State)
		{
		case FOUND_NONE:
			if (b == 0xC2)
				State = FOUND_C2;
			break;

		case FOUND_C2:
			switch (b)
			{
			case 0x04:
				State = FOUND_04;
				break;

			case 0xC2:
				State = FOUND_C2;
				break;

			default:
				State = FOUND_NONE;
			}
			break;

		case FOUND_04:
			switch (b)
			{
			case 0x00:
				State = FOUND_00;
				break;

			case 0xC2:
				State = FOUND_C2;
				break;

			default:
				State = FOUND_NONE;
			}
			break;

		case FOUND_00:
			return (ULONG_PTR)Buffer - 3;
		}
	}
	return NULL;
}

ULONG_PTR FindReturnAddress()
{
	__try
	{
	ULONG_PTR ReturnAddress = FindReturnAddressWorker();

	if (ReturnAddress != NULL)
	{
		g_FakeExtractionFilter = (ULONG_PTR)FakeExtractionFilterAsm;
		return ReturnAddress;
	}
}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}

	ULONG OldProtect;
	ULONG_PTR ReturnAddress = (ULONG_PTR)GetModuleHandleW(NULL) + 0xFE0;

	VirtualProtect((PVOID)ReturnAddress, 1, PAGE_READONLY, &OldProtect);
	g_FakeExtractionFilter = (ULONG_PTR)FakeExtractionFilterWithException;

	return ReturnAddress;
}


TVPXP3ArchiveExtractionFilterFunc WINAPI InitFakeFilter()
{
	PIMAGE_DOS_HEADER       DosHeader;
	PIMAGE_NT_HEADERS       NtHeader;
	PIMAGE_SECTION_HEADER   SectionHeader;
	GLOBAL_INFO             *pInfo;

	g_FakeReturnAddress = FindReturnAddress();

	DosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
	NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);
	SectionHeader   = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeader + NtHeader->FileHeader.SizeOfOptionalHeader + sizeof(*NtHeader) - sizeof(NtHeader->OptionalHeader));

	pInfo = (GLOBAL_INFO *)CMem::AllocZero(sizeof(*pInfo));
	pInfo->FirstSectionAddress  = SectionHeader->VirtualAddress + (ULONG_PTR)DosHeader;
	pInfo->SizeOfImage          = NtHeader->OptionalHeader.SizeOfImage;

	return FakeExtractionFilter;
}


