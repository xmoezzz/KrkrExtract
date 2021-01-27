#pragma once

#include "my.h"
#include "ml.h"
#include "tp_stub.h"
#include <KrkrClientProxyer.h>
#include <memory>


#define KRKR2_TLG5_MAGIC "TLG5.0"
#define KRKR2_TLG6_MAGIC "TLG6.0"
#define KRKR2_TLG0_MAGIC "TLG0.0"

#define KRKR2_TLG5_TAG TAG4('TLG5')
#define KRKR2_TLG6_TAG TAG4('TLG6')
#define KRKR2_TLG0_TAG TAG4('TLG0')

NTSTATUS InitLayer(KrkrClientProxyer* Proxyer);
NTSTATUS SavePng(LPCWSTR FileName, LPCWSTR Path);
NTSTATUS SaveAmv(LPCWSTR FileName, LPCWSTR Path);

HRESULT WINAPI ImageWorkerV2Link(iTVPFunctionExporter *exporter);

NTSTATUS 
DumpPsbTjs2(
	KrkrClientProxyer* Proxyer,
	IStream*           Stream,
	BOOL               SaveText,
	BOOL               SaveImage,
	BOOL               SaveScript,
	const wstring&     ExtName,
	PCWSTR             OutputFileName
);

NTSTATUS DecodeText(KrkrClientProxyer* Proxyer, PCWSTR OutFileName, PBYTE Buffer, ULONG Size);
NTSTATUS DecompilePsbJson(KrkrClientProxyer* Proxyer, IStream* PsbStream, LPCWSTR BasePath, LPCWSTR FileName);
NTSTATUS SavePbd(LPCWSTR FileName, LPCWSTR Path);
NTSTATUS DecodeTLG5(PBYTE InBuffer, ULONG InSize, std::shared_ptr<BYTE>& OutBuffer, ULONG& OutSize);
NTSTATUS DecodeTLG6(PBYTE InBuffer, ULONG InSize, std::shared_ptr<BYTE>& OutBuffer, ULONG& OutSize);


NTSTATUS Bmp2PNG(PBYTE Buffer, ULONG Size, PCWSTR pngfn);
NTSTATUS Bmp2JPG(PBYTE Buffer, ULONG Size, PCWSTR Path);
NTSTATUS Image2JPG(PBYTE Buffer, ULONG Size, PCWSTR Path);

