#pragma once

#include "my.h"
#include "ml.h"
#include "KrkrHeaders.h"

class KrkrUniversalDumper
{

public:
	KrkrUniversalDumper();
	~KrkrUniversalDumper();

	NTSTATUS NTAPI DoDump();
	Void     NTAPI InternalReset();

	HANDLE   hThread;

private:
	Void NTAPI AddPath(LPWSTR lpPath);

	NTSTATUS WINAPI DumpFile();

	NTSTATUS NTAPI ProcessFile(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessPNG(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessTLG(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessTEXT(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessPSB(IStream* Stream, LPCWSTR OutFileName, wstring& ExtName);

};

HANDLE NTAPI StartUniversalDumper();

