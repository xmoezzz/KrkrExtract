#include "DecryptionCode.h"

#define SEH_TRY    __try
#define SEH_EXCEPT __except


LONG ExceptionFilter(LPEXCEPTION_POINTERS pExceptionPointers, HMODULE hModule, ULONG SizeOfImage)
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

tTVPXP3ArchiveExtractionFilter GetExtractionFilter()
{
	ULONG                               SizeOfImage, OldProtection;
	HMODULE                             hModule;
	PIMAGE_DOS_HEADER                   pDosHeader;
	PIMAGE_NT_HEADERS                   pNtHeader;
	PEXCEPTION_POINTERS                 pExceptionPointers;
	tTVPXP3ArchiveExtractionFilter      pfFilter;

	hModule = GetModuleHandleW(NULL);
	pDosHeader = (PIMAGE_DOS_HEADER)hModule;
	pNtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)hModule + pDosHeader->e_lfanew);
	SizeOfImage = pNtHeader->OptionalHeader.SizeOfImage;

	if (!VirtualProtect(hModule, SizeOfImage, PAGE_EXECUTE_READ, &OldProtection))
	{
		return NULL;
	}

	pExceptionPointers = NULL;

	SEH_TRY
	{
		pfFilter = (tTVPXP3ArchiveExtractionFilter)0x23333333;
		TVPSetXP3ArchiveExtractionFilter(pfFilter);
	}
	SEH_EXCEPT(ExceptionFilter(pExceptionPointers = GetExceptionInformation(), hModule, SizeOfImage))
	{
		VirtualProtect(hModule, SizeOfImage, PAGE_EXECUTE_READWRITE, &OldProtection);
		*(PULONG_PTR)&pfFilter = *(PULONG_PTR)pExceptionPointers->ExceptionRecord->ExceptionInformation[1];
	}
	return pfFilter;
}


wstring ToLowerString(const WCHAR* lpString)
{
	wstring result;
	WCHAR* StringInfo = (WCHAR*)CMem::Alloc((lstrlenW(lpString) + 1) * 2);
	RtlCopyMemory(StringInfo, lpString, (lstrlenW(lpString) + 1) * 2);
	for (int index = 0; index < lstrlenW(StringInfo); index++)
	{
		if (StringInfo[index] <= 0x00FF && StringInfo[index] > 0)
		{
			unsigned char NarrowChar = (unsigned char)StringInfo[index];
			if (isalpha(NarrowChar))
			{
				if (isupper(NarrowChar))
				{
					WCHAR WideChar = (WCHAR)tolower(NarrowChar);
					StringInfo[index] = WideChar;
				}
			}
		}
	}
	result = StringInfo;
	CMem::Free(StringInfo);
	return result;
}
