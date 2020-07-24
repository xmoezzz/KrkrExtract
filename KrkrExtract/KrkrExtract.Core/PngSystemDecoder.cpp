#include "Decoder.h"
#include "KrkrExtract.h"
#include <atomic>

static BOOL g_LayerInited = FALSE;


NTSTATUS InitLayer(KrkrClientProxyer* Proxyer)
{
	if (g_LayerInited == TRUE)
		return STATUS_SUCCESS;

	try 
	{

		TVPExecuteScript(ttstr(L"var tmpWinKrkrExtract=new Window();"));
		TVPExecuteScript(ttstr(L"var tmpLayer=new Layer(tmpWinKrkrExtract, null);"));
		TVPExecuteScript(ttstr(L"tmpLayer.opacity=255;"));
		g_LayerInited = TRUE;
	}
	catch (...) {
		Proxyer->TellServerLogOutput(LogLevel::LOG_WARN, L"Layer Initialization : failed");
	}

	if (g_LayerInited) {
		Proxyer->TellServerLogOutput(LogLevel::LOG_INFO, L"Layer Initialization : ok");
	}

	return g_LayerInited ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


NTSTATUS SavePng(LPCWSTR FileName, LPCWSTR Path)
{
	wstring Code;

	static WCHAR ShellCode[] = L"tmpLayer.saveLayerImagePng(\"";

	try {
		Code = L"tmpLayer.loadImages(\"";
		Code += FileName;
		Code += L"\");";

		TVPExecuteScript(ttstr(Code.c_str()));
		TVPExecuteScript(ttstr(L"tmpLayer.setSizeToImageSize();"));

		Code = ShellCode;
		Code += Path;
		Code += L"\");";

		TVPExecuteScript(ttstr(Code.c_str()));
	}
	catch (...) {
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}
