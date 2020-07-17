#pragma comment(linker, "/SECTION:.text,ERW /MERGE:.rdata=.text /MERGE:.data=.text")
#pragma comment(linker, "/SECTION:.Xmoe,ERW /MERGE:.text=.Xmoe")

#include "HookLoadLibary.h"
#include "PluginHook.h"
#include "MiniLocale.h"
#include <Psapi.h>
#include "WinFile.h"

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Version.lib")

__declspec(dllexport)void WINAPI XmoeProc()
{
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{

	UNREFERENCED_PARAMETER(lpReserved);

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		if (GlobalData::GetGlobalData(hModule) == nullptr)
		{
			MessageBoxW(NULL, L"Internal exception!", L"FATAL", MB_OK);
			ExitProcess(-1);
		}
		
		GlobalData::GetGlobalData()->hImageModule = LoadLibraryW(L"ImageWorker.dll");
		DeleteFileW(L"KrkrExtractFailed.txt");

		InitHook();


		static WCHAR Pattern[] = L"TVP(KIRIKIRI) Z core / Scripting Platform for Win32";

		WCHAR FileName[MAX_PATH * 2] = { 0 };
		//TVP(KIRIKIRI) Z core / Scripting Platform for Win32
		//TVP(KIRIKIRI) 2 core / Scripting Platform for Win32
		GetModuleFileNameExW(GetCurrentProcess(), NULL, FileName, MAX_PATH * 2);

		WinFile File;
		if (File.Open(FileName, WinFile::FileRead) == S_OK)
		{
			DWORD RawFileSize = File.GetSize32();
			PBYTE RawBuffer = nullptr;
			RawBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, RawFileSize);
			if (RawBuffer)
			{
				File.Read(RawBuffer, RawFileSize);
				if (GlobalData::GetGlobalData()->FindCodeSlow((const PCHAR)RawBuffer, RawFileSize, (PCHAR)Pattern, lstrlenW(Pattern) * 2))
				{
					InitMiniLocale();
					//M2°æ±¾µÄKrkrz
					GlobalData::GetGlobalData()->ModuleType = ModuleVersion::Krkrz;
				}
				HeapFree(GetProcessHeap(), 0, RawBuffer);
			}
			File.Release();
		}
	}
	break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

