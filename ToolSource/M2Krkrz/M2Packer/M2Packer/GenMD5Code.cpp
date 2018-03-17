#include "GenMD5Code.h"

VOID WINAPI GenMD5Code(const WCHAR* FileName, wstring& OutHex)
{
	BYTE OutBuffer[16] = { 0 };

	MD5Context Context;
	MD5Init(&Context);
	MD5Update(&Context, (BYTE*)FileName, lstrlenW(FileName) * 2);
	MD5Final(OutBuffer, &Context);

	WCHAR OutTemp[3] = { 0 };

	for (ULONG32 i = 0; i < 16; i++)
	{
		wsprintfW(OutTemp, L"%x", (OutBuffer[i] & 0xF0) >> 4);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, 0, sizeof(wchar_t)* 3);
		wsprintfW(OutTemp, L"%x", OutBuffer[i] & 0x0F);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, 0, sizeof(wchar_t)* 3);
	}
}
