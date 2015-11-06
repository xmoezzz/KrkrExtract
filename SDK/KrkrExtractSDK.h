#ifndef _KrkrExtractSDK_
#define _KrkrExtractSDK_

#include <windows.h>

//All dll plugins use this export function.
//Param in:
//[const WCHAR*] lpFileName: The full path of target resource file.
//[const WCHAR*] OutPath : The output, NULL or nullptr for default path.
//Param out:
//[int]0 for current dll plugin can extract the target file,
//     or the extraction succeed.

extern "C" __declspec(dllexport)
int WINAPI ExecExtract(const WCHAR* lpFileName, const WCHAR* OutPath);

#endif


//example:
//Just testing this plugin:
/*
ULONG Atom = 0xFFFFFFFF;
ExecExtract(L"11.psb", (WCHAR*)&Atom);
*/
//Do extraction:
/*
ExecExtract(L"11.psb", nullptr);
or
ExecExtract(L"11.psb", L"F:\psb");
*/