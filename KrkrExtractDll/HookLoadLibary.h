#ifndef _HookLoadLibary_
#define _HookLoadLibary_

#include <Windows.h>

#pragma comment(lib, "ntdll.lib")

EXTERN_C
{
	__declspec(dllimport)
		BOOLEAN
		NTAPI
		RtlFreeHeap(
		__in PVOID HeapHandle,
		__in_opt ULONG Flags,
		__in __post_invalid  PVOID BaseAddress
		);


	__declspec(dllimport)
		PVOID
		NTAPI
		RtlAllocateHeap(
		__in PVOID HeapHandle,
		__in_opt ULONG Flags,
		__in SIZE_T Size
		);

	__declspec(dllimport)
		UINT32
		NTAPI
		RtlUnicodeToMultiByteN(
		__out_bcount_part(MaxBytesInMultiByteString, *BytesInMultiByteString) PCHAR MultiByteString,
		__in ULONG MaxBytesInMultiByteString,
		__out_opt PULONG BytesInMultiByteString,
		__in_bcount(BytesInUnicodeString) PCWCH UnicodeString,
		__in ULONG BytesInUnicodeString
		);

	__declspec(dllimport)
		UINT32 
		NTAPI
		RtlMultiByteToUnicodeN(
		_Out_           PWCH   UnicodeString,
		_In_            ULONG  MaxBytesInUnicodeString,
		_Out_opt_       PULONG BytesInUnicodeString,
		_In_      const CHAR   *MultiByteString,
		_In_            ULONG  BytesInMultiByteString
		);
}



HRESULT WINAPI InitHook();

#endif
