#include "FakePNG.h"
#include "KrkrExtract.h"


static BOOL Inited = FALSE;


Void WINAPI InitLayer()
{
	GlobalData*  Info;

	Info = GlobalData::GetGlobalData();
	if (Inited == TRUE)
		return;

	if(GlobalData::GetGlobalData()->DebugOn) 
		PrintConsoleW(L"Plugin loaded\n");
	
	TVPExecuteScript(ttstr(L"var tmpWinKrkrExtract=new Window();"));
	TVPExecuteScript(ttstr(L"var tmpLayer=new Layer(tmpWinKrkrExtract, null);"));
	TVPExecuteScript(ttstr(L"tmpLayer.opacity=255;"));

	if (GlobalData::GetGlobalData()->DebugOn) 
		PrintConsoleW(L"Layer Inited : ok\n");

	Inited = TRUE;
}


Void WINAPI SavePng(LPCWSTR FileName, LPCWSTR Path)
{
	wstring Code;

	static WCHAR ShellCode[] = L"tmpLayer.saveLayerImagePng(\"";

	Code =  L"tmpLayer.loadImages(\"";
	Code += FileName;
	Code += L"\");";

	TVPExecuteScript(ttstr(Code.c_str()));
	TVPExecuteScript(ttstr(L"tmpLayer.setSizeToImageSize();"));

	Code =  ShellCode;
	Code += Path;
	Code += L"\");";

	TVPExecuteScript(ttstr(Code.c_str()));
}
