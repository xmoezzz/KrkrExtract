#include "GenMD5Code.h"

Void WINAPI GenMD5Code(const WCHAR* FileName, wstring& OutHex)
{
	unsigned char OutBuffer[16];

	RtlZeroMemory(OutBuffer, sizeof(OutBuffer));

	MD5Context Context;
	MD5Init(&Context);
	MD5Update(&Context, (const unsigned char*)FileName, wcslen(FileName) * 2);
	MD5Final(OutBuffer, &Context);

	WCHAR OutTemp[4];
	RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);

	int iPos = 0;
	for (unsigned i = 0; i < 16; i++)
	{
		FormatStringW(OutTemp, L"%x", (OutBuffer[i] & 0xF0) >> 4);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t)* 4);
		FormatStringW(OutTemp, L"%x", OutBuffer[i] & 0x0F);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t)* 4);
	}
}

Void WINAPI GenMD5Code(const WCHAR* FileName, const WCHAR* ProjectName, wstring& OutHex)
{
	unsigned char OutBuffer[16];

	RtlZeroMemory(OutBuffer, sizeof(OutBuffer));

	MD5Context Context;
	MD5Init(&Context);
	MD5Update(&Context, (const unsigned char*)FileName, wcslen(FileName) * 2);
	MD5Update(&Context, (const unsigned char*)ProjectName, wcslen(ProjectName) * 2);
	MD5Final(OutBuffer, &Context);

	WCHAR OutTemp[4];
	RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);

	int iPos = 0;
	for (unsigned i = 0; i < 16; i++)
	{
		FormatStringW(OutTemp, L"%x", (OutBuffer[i] & 0xF0) >> 4);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
		FormatStringW(OutTemp, L"%x", OutBuffer[i] & 0x0F);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
	}
}

