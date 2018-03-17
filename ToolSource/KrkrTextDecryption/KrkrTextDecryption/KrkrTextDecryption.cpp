#include <Windows.h>
#include "WinFile.h"
#include <string>
#include "zlib\zlib.h"

#pragma comment(lib,"zlib.lib")

using std::wstring;


int wmain(int argc, wchar_t* argv[])
{
	if (argc != 2)
	{
		return -1;
	}
	WinFile File;
	if (File.Open(argv[1], WinFile::FileRead) != S_OK)
	{
		wstring Info(L"不能打开：\n");
		Info += argv[1];
		MessageBoxW(NULL, Info.c_str(), L"", MB_OK);
		return -1;
	}
	PBYTE Buffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, File.GetSize32());
	if (Buffer == nullptr)
	{
		wstring Info(L"分配内存失败：\n");
		Info += argv[1];
		MessageBoxW(NULL, Info.c_str(), L"", MB_OK);
		File.Release();
		return -1;
	}

	DWORD RawFileSize = File.GetSize32();
	File.Read(Buffer, File.GetSize32());
	File.Release();
	if (Buffer[0] == 0xfe && Buffer[1] == 0xfe)
	{
		UCHAR CryptMode = Buffer[2];
		if (CryptMode != 0 && CryptMode != 1 && CryptMode != 2)
		{
			HeapFree(GetProcessHeap(), 0, Buffer);
			return -1;
		}

		// compressed text stream
		if (CryptMode == 2)
		{	
			UINT64 compressed   = *(UINT64*)&Buffer[5];
			UINT64 uncompressed = *(UINT64*)&Buffer[13];

			if (compressed != (DWORD)compressed || uncompressed != (DWORD)uncompressed)
			{
				HeapFree(GetProcessHeap(), 0, Buffer);
				MessageBoxW(NULL, L"TextStream is too large to hold!", L"", MB_OK);
				return -1;
			}

			BYTE *DecompressBuffer = (BYTE *)HeapAlloc(GetProcessHeap(), 0, (DWORD)uncompressed);
			if (!Buffer)
			{
				HeapFree(GetProcessHeap(), 0, Buffer);
				MessageBoxW(NULL, L"不能为解压分配内存", L"", MB_OK);
				return -1;
			}

			unsigned long destlen = (unsigned long)uncompressed;
			int result = uncompress(DecompressBuffer, &destlen, Buffer + 21, (unsigned long)compressed);

			if (result != Z_OK || destlen != (unsigned long)uncompressed) 
			{
				HeapFree(GetProcessHeap(), 0, DecompressBuffer);
				HeapFree(GetProcessHeap(), 0, Buffer);
				MessageBoxW(NULL, L"解压失败", L"", MB_OK);
				return -1;
			}
			wstring OutFileName(argv[1]);
			OutFileName += L".out";

			WinFile OutputFile;
			if (OutputFile.Open(OutFileName.c_str(), WinFile::FileWrite) != S_OK)
			{
				MessageBoxW(NULL, L"写入文件失败", L"", MB_OK);
				HeapFree(GetProcessHeap(), 0, DecompressBuffer);
			}

			OutputFile.Write(DecompressBuffer, destlen);
			OutputFile.Release();
			HeapFree(GetProcessHeap(), 0, DecompressBuffer);
		}
		else if (CryptMode == 1)
		{
			DWORD act_len = (RawFileSize - 5) + 2;
			WCHAR *act;

			act = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, (DWORD)act_len);
			if (!act)
			{
				MessageBoxW(NULL, L"不能为解析分配内存", L"", MB_OK);
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

			wstring OutFileName(argv[1]);
			OutFileName += L".out";

			WinFile OutputFile;
			if (OutputFile.Open(OutFileName.c_str(), WinFile::FileWrite) != S_OK)
			{
				MessageBoxW(NULL, L"写入文件失败", L"", MB_OK);
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
				MessageBoxW(NULL, L"不能为解析分配内存", L"", MB_OK);
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

			wstring OutFileName(argv[1]);
			OutFileName += L".out";

			WinFile OutputFile;
			if (OutputFile.Open(OutFileName.c_str(), WinFile::FileWrite) != S_OK)
			{
				MessageBoxW(NULL, L"写入文件失败", L"", MB_OK);
				HeapFree(GetProcessHeap(), 0, act);
			}

			OutputFile.Write((PBYTE)act, act_len);
			OutputFile.Release();
			HeapFree(GetProcessHeap(), 0, act);
		}
	}

	HeapFree(GetProcessHeap(), 0, Buffer);
	return 0;
}

