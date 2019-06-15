#pragma once

#include "my.h"
#include "ml.h"


ForceInline wstring FASTCALL GetPackageName(wstring& FileName)
{
	auto Index = FileName.find_last_of(L'\\');

	if (Index != std::wstring::npos)
		FileName = FileName.substr(Index + 1, std::wstring::npos);

	Index = FileName.find_last_of(L'/');
	if (Index != std::wstring::npos)
		FileName = FileName.substr(Index + 1, std::wstring::npos);

	return FileName;
}

ForceInline wstring FASTCALL GetExtensionUpper(const wstring& FileName)
{
	auto Index = FileName.find_last_of(L'.');
	if (Index != std::wstring::npos)
	{
		wstring Result;
		for (auto& EachChar : FileName.substr(Index + 1))
		{
			Result += CHAR_UPPER(EachChar);
		}
		return Result;
	}

	return {};
}


#if 0

ForceInline Void FASTCALL GenMD5Code(LPCWSTR FileName, wstring& OutHex)
{
	BYTE OutBuffer[16];

	RtlZeroMemory(OutBuffer, sizeof(OutBuffer));

	TVP_md5_state_t Context;

	TVP_md5_init(&Context);
	TVP_md5_append(&Context, (LPCBYTE)FileName, StrLengthW(FileName) * 2);
	TVP_md5_finish(&Context, OutBuffer);

	WCHAR OutTemp[4];
	RtlZeroMemory(OutTemp, sizeof(WCHAR) * 4);

	int iPos = 0;
	for (ULONG i = 0; i < 16; i++)
	{
		FormatStringW(OutTemp, L"%x", (OutBuffer[i] & 0xF0) >> 4);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(WCHAR) * 4);
		FormatStringW(OutTemp, L"%x", OutBuffer[i] & 0x0F);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(WCHAR) * 4);
	}
}

ForceInline Void WINAPI GenMD5Code(LPCWSTR FileName, LPCWSTR ProjectName, wstring& OutHex)
{
	BYTE OutBuffer[16];

	RtlZeroMemory(OutBuffer, sizeof(OutBuffer));

	TVP_md5_state_t Context;

	TVP_md5_init(&Context);
	TVP_md5_append(&Context, (LPCBYTE)FileName, StrLengthW(FileName) * 2);
	TVP_md5_append(&Context, (LPCBYTE)ProjectName, StrLengthW(ProjectName) * 2);
	TVP_md5_finish(&Context, OutBuffer);

	WCHAR OutTemp[4];
	RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);

	int iPos = 0;
	for (ULONG i = 0; i < 16; i++)
	{
		FormatStringW(OutTemp, L"%x", (OutBuffer[i] & 0xF0) >> 4);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(WCHAR) * 4);
		FormatStringW(OutTemp, L"%x", OutBuffer[i] & 0x0F);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(WCHAR) * 4);
	}
}


#else
//[+] use 3rd lib

#include "md5.h"

ForceInline VOID WINAPI GenMD5Code(const WCHAR* FileName, wstring& OutHex)
{
	unsigned char OutBuffer[16];

	RtlZeroMemory(OutBuffer, sizeof(OutBuffer));

	MD5Context Context;
	MD5Init(&Context);
	MD5Update(&Context, (const BYTE*)FileName, wcslen(FileName) * 2);
	MD5Final(OutBuffer, &Context);

	WCHAR OutTemp[4];
	RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);

	int iPos = 0;
	for (unsigned i = 0; i < 16; i++)
	{
		wsprintfW(OutTemp, L"%x", (OutBuffer[i] & 0xF0) >> 4);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
		wsprintfW(OutTemp, L"%x", OutBuffer[i] & 0x0F);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
	}
}

ForceInline VOID WINAPI GenMD5Code(const WCHAR* FileName, const WCHAR* ProjectName, wstring& OutHex)
{
	unsigned char OutBuffer[16];

	RtlZeroMemory(OutBuffer, sizeof(OutBuffer));

	MD5Context Context;
	MD5Init(&Context);
	MD5Update(&Context, (const BYTE*)FileName, wcslen(FileName) * 2);
	MD5Update(&Context, (const BYTE*)ProjectName, wcslen(ProjectName) * 2);
	MD5Final(OutBuffer, &Context);

	WCHAR OutTemp[4];
	RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);

	int iPos = 0;
	for (unsigned i = 0; i < 16; i++)
	{
		wsprintfW(OutTemp, L"%x", (OutBuffer[i] & 0xF0) >> 4);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
		wsprintfW(OutTemp, L"%x", OutBuffer[i] & 0x0F);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
	}
}



#endif


ForceInline VOID FASTCALL FormatPath(wstring& Package, ttstr& out)
{
	out = L"file://./";
	for (SIZE_T iPos = 0; iPos < Package.length(); iPos++)
	{
		if (Package[iPos] == L':')
			continue;
		
		if (Package[iPos] == L'\\')
			out += L'/';
		else
			out += Package[iPos];
	}
	out += L'>';
}

ForceInline std::wstring FASTCALL GetFileName(std::wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L"\\");
	if (Ptr == std::wstring::npos)
		return Path;

	return Path.substr(Ptr + 1, std::wstring::npos);
}


ForceInline std::wstring FASTCALL GetFileBasePath(std::wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L"\\");
	if (Ptr == std::wstring::npos)
		return Path;

	return Path.substr(0, Ptr);
}




ForceInline std::wstring FASTCALL GetFileNameExtension(std::wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == std::wstring::npos)
		return {};

	return Path.substr(Ptr + 1, std::wstring::npos);
}


ForceInline std::wstring FASTCALL GetFileNamePrefix(std::wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == std::wstring::npos)
		return Path;

	return Path.substr(0, Ptr);
}


ForceInline std::wstring FASTCALL ReplaceFileNameExtension(std::wstring& Path, PCWSTR NewExtensionName)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == std::wstring::npos)
		return Path + NewExtensionName;

	return Path.substr(0, Ptr) + NewExtensionName;
}

