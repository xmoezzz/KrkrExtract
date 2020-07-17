#include "PluginHook.h"


FORCEINLINE wstring __fastcall GetPackageNameFastCall(wstring& fileName)
{
	wstring temp(fileName);
	wstring::size_type pos = temp.find_last_of(L"\\");

	if (pos != wstring::npos)
	{
		temp = temp.substr(pos + 1, temp.length());
	}

	wstring temp2(temp);
	wstring::size_type pos2 = temp2.find_last_of(L"\\");
	if (pos2 != wstring::npos)
	{
		temp2 = temp2.substr(pos + 1, temp2.length());
	}
	return temp2;
}


FORCEINLINE wstring __fastcall GetExtensionFastCall(const wchar_t* lpFile)
{
	wstring temp(lpFile);
	return temp.substr(temp.find_last_of(L".", temp.length()));
}

HRESULT WINAPI PluginHook::StartPacker()
{
	if (GlobalData::GetGlobalData()->isRunning)
	{
		MessageBoxW(NULL, L"Another task is under processing!", L"KrkrExtract", MB_OK);
		return S_FALSE;
	}
	GlobalData::GetGlobalData()->isRunning = TRUE;

	DeleteFileW(L"KrkrzTempWorker.xp3");

	wstring BasePath = GlobalData::GetGlobalData()->GetFolder();
	wstring OutFile = GlobalData::GetGlobalData()->GetOutputPack();
	wstring GuessPackage = GlobalData::GetGlobalData()->GetGuessPack();
	
	if (Packer)
	{
		Packer->Reset();
		delete Packer;
		Packer = nullptr;
	}
	Packer = new PackInfo(Gui, WinText.c_str());
	if (Packer == nullptr)
	{
		MessageBoxW(Gui, L"Couldn't Allocate Memory for Packing!", L"KrkrExtract", MB_OK);
		GlobalData::GetGlobalData()->isRunning = FALSE;
		return S_FALSE;
	}
	Packer->ForceInit(Gui, WinText.c_str());
	HRESULT Result = Packer->DoPack(BasePath.c_str(), GuessPackage.c_str(), OutFile.c_str());
	return Result;

}
