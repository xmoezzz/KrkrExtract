#include "KrkrTextDecode.h"
#include "zlib.h"
#include "WinFile.h"

/*
Check if the given unsigned char * is a valid utf-8 sequence.

Return value :
If the string is valid utf-8, 0 is returned.
Else the position, starting from 1, is returned.

Valid utf-8 sequences look like this :
0xxxxxxx
110xxxxx 10xxxxxx
1110xxxx 10xxxxxx 10xxxxxx
11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
*/

size_t IsUtf8String(unsigned char *str, size_t len)
{
	size_t i = 0;
	size_t continuation_bytes = 0;

	while (i < len)
	{
		if (str[i] <= 0x7F)
			continuation_bytes = 0;
		else if (str[i] >= 0xC0 /*11000000*/ && str[i] <= 0xDF /*11011111*/)
			continuation_bytes = 1;
		else if (str[i] >= 0xE0 /*11100000*/ && str[i] <= 0xEF /*11101111*/)
			continuation_bytes = 2;
		else if (str[i] >= 0xF0 /*11110000*/ && str[i] <= 0xF4 /* Cause of RFC 3629 */)
			continuation_bytes = 3;
		else
			return i + 1;
		i += 1;
		while (i < len && continuation_bytes > 0
			&& str[i] >= 0x80
			&& str[i] <= 0xBF)
		{
			i += 1;
			continuation_bytes -= 1;
		}
		if (continuation_bytes != 0)
			return i + 1;
	}
	return 0;
}

int WINAPI DecodeText(PBYTE Buffer, ULONG RawFileSize, HWND hWnd, const WCHAR* lpOutFileName)
{
	if (Buffer[0] == 0xfe && Buffer[1] == 0xfe)
	{
		UCHAR CryptMode = Buffer[2];
		if (CryptMode != 0 && CryptMode != 1 && CryptMode != 2)
		{
			MessageBoxW(hWnd, L"Text:Invalid Mode", L"KrkrExtract", MB_OK);
			return -1;
		}

		// compressed text stream
		if (CryptMode == 2)
		{
			UINT64 compressed = *(UINT64*)&Buffer[5];
			UINT64 uncompressed = *(UINT64*)&Buffer[13];

			if (compressed != (DWORD)compressed || uncompressed != (DWORD)uncompressed)
			{
				MessageBoxW(hWnd, L"TextStream is too large to hold!", L"", MB_OK);
				return -1;
			}

			BYTE *DecompressBuffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, (DWORD)uncompressed);
			if (!DecompressBuffer)
			{
				MessageBoxW(hWnd, L"Text: Cannot Allocate memory for uncompression", 
					L"KrkrExtract", MB_OK);
				return -1;
			}

			unsigned long destlen = (unsigned long)uncompressed;
			int result = uncompress(DecompressBuffer, &destlen, Buffer + 21, (unsigned long)compressed);

			if (result != Z_OK || destlen != (unsigned long)uncompressed)
			{
				HeapFree(GetProcessHeap(), 0, DecompressBuffer);
				MessageBoxW(hWnd, L"Text:Failed to uncompress", L"KrkrExtract", MB_OK);
				return -1;
			}

			WinFile OutputFile;
			if (OutputFile.Open(lpOutFileName, WinFile::FileWrite) != S_OK)
			{
				MessageBoxW(hWnd, L"Text:Failed to write file", L"KrkrExtract", MB_OK);
				HeapFree(GetProcessHeap(), 0, DecompressBuffer);
			}

			if (IsUtf8String(DecompressBuffer, destlen)!=destlen)
			{
				WORD Bom = 0xfeff;
				OutputFile.Write((PBYTE)&Bom, 2);
				OutputFile.Write(DecompressBuffer, destlen);
				OutputFile.Release();
			}
			else
			{
				OutputFile.Write(DecompressBuffer, destlen);
				OutputFile.Release();
			}
			HeapFree(GetProcessHeap(), 0, DecompressBuffer);
		}
		else if (CryptMode == 1)
		{
			DWORD act_len = (RawFileSize - 5) + 2;
			WCHAR *act;

			act = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, (DWORD)act_len);
			if (!act)
			{
				MessageBoxW(hWnd, L"Text:Cannot Allocate memory for decoding", L"KrkrExtract", MB_OK);
				HeapFree(GetProcessHeap(), 0, Buffer);
				return -1;
			}
			act[0] = 0xfeff;

			WCHAR *buf = (WCHAR *)&Buffer[5];
			for (DWORD i = 0; i < (RawFileSize - 5) / 2; i++)
			{
				WCHAR ch = buf[i];
				ch = ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
				act[i + 1] = ch;
			}

			WinFile OutputFile;
			if (OutputFile.Open(lpOutFileName, WinFile::FileWrite) != S_OK)
			{
				MessageBoxW(hWnd, L"Text:Failed to write file", L"KrkrExtract", MB_OK);
				HeapFree(GetProcessHeap(), 0, act);
			}

			OutputFile.Write((PBYTE)act, act_len);
			OutputFile.Release();
			HeapFree(GetProcessHeap(), 0, act);

		}
		else if (CryptMode == 0)
		{
			DWORD act_len = (RawFileSize - 5) + 2;
			WCHAR *act = nullptr;

			act = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, (DWORD)act_len);
			if (!act)
			{
				MessageBoxW(hWnd, L"Text:Cannot Allocate memory for decoding", 
					L"KrkrExtract", MB_OK);
				HeapFree(GetProcessHeap(), 0, Buffer);
				return -1;
			}
			act[0] = 0xfeff;

			WCHAR *buf = (WCHAR *)&Buffer[5];
			for (DWORD i = 0; i < RawFileSize - 5; i++)
			{
				WCHAR ch = buf[i];
				if (ch >= 0x20)
				{
					act[i + 1] = ch ^ (((ch & 0xfe) << 8) ^ 1);
				}
			}

			WinFile OutputFile;
			if (OutputFile.Open(lpOutFileName, WinFile::FileWrite) != S_OK)
			{
				MessageBoxW(hWnd, L"Text:Failed to write file", L"KrkrExtract", MB_OK);
				HeapFree(GetProcessHeap(), 0, act);
			}

			OutputFile.Write((PBYTE)act, act_len);
			OutputFile.Release();
			HeapFree(GetProcessHeap(), 0, act);
		}
	}
	return 0;
}

