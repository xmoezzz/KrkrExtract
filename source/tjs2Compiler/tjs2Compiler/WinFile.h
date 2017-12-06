//Version 2016.01.28

#ifndef _WINFILE_
#define _WINFILE_

#include <Windows.h>

#define KERNEL32_SUCCESS(x) x == S_OK
#define USER32_SUCCESS(x)   x == TRUE

class WinFile
{
private:
	HANDLE hFile;
	ULONG  Access;
public:

	enum{ FileWrite = 1, FileRead = 2 };
	WinFile() : hFile((HANDLE)-1), Access(FileRead)
	{

	}
	~WinFile()
	{
		if (hFile != (HANDLE)-1)
		{
			CloseHandle(hFile);
		}
		hFile = (HANDLE)-1;
		Access = WinFile::FileRead;
	}

	VOID WINAPI Release()
	{
		this->~WinFile();
	}

	HRESULT WINAPI Open(const WCHAR* FileName, ULONG FileMethod)
	{
		if (hFile != (HANDLE)-1)
		{
			return S_FALSE;
		}
		if (FileMethod != WinFile::FileRead && FileMethod != WinFile::FileWrite)
		{
			return S_FALSE;
		}

		this->hFile = CreateFileW(FileName, FileMethod == WinFile::FileRead ? GENERIC_READ : GENERIC_WRITE,
			FileMethod == WinFile::FileRead ? FILE_SHARE_READ : FILE_SHARE_WRITE, NULL,
			FileMethod == WinFile::FileRead ? OPEN_EXISTING : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (this->hFile == (HANDLE)-1)
		{
			return S_FALSE;
		}
		this->Access = FileMethod;
		return S_OK;
	}

	HRESULT WINAPI Write(PBYTE Buffer, DWORD Size)
	{
		if (hFile != (HANDLE)-1)
		{
			if (Access != WinFile::FileWrite)
			{
				goto out;
			}
			DWORD nRet = 0;
			WriteFile(this->hFile, Buffer, Size, &nRet, NULL);
			return S_OK;
		}
	out:
		return S_FALSE;
	}

	HRESULT WINAPI Write(PBYTE Buffer, DWORD Size, DWORD& OutSize)
	{
		if (hFile != (HANDLE)-1)
		{
			if (Access != WinFile::FileWrite)
			{
				goto out;
			}
			OutSize = 0;
			WriteFile(this->hFile, Buffer, Size, &OutSize, NULL);
			return S_OK;
		}
	out:
		return S_FALSE;
	}

	HRESULT WINAPI Read(PBYTE Buffer, ULONG Size)
	{
		if (hFile != (HANDLE)-1)
		{
			DWORD nRet = 0;
			ReadFile(this->hFile, Buffer, Size, &nRet, NULL);
			return S_OK;
		}
		return S_FALSE;
	}

	HRESULT WINAPI Read(PBYTE Buffer, ULONG Size, DWORD& OutSize)
	{
		if (hFile != (HANDLE)-1)
		{
			OutSize = 0;
			ReadFile(this->hFile, Buffer, Size, &OutSize, NULL);
			return S_OK;
		}
		return S_FALSE;
	}

	HRESULT WINAPI Seek(LONG Where, DWORD Method)
	{
		if (hFile != (HANDLE)-1)
		{
			::SetFilePointer(this->hFile, Where, NULL, Method);
			return S_OK;
		}
		return S_FALSE;
	}

	HRESULT WINAPI Seek(LONG Where, DWORD Method, DWORD& OutPos)
	{
		if (hFile != (HANDLE)-1)
		{
			OutPos = ::SetFilePointer(this->hFile, Where, NULL, Method);
			return S_OK;
		}
		return S_FALSE;
	}

	ULONG32 WINAPI GetSize32()
	{
		if (hFile != (HANDLE)-1)
		{
			DWORD nRet = 0;
			return GetFileSize(this->hFile, &nRet);
		}
		return 0;
	}

	ULONG64 WINAPI GetSize64()
	{
		if (hFile != (HANDLE)-1)
		{
			LARGE_INTEGER Result;
			Result.QuadPart = (ULONG64)0;
			GetFileSizeEx(this->hFile, &Result);
			return Result.QuadPart;
		}
		return 0;
	}

	ULONG32 WINAPI GetCurrentPos32()
	{
		if (hFile != (HANDLE)-1)
		{
			return SetFilePointer(this->hFile, 0, 0, FILE_CURRENT);
		}
		return 0;
	}

	ULONG64 WINAPI GetCurrentPos64()
	{
		if (hFile != (HANDLE)-1)
		{
			LARGE_INTEGER InParam, Result;
			InParam.QuadPart = (ULONG64)0;
			Result.QuadPart = (ULONG64)0;
			SetFilePointerEx(this->hFile, InParam, &Result, FILE_CURRENT);
			return Result.QuadPart;
		}
		return 0;
	}
};


#endif
