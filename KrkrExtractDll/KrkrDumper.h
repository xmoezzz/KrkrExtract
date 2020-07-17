#pragma once

#include <my.h>
#include "tp_stub.h"
#include "KrkrExtract.h"
#include "XP3Parser.h"
#include "FakePNG.h"
#include "TLGDecoder.h"
#include "PNGWorker.h"
#include <Shlwapi.h> 
#include <shcore.h>

class KrkrDumper
{

public:
	KrkrDumper();
	~KrkrDumper();

	HANDLE         hThread;
	BOOL           IsIndex;
	WCHAR          FileName[MAX_PATH];
	WCHAR          FileNameIndex[MAX_PATH];
	ITaskbarList3* TaskList;

	NTSTATUS NTAPI DoDump();
	Void     NTAPI SetFile(LPCWSTR lpFile);
	Void     NTAPI InternalReset();

private:
	Void NTAPI AddPath(LPWSTR lpPath);

	NTSTATUS NTAPI DumpFileByIStream(ttstr M2Prefix, ttstr NormalPrefix);
	NTSTATUS NTAPI DumpFileByRawFile();

	NTSTATUS WINAPI ParseXP3File(PWCHAR lpFileName);
	NTSTATUS WINAPI ParseXP3iFile(PWCHAR lpFileName);
	NTSTATUS WINAPI ProcessXP3Archive(LPCWSTR FileName, NtFileDisk& file);
	NTSTATUS WINAPI DumpFile();


	NTSTATUS NTAPI ProcessFile(IStream* Stream, LPCWSTR OutFileName, XP3Index& it);
	NTSTATUS NTAPI ProcessPNG(IStream* Stream, LPCWSTR OutFileName, XP3Index& it);
	NTSTATUS NTAPI ProcessTLG(IStream* Stream, LPCWSTR OutFileName, XP3Index& it);
	NTSTATUS NTAPI ProcessTEXT(IStream* Stream, LPCWSTR OutFileName, XP3Index& it);
	NTSTATUS NTAPI ProcessPSB(IStream* Stream, LPCWSTR OutFileName, XP3Index& it, wstring& ExtName);
};

