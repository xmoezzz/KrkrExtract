#pragma once

#include "my.h"
#include "ml.h"
#include "KrkrHeaders.h"
#include "KrkrUniversalDumper.h"
#include "KrkrDumperBase.h"
#include <unordered_set>

class KrkrUniversalDumper : public KrkrDumperBase
{

public:
	KrkrUniversalDumper();
	~KrkrUniversalDumper();

	NTSTATUS NTAPI DoDump();
	VOID     NTAPI InternalReset();

	HANDLE   hThread;

private:
	VOID NTAPI AddPath(LPWSTR lpPath);
	NTSTATUS WINAPI DumpFile();

	std::unordered_set<wstring> FileList;
};

HANDLE NTAPI StartUniversalDumper();

