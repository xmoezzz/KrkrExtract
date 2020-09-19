#include "Decoder.h"
#include "KrkrExtract.h"

static PCWSTR g_AmvShellCode =
    L"var tmpWinKrkrExtract = new Window();"
    L"var mainlayer = new Layer(tmpWinKrkrExtract, null);"
    L"var drawlayer = new Layer(tmpWinKrkrExtract, mainlayer);"
    L"drawlayer.type = ltAddAlpha;"
    L"drawlayer.face = dfAddAlpha;"
    L"drawlayer.absolute = 100000;"
    L"drawlayer.visible = false;"
    L"var tmpLayer = new Layer(drawlayer.window, drawlayer);"
    L"tmpLayer.opacity = 255;"
    L"var amv = new AlphaMovie();"
    L"amv.open(%s);"
    L"amv.play();"
    L"tmpLayer.setPos(0, 0, amv.screenWidth, amv.screenHeight);"
    L"for (var LayerIndex = 0; LayerIndex < amv.numOfFrame; LayerIndex++)"
    L"{"
    L"	drawlayer.opacity = 255;"
    L"	amv.showNextImage(drawlayer);"
    L"	var saveFileName = %s + \"_%%d\".sprintf(LayerIndex) + \".png\";"
    L"	tmpLayer.hasImage = false;"
    L"	tmpLayer.hasImage = true;"
    L"	tmpLayer.copyRect(drawlayer.left, drawlayer.top, drawlayer, 0, 0, drawlayer.width, drawlayer.height);"
    L"	tmpLayer.visable = true;"
    L"	tmpLayer.saveLayerImagePng(saveFileName);"
    L"}";


NTSTATUS SaveAmv(LPCWSTR FileName, LPCWSTR Path)
{
	wstring ShellCode;

	ShellCode.reserve(0x1000);
	RtlZeroMemory(&ShellCode[0], 0x1000 * sizeof(WCHAR));
	FormatStringW(&ShellCode[0], g_AmvShellCode, FileName, Path);
	
	try {
		TVPExecuteScript(ttstr(ShellCode.c_str()));
	}
	catch (...) {
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}


