#include "Tjs2Disasm.h"
#include "tjs2/tjsByteCodeLoader.h"


class CConsoleOutput : public iTJSConsoleOutput
{
public:
	NtFileDisk  File;

	CConsoleOutput(PCWSTR lpFileName)
	{
		static BYTE Bom[] = { 0xFF, 0xFE };

		File.Create(lpFileName);
		File.Write(Bom, 2);
	}

	~CConsoleOutput()
	{
		File.Close();
	}
	virtual void ExceptionPrint(const tjs_char *msg)
	{
	};

	virtual void Print(const tjs_char *msg)
	{
		LARGE_INTEGER Bytes;

		File.Print(&Bytes, L"%ls\r\n", msg);
	};
};


NTSTATUS FASTCALL TjsDecompileStorage(IStream* Stream, std::wstring& FileName)
{
	ULONG                       FileSize;
	PBYTE                       FileBuffer;
	ShinkuTJS::tTJSScriptBlock *blk;
	ShinkuTJS::tTJS            *TVPScriptEngine;
	STATSTG                     Stat;
	ULONG                       Bytes;
	tTJSByteCodeLoader          Loader;

	if (!Stream)
		return STATUS_INVALID_PARAMETER;

	TVPScriptEngine = new ShinkuTJS::tTJS();
	if (!TVPScriptEngine)
		return STATUS_NO_MEMORY;

	Stream->Stat(&Stat, STATFLAG_DEFAULT);
	FileSize = Stat.cbSize.LowPart;
	FileBuffer = (PBYTE)AllocateMemoryP(FileSize);

	if (!FileBuffer)
	{
		TVPScriptEngine->Release();
		return STATUS_NO_MEMORY;
	}

	Stream->Read(FileBuffer, FileSize, &Bytes);

	PrintConsoleW(L"%ls\n", FileName.c_str());
	iTJSConsoleOutput* consoleOutput = new CConsoleOutput(FileName.c_str());
	TVPScriptEngine->SetConsoleOutput(consoleOutput);
	try
	{
		blk = Loader.ReadByteCode(TVPScriptEngine, L"", FileBuffer, FileSize);
		blk->Dump();
	}
	catch (std::exception& e)
	{
		PrintConsoleA("err : %s\n", e.what());
		TVPScriptEngine->Release();
		delete consoleOutput;

		FreeMemoryP(FileBuffer);
		return STATUS_UNSUCCESSFUL;
	}

	TVPScriptEngine->Release();
	delete consoleOutput;

	FreeMemoryP(FileBuffer);
	return STATUS_SUCCESS;
}


