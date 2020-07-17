#include "GenMD5Code.h"

VOID WINAPI GenMD5Code(const WCHAR* FileName, wstring& OutHex)
{
	unsigned char OutBuffer[16] = { 0 };

	MD5Context Context;
	MD5Init(&Context);
	MD5Update(&Context, (const unsigned char*)FileName, wcslen(FileName) * 2);
	MD5Final(OutBuffer, &Context);

	WCHAR OutTemp[4] = { 0 };

	int iPos = 0;
	for (unsigned i = 0; i < 16; i++)
	{
		wsprintfW(OutTemp, L"%x", (OutBuffer[i] & 0xF0) >> 4);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t)* 4);
		wsprintfW(OutTemp, L"%x", OutBuffer[i] & 0x0F);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t)* 4);
	}
}

