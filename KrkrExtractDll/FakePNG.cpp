#include "FakePNG.h"
#include "Console.h"
#include "GlobalInit.h"


static BOOL Inited = FALSE;
void WINAPI InitLayer()
{
	if (GlobalData::GetGlobalData()->hImageModule == nullptr || Inited == TRUE)
	{
		return;
	}

	OutputString(L"Plugin loaded\n");
	typedef HRESULT(_stdcall *tlink)(iTVPFunctionExporter *);
	tlink v2link = (tlink)GetProcAddress(GlobalData::GetGlobalData()->hImageModule, "V2Link");
	v2link(GlobalData::GetGlobalData()->TVPFunctionExporter);
	
	TVPExecuteScript(ttstr(L"var tmpWinKrkrExtract=new Window();"));
	TVPExecuteScript(ttstr(L"var tmpLayer=new Layer(tmpWinKrkrExtract, null);"));
	TVPExecuteScript(ttstr(L"tmpLayer.opacity=255;"));

	OutputString(L"Layer Inited : ok\n");
	Inited = TRUE;
}


void WINAPI SavePng(const wchar_t * filename, const wchar_t * path)
{
	const wstring ConstU = L"tmpLayer.saveLayerImagePng(\"";
	wstring str, fname = filename;
	str = L"tmpLayer.loadImages(\"" + fname + L"\");";
	TVPExecuteScript(ttstr(str.c_str()));
	TVPExecuteScript(ttstr(L"tmpLayer.setSizeToImageSize();"));
	str = ConstU + wstring(path) + L"\");";
	TVPExecuteScript(ttstr(str.c_str()));
}
