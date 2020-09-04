#pragma once

#include "my.h"
#include "ml.h"
#include "md5.h"
#include "tp_stub.h"

#define HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) :\
 ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)))


#define WALK_RESOURCE_BUFFERM(Buffer, Size) [&](PBYTE Buffer, ULONG Size)->NTSTATUS
#define WALK_RESOURCE_BUFFER_DUMMY WALK_RESOURCE_BUFFERM(Buffer, Size) { return STATUS_SUCCESS; }

template<class CallbackFunc>
inline IStream* LoadFromResource(HMODULE Module, UINT nResID, LPCWSTR lpTyp, CallbackFunc Callback)
{
	NTSTATUS         Status;
	WCHAR            ExeFileBaseName[MAX_PATH];
	HRSRC            hRsrc;
	PBYTE            Buffer, NewBuffer;
	ULONG            Size;
	IStream*         Stream;
	HGLOBAL          ResourceData;
	HGLOBAL          NewBufferHandle;

	hRsrc = FindResourceW(Module, MAKEINTRESOURCE(nResID), lpTyp);
	if (hRsrc == NULL)
		return NULL;

	ResourceData = LoadResource(Module, hRsrc);
	if (ResourceData == NULL)
		return NULL;

	Buffer = (PBYTE)LockResource(ResourceData);
	Size   = SizeofResource(Module, hRsrc);
	NewBufferHandle = GlobalAlloc(GHND, Size);
	NewBuffer       = (PBYTE)GlobalLock(NewBufferHandle);
	RtlCopyMemory(NewBuffer, Buffer, Size);

	Status = Callback(NewBuffer, Size);
	if (NT_FAILED(Status))
	{
		GlobalFree(NewBufferHandle);
		return NULL;
	}

	GlobalUnlock(NewBufferHandle);

	if (FAILED(CreateStreamOnHGlobal(NewBufferHandle, TRUE, &Stream)))
	{
		GlobalFree(NewBufferHandle);
		return NULL;
	}

	FreeResource(ResourceData);

	return Stream;
}


wstring GetPackageName(wstring FileName);
wstring GetDirName(wstring FileName);
wstring GetExtensionUpper(const wstring& FileName);


void GenMD5Code(PCWSTR FileName, wstring& OutHex);
void GenMD5Code(const BYTE* Buffer, const DWORD Size, wstring& OutHex);
void GenMD5Code(PCWSTR FileName, PCWSTR ProjectName, wstring& OutHex);
void DumpHex(const void* data, size_t size);
void FormatPath(wstring& Package, ttstr& out);

template<class ByteType>
inline BOOL SafeStringLength(const ByteType* StringPtr, const SIZE_T MaxLength, SIZE_T& StringLength)
{
	SIZE_T Size;

	StringLength = 0;
	if (!StringPtr || !MaxLength)
		return TRUE;

	Size = 0;
	while (Size < MaxLength)
	{
		if (Size == MaxLength)
		{
			StringLength = Size;
			return FALSE;
		}

		if (!StringPtr[Size])
			break;

		Size++;
	}
	StringLength = Size;
	return TRUE;
}

wstring GetFileName(wstring& Path);
wstring GetFileBasePath(wstring& Path);
wstring GetFileNameExtension(wstring& Path);
wstring GetFileNamePrefix(wstring& Path);
wstring GetUpperExtName(wstring& FileName);
wstring ToLowerString(LPCWSTR lpString);
wstring ReplaceFileNameExtension(wstring& Path, PCWSTR NewExtensionName);
wstring FormatPathFull(LPCWSTR Path);

ULONG64 MurmurHash64B(const void * key, int len, ULONG seed = 0xEE6B27EB);
NTSTATUS CopyExeIcon(PCWCHAR To, ULONG_PTR Check);

HRESULT NtStatusToHResult(NTSTATUS Status);

