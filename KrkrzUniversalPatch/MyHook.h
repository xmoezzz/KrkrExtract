#pragma once
#include "my.h"

typedef struct _IAT_PATCH_DATA
{
	LPVOID   hModule;
	LPVOID   SourceFunction;
	LPVOID   DestFunction;
	LPCSTR   DllName;
}IAT_PATCH_DATA, *PIAT_PATCH_DATA;

typedef struct _EAT_PATCH_DATA
{
	LPCSTR   ModName;
	LPCSTR   FunName;
	ULONG64  ProxyFunAddr;
}EAT_PATCH_DATA, *PEAT_PATCH_DATA;

typedef struct _INLINE_PATCH_DATA
{
	LPVOID   pTarget;
	LPVOID   pHook;
	LPVOID*  pDetour;
}INLINE_PATCH_DATA, *PINLINE_PATCH_DATA;

typedef struct _CODE_PATCH_DATA
{
	LPVOID   lpAddress;
	LPVOID   pCode;
	ULONG    CodeSize;
}CODE_PATCH_DATA, *PCODE_PATCH_DATA;


enum HOOK_INFO
{
	IAT_HOOK,
	EAT_HOOK,
	INLINE_HOOK,
	MEMORY_HOOK
};

NTSTATUS NTAPI InlinePatchMemory(PINLINE_PATCH_DATA Data, ULONG_PTR Size);
NTSTATUS NTAPI IATPatchMemory(PIAT_PATCH_DATA    Data, ULONG_PTR Size);
NTSTATUS NTAPI EATPatchMemory(PEAT_PATCH_DATA    Data, ULONG_PTR Size);
NTSTATUS NTAPI CodePatchMemory(PCODE_PATCH_DATA   Data, ULONG_PTR Size);

NTSTATUS NTAPI SetNopCode(LPBYTE Code, ULONG_PTR Size);

//DYNAMIC_ADDRESS
