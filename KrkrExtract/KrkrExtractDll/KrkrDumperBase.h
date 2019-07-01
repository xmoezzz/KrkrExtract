#pragma once
#include "my.h"
#include "ml.h"
#include "KrkrHeaders.h"
#include <atomic>

class KrkrDumperBase
{
public:
	KrkrDumperBase() {}
	~KrkrDumperBase() 
	{
	}

	NTSTATUS NTAPI ProcessFile(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessPNG(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessTLG(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessTEXT(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessPBD(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessAMV(IStream* Stream, LPCWSTR OutFileName);
	NTSTATUS NTAPI ProcessPSB(IStream* Stream, LPCWSTR OutFileName, wstring& ExtName);

	NTSTATUS NTAPI CreateAmvExtractor();
	NTSTATUS NTAPI CleanAmvExtractor();

private:
	NTSTATUS NTAPI ProcessAmvToPNG(vector<wstring>& FileList, wstring DestDir);
	NTSTATUS NTAPI ProcessAmvToJPG(vector<wstring>& FileList, wstring DestDir);
	NTSTATUS NTAPI ProcessAmvToGIF(vector<wstring>& FileList, wstring OriName, wstring DestDir);

	std::wstring      BasePath;
};


wstring FASTCALL FormatPathFull(LPCWSTR Path);

