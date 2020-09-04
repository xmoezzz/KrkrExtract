#include "Helper.h"
#include "SafeMemory.h"

wstring GetPackageName(wstring FileName)
{
	auto Index = FileName.find_last_of(L'\\');

	if (Index != std::wstring::npos)
		FileName = FileName.substr(Index + 1, std::wstring::npos);

	Index = FileName.find_last_of(L'/');
	if (Index != std::wstring::npos)
		FileName = FileName.substr(Index + 1, std::wstring::npos);

	return FileName;
}

wstring GetDirName(wstring FileName)
{
	for (auto& NameChar : FileName) 
	{
		if (NameChar == L'/') {
			NameChar = L'\\';
		}
	}

	auto Index = FileName.find_last_of(L'\\');
	if (Index != wstring::npos) {
		return FileName.substr(0, Index);
	}

	return FileName;
}


wstring GetUpperExtName(wstring& FileName)
{
	auto Index = FileName.find_last_of(L'.');
	if (Index != std::wstring::npos)
	{
		wstring Result;
		for (auto& EachChar : FileName.substr(Index + 1, std::wstring::npos))
		{
			Result += CHAR_UPPER(EachChar);
		}
		return Result;
	}

	return {};
}

wstring GetExtensionUpper(const wstring& FileName)
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

wstring ToLowerString(LPCWSTR lpString)
{
	wstring Result;

	for (LONG_PTR i = 0; i < lstrlenW(lpString); i++)
		Result += (WCHAR)CHAR_LOWER(lpString[i]);

	return Result;
}


wstring FormatPathFull(LPCWSTR Path)
{
	WCHAR  Buffer[MAX_PATH];

	if (RtlCompareMemory(Path, L"file:", 10) == 10)
		return Path;

	else if (Path[1] == L':' && (Path[0] <= L'Z' && Path[0] >= L'A' || Path[0] <= L'z' && Path[0] >= L'a'))
	{
		wstring Result(L"file://./");
		Result += Path[0];
		for (ULONG i = 2; Path[i] != 0; ++i)
		{
			if (Path[i] == L'\\')
				Result += L'/';
			else
				Result += Path[i];
		}
		return Result;
	}
	else
	{
		BOOL Flag = FALSE;
		for (ULONG i = 0; Path[i] != 0; ++i)
		{
			if (Path[i] == '/' || Path[i] == '\\' || Path[i] == '*')
			{
				Flag = TRUE;
				break;
			}
		}
		if (!Flag)
		{
			RtlZeroMemory(Buffer, countof(Buffer) * sizeof(WCHAR));
			GetCurrentDirectoryW(countof(Buffer), Buffer);
			wsprintfW(Buffer + lstrlenW(Buffer), L"/%s", Path);
			auto&& Result = FormatPathFull(Buffer);
			return Result;
		}
	}
	return {};
}


void GenMD5Code(PCWSTR FileName, wstring& OutHex)
{
	BYTE OutBuffer[16];

	RtlZeroMemory(OutBuffer, sizeof(OutBuffer));

	MD5Context Context;
	MD5Init(&Context);
	MD5Update(&Context, (const BYTE*)FileName, lstrlenW(FileName) * 2);
	MD5Final(OutBuffer, &Context);

	WCHAR OutTemp[4];
	RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);

	for (ULONG i = 0; i < 16; i++)
	{
		wsprintfW(OutTemp, L"%x", (OutBuffer[i] & 0xF0) >> 4);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
		wsprintfW(OutTemp, L"%x", OutBuffer[i] & 0x0F);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
	}
}


void GenMD5Code(const BYTE* Buffer, const DWORD Size, wstring& OutHex)
{
	BYTE OutBuffer[16];

	RtlZeroMemory(OutBuffer, sizeof(OutBuffer));

	MD5Context Context;
	MD5Init(&Context);
	MD5Update(&Context, Buffer, Size);
	MD5Final(OutBuffer, &Context);

	WCHAR OutTemp[4];
	RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);

	for (ULONG i = 0; i < 16; i++)
	{
		wsprintfW(OutTemp, L"%x", (OutBuffer[i] & 0xF0) >> 4);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
		wsprintfW(OutTemp, L"%x", OutBuffer[i] & 0x0F);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
	}
}


void GenMD5Code(PCWSTR FileName, PCWSTR ProjectName, wstring& OutHex)
{
	BYTE OutBuffer[16];

	RtlZeroMemory(OutBuffer, sizeof(OutBuffer));

	MD5Context Context;
	MD5Init(&Context);
	MD5Update(&Context, (const BYTE*)FileName, lstrlenW(FileName) * 2);
	MD5Update(&Context, (const BYTE*)ProjectName, lstrlenW(ProjectName) * 2);
	MD5Final(OutBuffer, &Context);

	WCHAR OutTemp[4];
	RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);

	for (ULONG i = 0; i < 16; i++)
	{
		wsprintfW(OutTemp, L"%x", (OutBuffer[i] & 0xF0) >> 4);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
		wsprintfW(OutTemp, L"%x", OutBuffer[i] & 0x0F);
		OutHex += OutTemp;
		RtlZeroMemory(OutTemp, sizeof(wchar_t) * 4);
	}
}


void FormatPath(wstring& Package, ttstr& out)
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

wstring GetFileName(wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L"\\");
	if (Ptr == wstring::npos)
		return Path;

	return Path.substr(Ptr + 1, wstring::npos);
}


wstring GetFileBasePath(wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L"\\");
	if (Ptr == wstring::npos)
		return Path;

	return Path.substr(0, Ptr);
}


wstring GetFileNameExtension(wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == wstring::npos)
		return {};

	return Path.substr(Ptr + 1, std::wstring::npos);
}

wstring GetFileNamePrefix(wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == wstring::npos)
		return Path;

	return Path.substr(0, Ptr);
}

wstring ReplaceFileNameExtension(wstring& Path, PCWSTR NewExtensionName)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == wstring::npos)
		return Path + NewExtensionName;

	return Path.substr(0, Ptr) + NewExtensionName;
}


NTSTATUS CopyExeIcon(PCWCHAR To, ULONG_PTR Check)
{
	HMODULE    hExe;
	HRSRC      hRes;
	HANDLE     hUpdateRes;

	hExe = GetModuleHandleW(NULL);
	hUpdateRes = BeginUpdateResourceW(To, FALSE);
	if (hUpdateRes == NULL)
		return STATUS_UNSUCCESSFUL;

	for (ULONG_PTR i = 0; i < Check; i++)
	{
		hRes = FindResource(hExe, MAKEINTRESOURCEW(i), RT_GROUP_ICON);
		if (hRes != NULL)
		{
			if (!UpdateResourceW(hUpdateRes, RT_GROUP_ICON, MAKEINTRESOURCEW(i), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), LockResource(LoadResource(hExe, hRes)), SizeofResource(hExe, hRes))
				)
				break;
		}
	}
	hRes = NULL;


	for (ULONG_PTR i = 0; i < Check; i++)
	{
		if (FindResource(hExe, MAKEINTRESOURCEW(i), RT_ICON) != NULL)
		{
			i--;
			for (ULONG_PTR rCount = i; rCount < (i + 32); rCount++)
			{
				hRes = FindResource(hExe, MAKEINTRESOURCEW(rCount), RT_ICON);
				if (hRes != NULL)
				{
					UpdateResourceW(hUpdateRes, RT_ICON, MAKEINTRESOURCEW(rCount), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), LockResource(LoadResource(hExe, hRes)), SizeofResource(hExe, hRes));
				}
			}
			break;
		}

	}
	hRes = NULL;

	for (ULONG_PTR i = 0; i < Check; i++) 
	{
		hRes = FindResource(hExe, MAKEINTRESOURCEW(i), RT_VERSION);
		if (hRes != NULL)
		{
			if (!UpdateResourceW(hUpdateRes, RT_VERSION, MAKEINTRESOURCEW(i), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), LockResource(LoadResource(hExe, hRes)), SizeofResource(hExe, hRes))
				)
				break;
		}
	}

	EndUpdateResourceW(hUpdateRes, FALSE);
	return STATUS_SUCCESS;
}


ULONG64 MurmurHash64B(const void * key, int len, ULONG seed)
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	unsigned int h1 = seed ^ len;
	unsigned int h2 = 0;

	const unsigned int * data = (const unsigned int *)key;

	while (len >= 8)
	{
		unsigned int k1 = *data++;
		k1 *= m; k1 ^= k1 >> r; k1 *= m;
		h1 *= m; h1 ^= k1;
		len -= 4;

		unsigned int k2 = *data++;
		k2 *= m; k2 ^= k2 >> r; k2 *= m;
		h2 *= m; h2 ^= k2;
		len -= 4;
	}

	if (len >= 4)
	{
		unsigned int k1 = *data++;
		k1 *= m; k1 ^= k1 >> r; k1 *= m;
		h1 *= m; h1 ^= k1;
		len -= 4;
	}

	switch (len)
	{
	case 3: h2 ^= ((unsigned char*)data)[2] << 16;
	case 2: h2 ^= ((unsigned char*)data)[1] << 8;
	case 1: h2 ^= ((unsigned char*)data)[0];
		h2 *= m;
	};

	h1 ^= h2 >> 18; h1 *= m;
	h2 ^= h1 >> 22; h2 *= m;
	h1 ^= h2 >> 17; h1 *= m;
	h2 ^= h1 >> 19; h2 *= m;

	unsigned long long h = h1;

	h = (h << 32) | h2;

	return h;
}


HRESULT NtStatusToHResult(NTSTATUS Status)
{
	return HRESULT_FROM_WIN32(RtlNtStatusToDosError(Status));
}



// @keenfuzz
void DumpHex(const void* data, size_t size)
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		PrintConsoleA("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		}
		else {
			ascii[i % 16] = '.';
		}
		if ((i + 1) % 8 == 0 || i + 1 == size) 
		{
			PrintConsoleA(" ");
			if ((i + 1) % 16 == 0) {
				PrintConsoleA("|  %s \n", ascii);
			}
			else if (i + 1 == size) {
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8) {
					PrintConsoleA(" ");
				}
				for (j = (i + 1) % 16; j < 16; ++j) {
					PrintConsoleA("   ");
				}
				PrintConsoleA("|  %s \n", ascii);
			}
		}
	}
}

