#pragma once

#include <my.h>
#include "tp_stub.h"
#include "KrkrExtract.h"
#include "XP3Parser.h"
#include "FakePNG.h"
#include "TLGDecoder.h"
#include "PNGWorker.h"
#include <Shlwapi.h> 
#include "KrkrDumperBase.h"


class KrkrDumper : public KrkrDumperBase
{

public:
	KrkrDumper();
	~KrkrDumper();

	HANDLE hThread;
	BOOL   IsIndex;
	WCHAR  FileName[MAX_PATH];
	WCHAR  FileNameIndex[MAX_PATH];

	NTSTATUS NTAPI DoDump();
	VOID     NTAPI SetFile(LPCWSTR lpFile);
	VOID     NTAPI InternalReset();

private:
	VOID NTAPI AddPath(LPWSTR lpPath);

	NTSTATUS NTAPI DumpFileByIStream(ttstr M2Prefix, ttstr NormalPrefix);
	NTSTATUS NTAPI DumpFileByRawFile();

	NTSTATUS WINAPI ParseXP3File(PWCHAR lpFileName);
	NTSTATUS WINAPI ParseXP3iFile(PWCHAR lpFileName);
	NTSTATUS WINAPI ProcessXP3Archive(LPCWSTR FileName, NtFileDisk& file);
	NTSTATUS WINAPI DumpFile();
};

