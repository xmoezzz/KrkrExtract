#include <Windows.h>
#include "tjs.h"
#include "TextStream.h"
#include "TVPStream.h"
#include "WinFile.h"
#include "tjsByteCodeLoader.h"

class CConsoleOutput : public iTJSConsoleOutput
{
public:
	FILE* file;
	CConsoleOutput()
	{
		static BYTE Bom[] = { 0xFF, 0xFE };
		file = _wfopen(L"res.txt", L"wb");
		fwrite(Bom, 1, 2, file);
	}
	~CConsoleOutput()
	{
		if (file) fclose(file);
		file = NULL;
	}
	virtual void ExceptionPrint(const tjs_char *msg)
	{
		wprintf(msg);
	};

	virtual void Print(const tjs_char *msg)
	{
		fwprintf(file, L"%ls\r\n", msg);
	};
};

void TVPDecompileStorage(const ttstr& name, const ttstr& outputpath)
{
	WinFile  File;
	ULONG    FileSize;
	PBYTE    FileBuffer;

	tTJS *TVPScriptEngine = new tTJS();

	if (File.Open(name.c_str(), WinFile::FileRead) != S_OK)
		return;

	FileSize   = File.GetSize32();
	FileBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, FileSize);

	if (!FileBuffer)
		return;


	File.Read(FileBuffer, FileSize);

	iTJSConsoleOutput* consoleOutput = new CConsoleOutput();
	TVPScriptEngine->SetConsoleOutput(consoleOutput);

	tTJSByteCodeLoader Loader;
	tTJSScriptBlock *blk = Loader.ReadByteCode(TVPScriptEngine, L"", FileBuffer, FileSize);
	blk->Dump();

	HeapFree(GetProcessHeap(), 0, FileBuffer);
}

//-------TJS2 Decompiler-----------
int wmain(int argc, WCHAR* argv[])
{
	if (argc != 2 && argc != 3)
		return 0;

	TVPDecompileStorage(ttstr(argv[1]), argc == 3 ? ttstr(argv[2]) : ttstr(argv[1]) + L".txt");
	return 0;
}
