#pragma once
#include "my.h"
#include "ml.h"
#include "KrkrHeaders.h"

class KrkrDumperBase
{
public:
	KrkrDumperBase() {}
	~KrkrDumperBase() {}

	NTSTATUS NTAPI ProcessFile(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessPNG(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessTLG(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessTEXT(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessPBD(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessPSB(IStream* Stream, LPCWSTR OutFileName, wstring& ExtName);
};


wstring FASTCALL FormatPathFull(LPCWSTR Path);
