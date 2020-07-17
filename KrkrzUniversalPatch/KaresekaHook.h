#pragma once

#include "my.h"

typedef HRESULT(NTAPI *FuncV2Link)(iTVPFunctionExporter *);
typedef tTJSBinaryStream* (FASTCALL * FuncCreateStream)(const ttstr &, tjs_uint32);
typedef PVOID(CDECL * FuncHostAlloc)(ULONG);

class KaresekaHook
{
public:
	KaresekaHook();
	static KaresekaHook* Handle;

	BOOL     Init(HMODULE hModule);
	BOOL     UnInit();
	NTSTATUS InitFileSystemXP3();
	IStream* CreateLocalStream(LPCWSTR lpFileName);
	NTSTATUS InitKrkrHook(LPCWSTR lpFileName, PVOID Module);
	NTSTATUS QueryFile(LPCWSTR QueryPathName, LPCWSTR FileName, PBYTE& FileBuffer, ULONG& FileSize, ULONG64& Hash);
	NTSTATUS QueryFileXP3(LPWSTR FileName, PBYTE& Buffer, ULONG& Size, ULONG64& Hash);

	FuncCreateStream      StubTVPCreateStream;
	FuncHostAlloc         StubHostAlloc;
	FuncV2Link            StubV2Link;
	ULONG_PTR             IStreamAdapterVtable;
	iTVPFunctionExporter* TVPFunctionExporter;
	PVOID                 m_SelfModule;


private:
	BOOL       Inited;
	BOOL       FileSystemInited;
};

KaresekaHook* FASTCALL GetKareseka();