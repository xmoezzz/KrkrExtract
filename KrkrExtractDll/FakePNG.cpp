#include "FakePNG.h"
#include "GlobalInit.h"


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


Void WINAPI SavePng(LPCWSTR filename, LPCWSTR path)
{
	const wstring ConstU = L"tmpLayer.saveLayerImagePng(\"";
	wstring str, fname = filename;
	str = L"tmpLayer.loadImages(\"" + fname + L"\");";
	TVPExecuteScript(ttstr(str.c_str()));
	TVPExecuteScript(ttstr(L"tmpLayer.setSizeToImageSize();"));
	str = ConstU + wstring(path) + L"\");";
	TVPExecuteScript(ttstr(str.c_str()));
}
