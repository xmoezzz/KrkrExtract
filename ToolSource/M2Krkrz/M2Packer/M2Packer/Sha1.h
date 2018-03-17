#ifndef _Sha1_Xmoe_
#define _Sha1_Xmoe_

#include <Windows.h>
#include <string>

using std::wstring;

int WINAPI GenSha1Code(WCHAR* pStr, wstring& Result);
int WINAPI GenSha1CodeConst(const WCHAR* pCStr, wstring& Result);

#endif
