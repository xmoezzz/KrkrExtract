#pragma once

#include <my.h>
#include "tp_stub.h"
#include <set>
#include <string>

typedef HRESULT(NTAPI *FuncV2Link)(iTVPFunctionExporter *);
typedef tTJSBinaryStream* (__fastcall * FuncCreateStream)(const ttstr &, tjs_uint32);
typedef PVOID(CDECL * FuncHostAlloc)(ULONG);


PVOID NTAPI XmoeAllocateMemory(ULONG_PTR Size);
VOID  NTAPI XmoeFreeMemory(PVOID Mem);


NTSTATUS NTAPI InitFileSystem(API_POINTER(XmoeAllocateMemory) AllocMemFun, API_POINTER(XmoeFreeMemory) FreeMemoryFun);
NTSTATUS NTAPI QueryFile(LPCWSTR FileName, PBYTE* FileBuffer, ULONG* FileSize);

class KaresekaHook
{
public:
	KaresekaHook();
	static KaresekaHook* Handle;

	BOOL     Init(HMODULE hModule);
	BOOL     UnInit();
	IStream* CreateLocalStream(LPCWSTR lpFileName);
	NTSTATUS InitKrkrHook(LPCWSTR lpFileName, PVOID Module);
	NTSTATUS QueryFile(LPCWSTR QueryPathName, LPCWSTR FileName, PBYTE& FileBuffer, ULONG& FileSize, ULONG64& Hash);
	NTSTATUS QueryFileInternal (LPCWSTR QueryPathName, LPCWSTR FileName, PBYTE& FileBuffer, ULONG& FileSize, ULONG64& Hash);

	FuncCreateStream      StubTVPCreateStream;
	FuncHostAlloc         StubHostAlloc;
	FuncV2Link            StubV2Link;
	ULONG_PTR             IStreamAdapterVtable;
	iTVPFunctionExporter* TVPFunctionExporter;
	PVOID                 m_SelfModule;

	API_POINTER(InitFileSystem) XmoeInitFileSystem;
	API_POINTER(::QueryFile)    XmoeQueryFile;

private:
	BOOL                   Inited;
	BOOL                   FileSystemInited;
};

KaresekaHook* __fastcall GetKareseka();
