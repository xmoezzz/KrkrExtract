#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <memory.h>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <assert.h>
#include <windows.h>
#include <zlib.h>
#include <Psapi.h>
#include <ntstatus.h>
#include "PsbCommon.h"
#include <my.h>


#include "lz4.h"
#include "lz4frame.h"

#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "psapi.lib")

#pragma comment(lib, "MyLibrary_x86_static.lib")
#pragma comment(lib, "jsoncpp.lib")

#define TJS_W _T
#define LZ4_MAGIC 0x184D2204


#define LOOP_ONCE for(ULONG_PTR LoopVar = 0; LoopVar < 1; LoopVar++)
#define LOOP_FOREVER while(1)

void TVPThrowExceptionMessage(_TCHAR* info)
{
	_putts(info);
	getchar();
	exit(-2);
}

enum ePsbValueType
{
	PsbValueVoid = 0,
	PsbValueBoolean = 1,
	PsbValueInteger = 2,
	PsbValueFloat = 3,
	PsbValueString = 4,
	PsbValueBinary = 5,
	PsbValueArray = 6,
	PsbValueDictionay = 7,
};

static unsigned char *getDataFromMDF(const unsigned char *buff, unsigned long &size)
{
	if (size <= 10 || memcmp(buff, "mdf", 4)) return NULL;

	unsigned long uncompsize = *(unsigned long*)&buff[4];
	unsigned char *uncomp = new unsigned char[uncompsize];
	if (Z_OK != uncompress(uncomp, &uncompsize, buff + 8, size - 8))
	{
		delete[] uncomp;
		return NULL;
	}
	size = uncompsize;
	return uncomp;
}


static unsigned char *getDataFromLz4(const unsigned char *buff, unsigned long &size)
{
	if (size <= 10 || *(PDWORD)buff != LZ4_MAGIC) return NULL;

	unsigned long uncompsize = *(unsigned long*)&buff[4];

	LZ4F_decompressionContext_t context;
	LZ4F_frameInfo_t frameInfo;
	LZ4F_errorCode_t err;
	size_t srcSize = size;
	unsigned char *uncompr_data;
	size_t pos, dstPos;
	size_t dstSize;

	err = LZ4F_createDecompressionContext(&context, LZ4F_VERSION);
	if (LZ4F_isError(err))
	{
		return NULL;
	}

	err = LZ4F_getFrameInfo(context, &frameInfo, buff, &srcSize);
	if (LZ4F_isError(err))
	{
		return NULL;
	}

	pos = srcSize;
	dstPos = 0;
	uncompr_data = new unsigned char[frameInfo.contentSize];

	do
	{
		dstSize = frameInfo.contentSize - dstPos;
		srcSize = size - pos;

		err = LZ4F_decompress(context, &uncompr_data[dstPos], &dstSize, &buff[pos], &srcSize, NULL);

		if (LZ4F_isError(err))
		{
			delete[] uncompr_data;
			return nullptr;
		}

		dstPos += dstSize;
		pos += srcSize;
	} while (err);

	size = frameInfo.contentSize;
	return uncompr_data;
}

static ePsbValueType GetValueTypeByID(int nID)
{
	switch (nID)
	{
	case 1u: //null
	case 35u:
	case 36u:
	case 37u:
	case 38u:
	case 63u:
		return PsbValueVoid;

	case 2u: //true
	case 3u: //false
	case 39u:
	case 47u:
	case 51u:
	case 55u:
	case 59u:
		return PsbValueBoolean;

	case 4u:
	case 5u:
	case 6u:
	case 7u:
	case 8u:
		//Int
	case 40u:
	case 48u:
	case 52u:
	case 56u:
	case 60u:

	case 9u:
	case 0xAu:
	case 0xBu:
	case 0xCu:

	case 41u:
	case 49u:
	case 53u:
	case 57u:
	case 61u:
		return PsbValueInteger;

	case 0x1Du:
	case 0x1Eu:
	case 0x1Fu: //Double

	case 46u:
	case 65u:
		return PsbValueFloat;

	case 0x15u:
	case 0x16u:
	case 0x17u:
	case 0x18u:

	case 44u:
		return PsbValueString;

	case 0x19u:
	case 0x1Au:
	case 0x1Bu:
	case 0x1Cu:

	case 45u:
		return PsbValueBinary;

	case 0x20u:
		return PsbValueArray;

	case 0x21u:
		return PsbValueDictionay;

	default:
		printf("%d\n", nID);
		TVPThrowExceptionMessage(TJS_W("psb: internal error: unknown internal type detected.\n"));
		return (ePsbValueType)-1;
	}
}

static unsigned int PsbGetCount(const unsigned char *code)
{
	switch (*code++)
	{
	case 13:
		return *code;
	case 14:
		return *(unsigned short*)code;
	case 15:
		return (*(unsigned long*)code) & 0xFFFFFF;
	case 16:
		return *(unsigned long*)code;
	default:
		return 0;
	}
}

struct PsbArray
{
	ULONG  nBytes;
	ULONG  nElementCount;
	ULONG  nSizeOf;
	LPBYTE pCode;
	PsbArray(LPBYTE code)
	{
		int l = *code - 11;
		nElementCount = PsbGetCount(code);
		nSizeOf = code[l] - 12;
		pCode = code + l + 1;
		nBytes = nElementCount * nSizeOf + l + 1;
	}
	unsigned long operator [](unsigned int idx)
	{
		if (nSizeOf - 1 < 5)
		{
			return (*(PULONG_PTR)(pCode + idx * nSizeOf))
				& (0xFFFFFFFF >> ((4 - nSizeOf) * 8));
		}
		return 0;
	}
};

LONG PsbGetLong(LPBYTE code)
{
	switch (*code++)
	{
	case 5:
		return *(PCHAR)code;

	case 6:
		return *(PSHORT)code;

	case 7:
		return *(PSHORT)code | (*(PCHAR)(code + 2) << 16);

	case 8:
		return *(PINT)code;

	default:
		return 0;
	}
}

static LONG64 PsbGetLongLong(LPBYTE code)
{
	switch (*code++)
	{
	case 9:  return (LONG64)*(PULONG)code | (((LONG64)*(PCHAR)(code + 4)) << 32);
	case 10: return (LONG64)*(PULONG)code | (((LONG64)*(PSHORT)(code + 4)) << 32);
	case 11: return (LONG64)*(PULONG)code | (((LONG64)*(PSHORT)(code + 4)) << 32)
		| (((LONG64)*(PCHAR)(code + 6)) << 48);
	case 12: return *(PLONG64)code;
	default:
		return 0;
	}
}

static float PsbGetFloat(const unsigned char *code)
{
	switch (*code++)
	{
	case 29:
		return 0.0f;
	case 30:
	{
		float ret; memcpy((void*)&ret, (const void*)code, sizeof(ret));
		return ret;
	}
	default:
		return 0;
	}
}

static double PsbGetDouble(const unsigned char *code)
{
	switch (*code++)
	{
	case 31:
	{
		double ret; memcpy((void*)&ret, (const void*)code, sizeof(ret));
		return ret;
	}
	default:
		return 0;
	}
}

static double PsbGetDoubleValue(LPBYTE code)
{
	switch (*code)
	{
	case 1: return NULL;
	case 2: return 1;
	case 3: return 0;
	case 4: case 5: case 6: case 7: case 8:
		return PsbGetLong(code);
	case 9: case 10: case 11: case 12:
		return PsbGetLongLong(code);
	case 29: case 30:
		return PsbGetFloat(code);
	case 31:
		return PsbGetDouble(code);
	default:
		TVPThrowExceptionMessage(TJS_W("psb: can't convert value to double."));
		return 0;
	}
}

LONG64 PsbGetIntegerValue(LPBYTE code)
{
	switch (*code)
	{
	case 1: return NULL;
	case 2: return 1;
	case 3: return 0;
	case 4: case 5: case 6: case 7: case 8:
		return PsbGetLong(code);
	case 9: case 10: case 11: case 12:
		return PsbGetLongLong(code);
	case 29: case 30:
		return PsbGetFloat(code);
	case 31:
		return PsbGetDouble(code);
	default:
		TVPThrowExceptionMessage(TJS_W("psb: can't convert value to long int."));
		return 0;
	}
}

LONG PsbGetIntValue(LPBYTE code)
{
	switch (*code)
	{
	case 1: return NULL;
	case 2: return 1;
	case 3: return 0;
	case 4: case 5: case 6: case 7: case 8:
		return PsbGetLong(code);
	case 9: case 10: case 11: case 12:
		return PsbGetLongLong(code);
	case 29: case 30:
		return PsbGetFloat(code);
	case 31:
		return PsbGetDouble(code);
	default:
		TVPThrowExceptionMessage(TJS_W("psb: can't convert value to int."));
		return 0;
	}
}

BOOLEAN PsbGetBooleanValue(LPBYTE code)
{
	switch (*code)
	{
	case 1: return NULL;
	case 2: return true;
	case 3: return false;
	case 4: case 5: case 6: case 7: case 8:
		return PsbGetLong(code);
	case 9: case 10: case 11: case 12:
		return PsbGetLongLong(code);
	case 29: case 30:
		return PsbGetFloat(code);
	case 31:
		return PsbGetDouble(code);
	default:
		TVPThrowExceptionMessage(TJS_W("psb: can't convert value to bool."));
		return false;
	}
}

BOOLEAN PsbFindStringDict(LPBYTE pDicCode, LPBYTE pKey, unsigned int &val)
{
	PsbArray arr1(pDicCode); // prefix tree
	PsbArray arr2(pDicCode + arr1.nBytes);
	ULONG idx = arr1[0] + *pKey, lastIdx = 0;
	while (idx < arr1.nElementCount && arr2[idx] == lastIdx)
	{
		if (!*pKey)
		{
			val = arr1[idx];
			return true;
		}
		lastIdx = idx;
		idx = *++pKey + arr1[idx];
	}
	return false;
}

BOOLEAN PsbFindBinaryDict(LPBYTE pDicCode, ULONG key, unsigned int &offset)
{
	PsbArray arr_key(pDicCode); // sorted
	if (!arr_key.nElementCount)
		return false;

	// binary search
	unsigned int l = 0, r = arr_key.nElementCount, m;
	while (true)
	{
		m = (l + r) / 2;
		unsigned int n = arr_key[m];
		if (n == key) break;
		if (n >= key) r = m;
		else         l = m + 1;
		if (l >= r) return false;
	}

	if (l < r)
	{
		PsbArray arr_val(pDicCode + arr_key.nBytes);
		// actual data stored at code + offset
		offset = arr_val[m] + arr_key.nBytes + arr_val.nBytes;
		return true;
	}
	return false;
}

ULONG PsbGetStringIndex(LPBYTE code)
{
	switch (*code++)
	{
	case 21:
		return *code;
	case 22:
		return *(unsigned short*)code;
	case 23:
		return (*(unsigned long*)code) & 0xFFFFFF;
	case 24:
		return *(unsigned long*)code;
	default:
		return 0;
	}
}

ULONG PsbGetBinaryIndex(LPBYTE code)
{
	switch (*code++)
	{
	case 25:
		return *code;
	case 26:
		return *(PSHORT)code;
	case 27:
		return (*(PULONG)code) & 0xFFFFFF;
	case 28:
		return *(PULONG)code;
	default:
		return 0;
	}
}


/*
0.Offical SDK
Dump - emotedriver:.rdata
Address   Hex dump                                         ASCII
633C2350  31 39 32 39 | 31 38 38 35 | 34                       192918854
*/
static PCHAR ReferKey[] = { { "192918854" } };

struct PSBObject
{
public:
	PSB_HEADER_V2* pInfo;
	LPBYTE         Data;
	ULONG          DataLength;

	PSBObject(LPBYTE data, ULONG len)
		: Data(data),
		DataLength(len)
	{
		pInfo = (PSB_HEADER_V2*)Data;

		if (pInfo->Version < 2)
		{
			TVPThrowExceptionMessage(TJS_W("psb: unsupport version!"));
		}

		m_key[0] = 0x075BCD15;
		m_key[1] = 0x159A55E5;
		m_key[2] = 0x1F123BB5;
		m_key[3] = atoi(ReferKey[0]);
		m_key[4] = 0;
		m_key[5] = 0;

		m_key[3] = GetPrivateKey();

		if (pInfo->Flag & 1 && pInfo->Version == 3)
			Decrypt(data + 0x8, 0, pInfo->Version == 3 ? 0x24 : 0x20);

		//PSB V2 will not mark the field 
		//root is a directory?
		if (pInfo->Flag & 2 || data[(ULONG_PTR)pInfo->pRootCode] != 0x21)
		{
			Decrypt(data + (ULONG_PTR)pInfo->pStringIndex, 0,
				(ULONG_PTR)pInfo->pBinOffsetArray - (ULONG_PTR)pInfo->pStringIndex);

			if (data[(ULONG_PTR)pInfo->pRootCode] != 0x21)
			{
				MessageBoxW(NULL, L"Unable to get private for this emote file...\n", 0, 0);
				::ExitProcess(0);
			}
		}

		pInfo->pHeader += (ULONG_PTR)Data;
		pInfo->pStringIndex += (ULONG_PTR)Data;
		pInfo->pStringOffsetArray += (ULONG_PTR)Data;
		pInfo->pStringPool += (ULONG_PTR)Data;
		pInfo->pBinOffsetArray += (ULONG_PTR)Data;
		pInfo->pBinSizeArray += (ULONG_PTR)Data;
		pInfo->pBinaryPool += (ULONG_PTR)Data;
		pInfo->pRootCode += (ULONG_PTR)Data;
	}

	ULONG  GetPrivateKey()
	{
		ULONG    Key;
		if (FindEmoteKeyByParse(&Key) >= 0 || FindEmoteKeyByMark(&Key) >= 0 || FindEmoteKeyByEmotePlayer(&Key) >= 0)
		{
		}
		else
		{
			Key = 0;
		}
		return Key;
	}


	NTSTATUS NTAPI FindEmoteKeyByMark(PULONG PrivateKey)
	{
		HMODULE      hModule;
		NTSTATUS     Status;
		ULONG64      Magic[4];
		MODULEINFO   ModudleInfo;
		ULONG_PTR    iPos;
		ULONG        AddressOfPrivate;

		LOOP_ONCE
		{
			Status = STATUS_UNSUCCESSFUL;

			hModule = GetModuleHandleW(L"emotedriver.dll");

			if (!hModule)
				hModule = LoadLibraryW(L"emotedriver.dll");

			if (!hModule)
				hModule = LoadLibraryW(L"plugin\\emotedriver.dll");

			if (!hModule)
				break;

			GetModuleInformation(GetCurrentProcess(), hModule, &ModudleInfo, NULL);

			iPos = 0;
			LOOP_FOREVER
			{
				if (iPos >= ModudleInfo.SizeOfImage)
				break;

				Magic[0] = *(PULONG64)((ULONG_PTR)hModule + iPos + 0);
				Magic[1] = *(PULONG64)((ULONG_PTR)hModule + iPos + 8);
				Magic[2] = *(PULONG64)((ULONG_PTR)hModule + iPos + 16);
				Magic[3] = *(PULONG64)((ULONG_PTR)hModule + iPos + 24);


				if (Magic[0] == 0x00004E414E512331 &&
					Magic[1] == 0x000000464E492331 &&
					Magic[2] == 0x000000444E492331 &&
					Magic[3] == 0x00004E414E532331
					)
				{
					iPos += 0x20;
					AddressOfPrivate = *(PULONG)((ULONG_PTR)hModule + iPos);

					*PrivateKey = atoi((LPCSTR)AddressOfPrivate);
					Status = STATUS_SUCCESS;

					break;
				}
				else
				{
					iPos++;
				}
			}
		}

		return Status;
	}

	NTSTATUS NTAPI FindEmoteKeyByParse(PULONG PrivateKey)
	{
		HMODULE    hModule;
		PVOID      FilterTextureAddress;
		ULONG64    Magic[2];
		ULONG_PTR  iPos;
		ULONG      AddressOfPrivate;
		NTSTATUS   Status;

		if (PrivateKey)
			*PrivateKey = 0;

		Status = STATUS_UNSUCCESSFUL;

		LOOP_ONCE
		{
			hModule = LoadLibraryW(L"emotedriver.dll");

			if (!hModule)
				hModule = LoadLibraryW(L"plugin\\emotedriver.dll");

			if (!hModule)
				return STATUS_NO_SUCH_FILE;

			FilterTextureAddress = GetProcAddress(hModule, "?EmoteFilterTexture@@YAXPAEKP6AX0K@Z@Z");

			if (!FilterTextureAddress)
				return STATUS_NOT_SUPPORTED;


			iPos = 0;

			LOOP_FOREVER
			{
				if (iPos >= 500)
				break;

				Magic[0] = *(PULONG64)((ULONG_PTR)FilterTextureAddress + iPos + 0);
				Magic[1] = *(PULONG64)((ULONG_PTR)FilterTextureAddress + iPos + 8);

				if (Magic[0] == 0x458B00000000A364 && Magic[1] == 0x68085D8B104D8B0C)
				{
					//push Offset
					iPos += 0x10;
					AddressOfPrivate = *(PULONG)((ULONG_PTR)FilterTextureAddress + iPos);

					*PrivateKey = atoi((LPCSTR)AddressOfPrivate);
					Status = STATUS_SUCCESS;

					break;
				}
				else
				{
					iPos++;
				}
			}
		}
		return Status;
	}

	NTSTATUS NTAPI FindEmoteKeyByEmotePlayer(PULONG PrivateKey)
	{
		ULONG64    Magic[3];
		PVOID      ModuleAddress;
		ULONG_PTR  iPos, SizeOfImage;
		ULONG      AddressOfPrivate;
		MODULEINFO ModuleInfo;
		NTSTATUS   Status;

		if (PrivateKey)
			*PrivateKey = 0;

		Status = STATUS_UNSUCCESSFUL;


		LOOP_ONCE
		{
			ModuleAddress = GetModuleHandleW(L"emoteplayer.dll");

			if (!ModuleAddress)
				ModuleAddress = LoadLibraryW(L"emoteplayer.dll");

			if (!ModuleAddress)
				ModuleAddress = LoadLibraryW(L"plugin\\emoteplayer.dll");

			if (!ModuleAddress)
				return STATUS_NO_SUCH_FILE;

			RtlZeroMemory(&ModuleInfo, sizeof(ModuleInfo));
			GetModuleInformation(GetCurrentProcess(), (HMODULE)ModuleAddress, &ModuleInfo, sizeof(ModuleInfo));
			iPos = 0;
			SizeOfImage = ModuleInfo.SizeOfImage;

			LOOP_FOREVER
			{
				if (iPos >= SizeOfImage)
				break;

				Magic[0] = *(PULONG64)((ULONG_PTR)ModuleAddress + iPos + 0);
				Magic[1] = *(PULONG64)((ULONG_PTR)ModuleAddress + iPos + 8);

				if (Magic[0] == 0xCD15FFFFFF3885C7 && Magic[1] == 0xFFFFFF3C85C7075B)
				{
					//push Offset
					/*
					CPU Disasm
					Address                      Hex dump                       Command                                      Comments
					05B37A0F                     |> \68 5C9AC005                push offset 05C09A5C                         ; ASCII "149203383"
					05B37A14                     |.  89B5 78FFFFFF              mov dword ptr [ebp-88],esi
					05B37A1A                     |.  E8 4D650800                call 05BBDF6C

					*/


					iPos -= (10 + 5 + 6 + 4);

					//push
					if (((PBYTE)ModuleAddress)[iPos - 1] == 0x68)
					{
						AddressOfPrivate = *(PULONG)((ULONG_PTR)ModuleAddress + iPos);

						*PrivateKey = atoi((LPCSTR)AddressOfPrivate);
						Status = STATUS_SUCCESS;

						break;
					}
					else
					{
						iPos += (10 + 5 + 6 + 4) + sizeof(Magic[0]) * 2;
					}
				}
				else
				{
					iPos++;
				}
			}

		}

		return Status;
	}

	~PSBObject()
	{
		delete[]Data;
	}

private:
	DWORD m_key[6];

	//sub_10001600
	void Decrypt(LPBYTE data, int offset, int length)
	{
		for (int i = 0; i < length; ++i)
		{
			if (0 == m_key[4])
			{
				DWORD v5 = m_key[3];
				DWORD v6 = m_key[0] ^ (m_key[0] << 11);
				m_key[0] = m_key[1];
				m_key[1] = m_key[2];
				DWORD eax = v6 ^ v5 ^ ((v6 ^ (v5 >> 11)) >> 8);
				m_key[2] = v5;
				m_key[3] = eax;
				m_key[4] = eax;
			}
			data[offset + i] ^= (BYTE)m_key[4];
			m_key[4] >>= 8;
		}
	}
};

class PSBNode
{
public:
	PSBObject *Object;
	LPBYTE pCode;

	PSBNode()
	{
		Init(nullptr, nullptr);
	}

	PSBNode(PSBObject* obj, LPBYTE code)
	{
		Init(obj, code);
	}

	PSBNode& operator = (const PSBNode& o)
	{
		Object = o.Object;
		pCode = o.pCode;

		return *this;
	}

	LPCSTR AsString() const
	{
		if (GetValueTypeByID(*pCode) != PsbValueString)
			return nullptr;

		PsbArray arrOffset(Object->pInfo->pStringOffsetArray);
		return Object->pInfo->pStringPool + arrOffset[PsbGetStringIndex(pCode)];
	}

	double AsDouble() const
	{
		return PsbGetDoubleValue(pCode);
	}

	LONG64 AsInt() const
	{
		return PsbGetIntValue(pCode);
	}

	LPCBYTE AsBinary(ULONG &size, ULONG& Index) const
	{
		if (!Object->pInfo->pBinaryPool)
			return NULL;

		PsbArray arrOffset(Object->pInfo->pBinOffsetArray);
		PsbArray arrSize(Object->pInfo->pBinSizeArray);
		ULONG idx = PsbGetBinaryIndex(pCode);
		Index = idx;
		size = arrSize[idx];
		return Object->pInfo->pBinaryPool + arrOffset[idx];
	}

	ePsbValueType GetType() const
	{
		return GetValueTypeByID(*pCode);
	}

	std::string ProbeDictName(unsigned int idx) const
	{
		LPBYTE code = Object->pInfo->pStringIndex;
		PsbArray arr1(code); code += arr1.nBytes;
		PsbArray arr2(code); code += arr2.nBytes;
		PsbArray arr3(code); code += arr3.nBytes;

		std::vector<char> reverse_name;
		ULONG c1 = arr2[arr3[idx]];
		while (c1)
		{
			ULONG c2 = arr2[c1];
			reverse_name.push_back(c1 - arr1[c2]);
			c1 = c2;
		}
		std::reverse(reverse_name.begin(), reverse_name.end());
		return std::string(&reverse_name[0], reverse_name.size());
	}

private:
	void Init(PSBObject* obj, LPBYTE code)
	{
		Object = obj;
		pCode = code;
	}
};

class PSBFile
{
	PSBObject *Object;

public:
	PSBFile() : Object(nullptr) {}

	~PSBFile() {
		if (Object) delete Object;
	}

	PSBObject* DetachObject()
	{
		PSBObject *obj = Object;
		Object = nullptr;
		return obj;
	}

	bool InitFromBuff(LPBYTE buff, ULONG size)
	{
		if (size > 8)
		{
			if (memcmp(buff, "PSB", 4))
			{
				LPBYTE uncomp = NULL;

				if (*(PDWORD)(buff) == LZ4_MAGIC)
				{
					uncomp = getDataFromLz4(buff, size);
				}
				else
				{
					uncomp = getDataFromMDF(buff, size);
				}
				if (uncomp)
				{
					delete[]buff;
					buff = uncomp;
				}
			}

			if (size < 64 || memcmp(buff, "PSB", 4))
				return false;
			if (Object)
				delete Object;

			Object = new PSBObject(buff, size);
			return true;
		}
		return false;
	}

	PSBNode GetRootNode()
	{
		return PSBNode(Object, Object->pInfo->pRootCode);
	}
};

#define INDENT "  "

class PSBDumper
{
	std::string lastDictName; // for bitmap dump
	std::wstring psbimgpath;
	std::string BuildScript;
	ULONG   m_GlobalRetryCount;

	template <typename PixT>
	static void DecompressRLE(LPBYTE dst, LPCBYTE data, ULONG datalen)
	{
		PixT *p = (PixT*)dst;
		LPCBYTE dataend = data + datalen;
		while (data < dataend)
		{
			BYTE code = *data++;
			if (code & 0x80)
			{
				int len = (code & 0x7F) + 3;
				PixT c = *(PixT*)data;
				for (int i = 0; i < len; ++i)
					*p++ = c;

				data += sizeof(PixT);
			}
			else
			{
				ULONG len = (code + 1);
				RtlCopyMemory(p, data, len * sizeof(PixT));
				p += len;
				data += len * sizeof(PixT);
			}
		}
	}

public:

	PSBDumper(LPWSTR _path)
	{
		const std::wstring tmp = _path;
		std::string::size_type pos = tmp.rfind(TJS_W("."));
		assert(pos != tmp.npos);
		psbimgpath = tmp.substr(0, pos);
	}

	void quoteString(LPCSTR str)
	{
		if (str)
		{
			BuildScript += '"';

			LPCSTR p = str;
			char ch;
			while ((ch = *p++))
			{
				if (ch == '"')
				{
					BuildScript += "\\\"";
				}
				else if (ch == '\\')
				{
					BuildScript += "\\\\";
				}
				else if (ch == 0x08)
				{
					BuildScript += "\\b";
				}
				else if (ch == 0x0c)
				{
					BuildScript += "\\f";
				}
				else if (ch == 0x0a)
				{
					BuildScript += "\\n";
				}
				else if (ch == 0x0d)
				{
					BuildScript += "\\r";
				}
				else if (ch == 0x09)
				{
					BuildScript += "\\t";
				}
				else if ((UCHAR)ch < 0x20)
				{
					CHAR buf[256];
					wsprintfA(buf, "\\u%04x", ch);
					BuildScript += buf;
				}
				else
				{
					BuildScript += (char)ch;
				}
			}
			BuildScript += '"';
		}
		else
		{
			BuildScript += "\"\"";
		}
	}

	void dumpArray(const PSBNode &node, const std::string& indentstr)
	{
		int count = PsbGetCount(node.pCode + 1);
		if (count == 0)
		{
			BuildScript += "(const) []";
			return;
		}
		PsbArray arr(node.pCode + 1);
		LPBYTE code = node.pCode + arr.nBytes + 1;
		BuildScript += "(const) [\r\n";
		std::string indentstr2 = indentstr + INDENT;
		for (ULONG i = 0; i < count; ++i)
		{
			if (i != 0)
			{
				BuildScript += ",\r\n";
			}
			BuildScript += indentstr2;
			dump(PSBNode(node.Object, code + arr[i]), indentstr2);
		}
		BuildScript += "\r\n" + indentstr + "]";
	}

	BOOLEAN dumpBitmap(const PSBNode &node, const std::string& indentstr)
	{
		PsbArray arr1(node.pCode + 1);
		PsbArray arr2(node.pCode + 1 + arr1.nBytes);

		std::map<std::string, PSBNode> bitmapInfo;
		int foundBitmap = -2;
		for (ULONG i = 0; i < arr1.nElementCount; ++i)
		{
			std::string name = node.ProbeDictName(arr1[i]);
			bitmapInfo[name] = PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes);
			if (name == "width" || name == "height" || name == "pixel")
			{
				++foundBitmap;
			}
		}

		if (foundBitmap <= 0)
		{
			return false;
		}

		int width = bitmapInfo.find("truncated_width") == bitmapInfo.end() ?
			bitmapInfo["width"].AsInt() : bitmapInfo["truncated_width"].AsInt();

		int height = bitmapInfo.find("truncated_height") == bitmapInfo.end() ?
			bitmapInfo["height"].AsInt() : bitmapInfo["truncated_height"].AsInt();

		bitmapInfo.erase(bitmapInfo.find("width"));
		bitmapInfo.erase(bitmapInfo.find("height"));

		auto itr = bitmapInfo.find("truncated_width");
		if (itr != bitmapInfo.end())
		{
			bitmapInfo.erase(itr);
		}


		itr = bitmapInfo.find("truncated_height");
		if (itr != bitmapInfo.end())
		{
			bitmapInfo.erase(itr);
		}

		const unsigned char *pal = nullptr;
		auto it = bitmapInfo.find("pal");
		ULONG palsize = 0, Index;
		if (it != bitmapInfo.end())
		{
			pal = it->second.AsBinary(palsize, Index);
			bitmapInfo.erase(it);
		}
		int pixelbytes = pal ? 1 : 4;
		ULONG pixdatasize;
		const unsigned char *pixeldata = bitmapInfo["pixel"].AsBinary(pixdatasize, Index);

		it = bitmapInfo.find("compress");
		bool allocBitmap = false;
		if (it != bitmapInfo.end() && !strcmp(it->second.AsString(), "RL"))
		{
			unsigned char *uncomp = new unsigned char[pixelbytes * width * height];
			if (pixelbytes == 1)
			{
				DecompressRLE<unsigned char>(uncomp, pixeldata, pixdatasize);
			}
			else
			{
				DecompressRLE<unsigned long>(uncomp, pixeldata, pixdatasize);
			}
			pixeldata = uncomp;
			allocBitmap = true;
			bitmapInfo.erase(bitmapInfo.find("compress"));
		}
		bitmapInfo.erase(bitmapInfo.find("pixel"));

		//
		WriteBmp(lastDictName, width, height, pixeldata, pal, pixelbytes);

		if (allocBitmap) delete[] pixeldata;

		BuildScript += "(const) %[\r\n";
		std::string indentstr2 = indentstr + INDENT;
		BuildScript += indentstr2 + "\"src\" => ";
		quoteString((lastDictName + ".bmp").c_str());

		for (it = bitmapInfo.begin(); it != bitmapInfo.end(); ++it)
		{
			BuildScript += ",\r\n" + indentstr2;
			quoteString(it->first.c_str());
			BuildScript += " => ";
			dump(it->second, indentstr2);
		}
		BuildScript += "\r\n" + indentstr + "]";

		return TRUE;
	}


	void WriteFile(std::string& FileName, LPBYTE Buffer, ULONG Size)
	{
		_wmkdir(psbimgpath.c_str());

		WCHAR WideDictName[1024] = { 0 };
		MultiByteToWideChar(CP_UTF8, 0, FileName.c_str(), FileName.length(),
			WideDictName, 1024);

		std::wstring filepath;
		filepath = psbimgpath + L"/" + std::wstring(WideDictName);
		FILE *fp = _wfopen(filepath.c_str(), L"wb");
		fwrite(Buffer, Size, 1, fp);
		fclose(fp);
	}


	void WriteBmp(std::string& FileName, int width, int height, LPCBYTE pixeldata, LPCBYTE pal, int pixelbytes)
	{
		unsigned int bmppitch = width * pixelbytes;
		bmppitch = (((bmppitch - 1) >> 2) + 1) << 2;

		_wmkdir(psbimgpath.c_str());

		WCHAR WideDictName[1024] = { 0 };
		MultiByteToWideChar(CP_UTF8, 0, FileName.c_str(), FileName.length(),
			WideDictName, 1024);

		std::wstring filepath;
		filepath = psbimgpath + L"/" + std::wstring(WideDictName) + L".bmp";
		FILE *fp = _wfopen(filepath.c_str(), L"wb");
		assert(fp);
		unsigned int uidata = 0x4d42;
		fwrite(&uidata, 2, 1, fp);  /* bfType */
		uidata = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmppitch * height + (pixelbytes == 1 ? 1024 : 0);
		fwrite(&uidata, 4, 1, fp);  /* bfSize */
		uidata = 0;
		fwrite(&uidata, 2, 1, fp);  /* bfReserved1 */
		fwrite(&uidata, 2, 1, fp);  /* bfReserved2 */
		uidata = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (pixelbytes == 1 ? 1024 : 0);
		fwrite(&uidata, 4, 1, fp);  /* bfOffBits */
		uidata = sizeof(BITMAPINFOHEADER);
		fwrite(&uidata, 4, 1, fp);  /* biSize */
		fwrite(&width, 4, 1, fp);  /* biWidth */
		uidata = -height;
		fwrite(&uidata, 4, 1, fp);  /* biHeight */
		uidata = 1;
		fwrite(&uidata, 2, 1, fp);  /* biPlanes */
		uidata = pixelbytes * 8;
		fwrite(&uidata, 2, 1, fp);  /* biBitCount */
		uidata = BI_RGB;
		fwrite(&uidata, 4, 1, fp);  /* biCompression */
		uidata = 0;
		fwrite(&uidata, 4, 1, fp);  /* biSizeImage */
		fwrite(&uidata, 4, 1, fp);  /* biXPelsPerMeter */
		fwrite(&uidata, 4, 1, fp);  /* biYPelsPerMeter */
		fwrite(&uidata, 4, 1, fp);  /* biClrUsed */
		fwrite(&uidata, 4, 1, fp);  /* biClrImportant */

		// write palette
		if (pixelbytes == 1)
		{
			fwrite(pal, 1024, 1, fp);
		}

		// write pixel
		static const char paddingData[16] = { 0 };
		int paddingSize = bmppitch - width * pixelbytes;
		assert(paddingSize < 16);
		const unsigned char *line = pixeldata;
		int srcpitch = width * pixelbytes;

		for (int y = 0; y < height; ++y)
		{
			fwrite(line, srcpitch, 1, fp);
			line += srcpitch;
			if (paddingSize) fwrite(paddingData, paddingSize, 1, fp);
		}

		fclose(fp);
	}

	void SaveEmoteImage(std::string& FileName, PBYTE Stream, const PsbTextureMetaData& meta)
	{
		auto pixels = new BYTE[meta.Width * meta.Height * 4];

		if ("RGBA8" == meta.TexType)
		{
			ReadRgba8(Stream, meta, pixels);
			WriteBmp(FileName, meta.Width, meta.Height, pixels, NULL, 4);
		}
		else if ("RGBA4444" == meta.TexType)
		{
			ReadRgba4444(Stream, meta, pixels);
			WriteBmp(FileName, meta.Width, meta.Height, pixels, NULL, 4);
		}
		else
		{
			WriteFile(FileName, Stream, meta.DataSize);
		}

		if (pixels)
			delete[] pixels;
	}

	USHORT ToUInt16(LPBYTE value, int index)
	{
		return (USHORT)(value[index] | value[index + 1] << 8);
	}

	void ReadRgba4444(LPBYTE input, const PsbTextureMetaData& meta, LPBYTE output)
	{
		int dst_stride = meta.Width * 4;
		int src_stride = meta.FullWidth * 2;
		int dst = 0;
		auto row = new byte[src_stride];

		for (ULONG i = 0; i < meta.Height; ++i)
		{
			RtlCopyMemory(row, (input + src_stride * i), src_stride);
			int src = 0;
			for (int x = 0; x < dst_stride; x += 4)
			{
				ULONG p = ToUInt16(row, src);
				src += 2;
				output[dst++] = (byte)((p & 0x000Fu) * 0xFFu / 0x000Fu);
				output[dst++] = (byte)((p & 0x00F0u) * 0xFFu / 0x00F0u);
				output[dst++] = (byte)((p & 0x0F00u) * 0xFFu / 0x0F00u);
				output[dst++] = (byte)((p & 0xF000u) * 0xFFu / 0xF000u);
			}
		}
		delete[] row;
	}

	void ReadRgba8(LPBYTE input, const PsbTextureMetaData meta, LPBYTE output)
	{
		int dst_stride = (int)meta.Width * 4;
		ULONG next_row = 0;
		int src_stride = meta.FullWidth * 4;
		int dst = 0;
		for (ULONG i = 0; i < meta.Height; ++i)
		{
			RtlCopyMemory(output + dst, input + next_row, dst_stride);
			dst += dst_stride;
			next_row += src_stride;
		}
	}

	void dumpDict(const PSBNode &node, const std::string& indentstr)
	{
		PsbArray arr1(node.pCode + 1);
		PsbArray arr2(node.pCode + 1 + arr1.nBytes);
		if (arr1.nElementCount == 0)
		{
			BuildScript += "(const) %[]";
			return;
		}

#if 0
		if (dumpBitmap(node, indentstr)) 
		{
			return;
		}
#endif 

		BuildScript += "(const) %[\r\n";
		std::string indentstr2 = indentstr + INDENT;
		for (ULONG i = 0; i < arr1.nElementCount; ++i)
		{
			if (i != 0)
			{
				BuildScript += ",\r\n";
			}
			BuildScript += indentstr2;
			lastDictName = node.ProbeDictName(arr1[i]);
			quoteString(lastDictName.c_str());
			BuildScript += " => ";
			dump(PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes), indentstr2);
		}
		BuildScript += "\r\n" + indentstr + "]";
	}


	void dumpBinary(const PSBNode &node)
	{
		ULONG FileSize, Index;
		LPCBYTE pData = node.AsBinary(FileSize, Index);

		CHAR  BuilderName[MAX_PATH] = { 0 };

		WriteFile(lastDictName, (PBYTE)pData, FileSize);
		
		wsprintfA(BuilderName, "\"Binary::%d\"", Index);
		BuildScript += BuilderName;
	}

	void dump(const PSBNode &node, const std::string& indentstr = "")
	{
		CHAR NumStr[1024];
		switch (node.GetType())
		{
		case PsbValueVoid:
			BuildScript += "void";
			break;

		case PsbValueBoolean:
			BuildScript += node.AsInt() ? "true" : "false";
			break;

		case PsbValueInteger:
			memset(NumStr, 0, sizeof(NumStr));
			sprintf(NumStr, "%d", node.AsInt());
			BuildScript += NumStr;
			break;

		case PsbValueFloat:
			memset(NumStr, 0, sizeof(NumStr));
			sprintf(NumStr, "%lf", node.AsDouble());
			BuildScript += NumStr;
			break;

		case PsbValueString:
			quoteString(node.AsString());
			break;
		case PsbValueBinary:
			dumpBinary(node);
			break;

		case PsbValueArray:
			dumpArray(node, indentstr);
			break;

		case PsbValueDictionay:
			dumpDict(node, indentstr);
			break;
		}
	}


	bool dumpEmote(const PSBNode &node)
	{
		if (node.GetType() == PsbValueDictionay)
		{
			PsbArray arr1(node.pCode + 1);
			PsbArray arr2(node.pCode + 1 + arr1.nBytes);

			std::map<std::string, PSBNode> EmoteInfo;
			for (ULONG i = 0; i < arr1.nElementCount; ++i)
			{
				std::string name = node.ProbeDictName(arr1[i]);
				EmoteInfo[name] = PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes);
			}

			auto it = EmoteInfo.find("source");
			if (it == EmoteInfo.end())
			{
				return false;
			}
			else
			{
				const PSBNode& sourceNode = it->second;

				PsbArray arr3(sourceNode.pCode + 1);
				PsbArray arr4(sourceNode.pCode + 1 + arr3.nBytes);

				std::map<std::string, PSBNode> EmoteTextureInfo;
				for (ULONG i = 0; i < arr3.nElementCount; ++i)
				{
					std::string name = node.ProbeDictName(arr3[i]);
					EmoteTextureInfo[name] = PSBNode(sourceNode.Object, sourceNode.pCode + 1 + arr4[i] + arr3.nBytes + arr4.nBytes);
				}

				for (auto emoteList : EmoteTextureInfo)
				{

					PsbArray arr5(emoteList.second.pCode + 1);
					PsbArray arr6(emoteList.second.pCode + 1 + arr5.nBytes);

					std::map<std::string, PSBNode> EmoteChunk;

					for (ULONG i = 0; i < arr5.nElementCount; ++i)
					{
						std::string name = node.ProbeDictName(arr5[i]);
						EmoteChunk[name] = PSBNode(emoteList.second.Object, emoteList.second.pCode + 1 + arr6[i] + arr5.nBytes + arr6.nBytes);
					}

					if (EmoteChunk.find("texture") == EmoteChunk.end())
						return false;

					const PSBNode& textureMeta = EmoteChunk["texture"];

					std::string FileName = emoteList.first.c_str();


					PsbArray arr7(textureMeta.pCode + 1);
					PsbArray arr8(textureMeta.pCode + 1 + arr7.nBytes);

					std::map<std::string, PSBNode> EmoteChunkInfo;
					for (ULONG i = 0; i < arr7.nElementCount; ++i)
					{
						std::string name = node.ProbeDictName(arr7[i]);
						EmoteChunkInfo[name] = PSBNode(textureMeta.Object, textureMeta.pCode + 1 +
							arr8[i] + arr7.nBytes + arr8.nBytes);
					}

					ULONG pixdatasize, Index;
					int width = EmoteChunkInfo["width"].AsInt();
					int height = EmoteChunkInfo["height"].AsInt();
					int truncated_width = EmoteChunkInfo["truncated_width"].AsInt();
					int truncated_height = EmoteChunkInfo["truncated_height"].AsInt();
					std::string TextureType = EmoteChunkInfo["type"].AsString();
					const unsigned char *pixeldata = EmoteChunkInfo["pixel"].AsBinary(pixdatasize, Index);

					printf("%d, %d, %d, %d\n", width, height, truncated_width, truncated_height);

					PsbTextureMetaData info;
					info.BPP = 32;
					info.FullWidth = width;
					info.FullHeight = height;
					info.Width = truncated_width;
					info.Height = truncated_height;
					info.TexType = TextureType;
					info.DataSize = pixdatasize;

					SaveEmoteImage(FileName, (LPBYTE)pixeldata, info);
				}
			}
		}
		return true;
	}


	void dumpPimg(const PSBNode& node)
	{
		PsbArray arr1(node.pCode + 1);
		PsbArray arr2(node.pCode + 1 + arr1.nBytes);

		std::map<std::string, PSBNode> PimgInfo;
		for (ULONG i = 0; i < arr1.nElementCount; ++i)
		{
			std::string name = node.ProbeDictName(arr1[i]);
			PimgInfo[name] = PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes);
		}

		for (auto it : PimgInfo)
		{
			if (it.second.GetType() == ePsbValueType::PsbValueBinary)
			{
				std::string FileName = it.first;
				ULONG FileSize, Index;
				LPCBYTE Buffer = it.second.AsBinary(FileSize, Index);

				WriteFile(FileName, (LPBYTE)Buffer, FileSize);
			}
		}
	}

	void start(const PSBNode &node, const std::wstring& FileName)
	{
		std::wstring FileExtension = GetExtensionName(FileName);

		dump(node);

		//Decompile Script
		std::wstring scriptFile = psbimgpath + TJS_W(".psb.tjs");
		FILE* fp = _wfopen(scriptFile.c_str(), TJS_W("wb"));
		fwrite(BuildScript.c_str(), 1, BuildScript.length(), fp);
		fclose(fp);

		//Dump Binary File: Emote, RL/Bitmap, Other
		if (!lstrcmpiW(FileExtension.c_str(), L"pimg"))
		{
			dumpPimg(node);
		}
		else
		{
			if (!dumpEmote(node))
			{
				dumpAllFile(node);
			}
		}

		dumpText(node);
	}

	void dumpText(const PSBNode& node)
	{
		PsbArray arr1(node.pCode + 1);
		PsbArray arr2(node.pCode + 1 + arr1.nBytes);

		std::map<std::string, PSBNode> SceneInfo;
		for (ULONG i = 0; i < arr1.nElementCount; ++i)
		{
			std::string name = node.ProbeDictName(arr1[i]);
			SceneInfo[name] = PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes);
		}

		if (SceneInfo.find("scenes") == SceneInfo.end() && SceneInfo.find("list") == SceneInfo.end())
			return;

		std::wstring FileName = psbimgpath + L".ks.txt";
		FILE* file = _wfopen(FileName.c_str(), L"wb");

		if (SceneInfo.find("scenes") != SceneInfo.end())
		{
			LOOP_ONCE
			{
				PSBNode SceneNode = SceneInfo["scenes"];

				int count = PsbGetCount(SceneNode.pCode + 1);
				if (count == 0)
					break;

				PsbArray arrScene(SceneNode.pCode + 1);
				LPBYTE code = SceneNode.pCode + arrScene.nBytes + 1;

				printf("Count : %d\n", count);
				for (ULONG i = 0; i < count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrScene[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					std::map<std::string, PSBNode> TextInfo;
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						std::string name2 = TextNode.ProbeDictName(arr3[j]);
						TextInfo[name2] = PSBNode(TextNode.Object, TextNode.pCode + 1 + arr4[j] + arr3.nBytes + arr4.nBytes);
					}

					//getchar();

					if (TextInfo.find("texts") == TextInfo.end())
						continue;

					PSBNode TextChunkNode = TextInfo["texts"];

					int count2 = PsbGetCount(TextChunkNode.pCode + 1);
					if (count2 == 0)
					{
						break;
					}

					PsbArray arrText(TextChunkNode.pCode + 1);
					LPBYTE code2 = TextChunkNode.pCode + arrText.nBytes + 1;

					for (ULONG k = 0; k < count2; k++)
					{
						PSBNode TextListNode(node.Object, code2 + arrText[k]);

						int count3 = PsbGetCount(TextListNode.pCode + 1);
						if (count3 == 0)
						{
							break;
						}

						PsbArray arrAtomText(TextListNode.pCode + 1);
						LPBYTE code3 = TextListNode.pCode + arrAtomText.nBytes + 1;

						for (ULONG idx = 0; idx < 3; idx++)
						{
							PSBNode TextString(node.Object, code3 + arrAtomText[idx]);

							if (TextString.GetType() == ePsbValueType::PsbValueString)
							{
								if (lstrlenA(TextString.AsString()))
									fprintf(file, "%s\r\n", TextString.AsString());
							}
						}
					}
				}
			}
		}

		//selects in scenes
		if (SceneInfo.find("scenes") != SceneInfo.end())
		{
			LOOP_ONCE
			{
				PSBNode SceneNode = SceneInfo["scenes"];

				int count = PsbGetCount(SceneNode.pCode + 1);
				if (count == 0)
					break;

				PsbArray arrScene(SceneNode.pCode + 1);
				LPBYTE code = SceneNode.pCode + arrScene.nBytes + 1;

				for (ULONG i = 0; i < count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrScene[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					std::map<std::string, PSBNode> TextInfo;
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						std::string name2 = TextNode.ProbeDictName(arr3[j]);
						TextInfo[name2] = PSBNode(TextNode.Object, TextNode.pCode + 1 + arr4[j] + arr3.nBytes + arr4.nBytes);
					}

					if (TextInfo.find("selects") == TextInfo.end())
						continue;

					PSBNode SelectsChunkNode = TextInfo["selects"];

					int count2 = PsbGetCount(SelectsChunkNode.pCode + 1);
					if (count2 == 0)
						continue;
					
					PsbArray arrText(SelectsChunkNode.pCode + 1);
					LPBYTE   code2 = SelectsChunkNode.pCode + arrText.nBytes + 1;

					for (ULONG k = 0; k < count2; k++)
					{
						PSBNode SelNode(node.Object, code2 + arrText[k]);

						PsbArray arr5(SelNode.pCode + 1);
						PsbArray arr6(SelNode.pCode + 1 + arr5.nBytes);

						std::map<std::string, PSBNode> SelInfo;
						for (ULONG m = 0; m < arr5.nElementCount; ++m)
						{
							std::string name3 = SelNode.ProbeDictName(arr5[m]);
							SelInfo[name3] = PSBNode(SelNode.Object, SelNode.pCode + 1 + arr6[m] + arr5.nBytes + arr6.nBytes);
						}


						if (SelInfo.find("text") != SelInfo.end() &&
							lstrlenA(SelInfo["text"].AsString()))
						{
							fprintf(file, "%s\r\n", SelInfo["text"].AsString());
						}
					}
				}
			}
		}

		if (SceneInfo.find("scenes") != SceneInfo.end())
		{
			LOOP_ONCE
			{
				PSBNode SceneNode = SceneInfo["scenes"];

				int count = PsbGetCount(SceneNode.pCode + 1);
				if (count == 0)
					break;

				PsbArray arrScene(SceneNode.pCode + 1);
				LPBYTE code = SceneNode.pCode + arrScene.nBytes + 1;

				for (ULONG i = 0; i < count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrScene[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					std::map<std::string, PSBNode> TextInfo;
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						std::string name2 = TextNode.ProbeDictName(arr3[j]);
						TextInfo[name2] = PSBNode(TextNode.Object, TextNode.pCode + 1 + arr4[j] + arr3.nBytes + arr4.nBytes);
					}

					if (TextInfo.find("selects") == TextInfo.end())
						continue;

					PSBNode SelectsChunkNode = TextInfo["selects"];

					int count2 = PsbGetCount(SelectsChunkNode.pCode + 1);
					if (count2 == 0)
						continue;

					PsbArray arrText(SelectsChunkNode.pCode + 1);
					LPBYTE   code2 = SelectsChunkNode.pCode + arrText.nBytes + 1;

					for (ULONG k = 0; k < count2; k++)
					{
						PSBNode SelNode(node.Object, code2 + arrText[k]);

						PsbArray arr5(SelNode.pCode + 1);
						PsbArray arr6(SelNode.pCode + 1 + arr5.nBytes);

						std::map<std::string, PSBNode> SelInfo;
						for (ULONG m = 0; m < arr5.nElementCount; ++m)
						{
							std::string name3 = SelNode.ProbeDictName(arr5[m]);
							SelInfo[name3] = PSBNode(SelNode.Object, SelNode.pCode + 1 + arr6[m] + arr5.nBytes + arr6.nBytes);
						}


						if (SelInfo.find("runLineStr") != SelInfo.end() &&
							lstrlenA(SelInfo["runLineStr"].AsString()))
						{
							fprintf(file, "%s\r\n", SelInfo["runLineStr"].AsString());
						}
					}
				}
			}
		}

		if (SceneInfo.find("list") != SceneInfo.end())
		{
			LOOP_ONCE
			{
				PSBNode ListNode = SceneInfo["list"];

				int count = PsbGetCount(ListNode.pCode + 1);

				if (count == 0)
					break;

				PsbArray arrList(ListNode.pCode + 1);
				LPBYTE code = ListNode.pCode + arrList.nBytes + 1;

				for (ULONG i = 0; i < count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrList[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					std::map<std::string, PSBNode> TitleInfo;
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						std::string name2 = TextNode.ProbeDictName(arr3[j]);
						TitleInfo[name2] = PSBNode(TextNode.Object, TextNode.pCode + 1 + arr4[j] + arr3.nBytes + arr4.nBytes);
					}

					if (TitleInfo.find("title") != TitleInfo.end() &&
						lstrlenA(TitleInfo["title"].AsString()))
					{
						fprintf(file, "%s\r\n", TitleInfo["title"].AsString());
					}

				}

				for (ULONG i = 0; i < count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrList[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					std::map<std::string, PSBNode> TitleInfo;
					//selects
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						std::string name2 = TextNode.ProbeDictName(arr3[j]);
						TitleInfo[name2] = PSBNode(TextNode.Object, TextNode.pCode + 1 + arr4[j] + arr3.nBytes + arr4.nBytes);
					}

					if (TitleInfo.find("selects") != TitleInfo.end() &&
						TitleInfo["selects"].GetType() != ePsbValueType::PsbValueVoid)
					{
						PsbArray arrSelects(TitleInfo["selects"].pCode + 1);
						LPBYTE code3 = TitleInfo["selects"].pCode + arrSelects.nBytes + 1;

						int SelectCount = PsbGetCount(TitleInfo["selects"].pCode + 1);

						if (SelectCount == 0)
							break;

						for (ULONG k = 0; k < SelectCount; k++)
						{
							PSBNode SelectNode(node.Object, code3 + arrSelects[k]);

							PsbArray arr5(SelectNode.pCode + 1);
							PsbArray arr6(SelectNode.pCode + 1 + arr5.nBytes);

							std::map<std::string, PSBNode> SelectsInfo;
							for (ULONG m = 0; m < arr5.nElementCount; ++m)
							{
								std::string name3 = SelectNode.ProbeDictName(arr5[m]);
								SelectsInfo[name3] = PSBNode(SelectNode.Object, SelectNode.pCode + 1 + arr6[m] + arr5.nBytes + arr6.nBytes);
							}

							if (SelectsInfo.find("runLineStr") != SelectsInfo.end() &&
								lstrlenA(SelectsInfo["runLineStr"].AsString()))
							{
								fprintf(file, "%s\r\n", SelectsInfo["runLineStr"].AsString());
							}
						}
					}
				}

				for (ULONG i = 0; i < count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrList[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					std::map<std::string, PSBNode> TitleInfo;
					//selects
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						std::string name2 = TextNode.ProbeDictName(arr3[j]);
						TitleInfo[name2] = PSBNode(TextNode.Object, TextNode.pCode + 1 + arr4[j] + arr3.nBytes + arr4.nBytes);
					}

					if (TitleInfo.find("selects") != TitleInfo.end() &&
						TitleInfo["selects"].GetType() != ePsbValueType::PsbValueVoid)
					{
						PsbArray arrSelects(TitleInfo["selects"].pCode + 1);
						LPBYTE code3 = TitleInfo["selects"].pCode + arrSelects.nBytes + 1;

						int SelectCount = PsbGetCount(TitleInfo["selects"].pCode + 1);

						if (SelectCount == 0)
							break;

						for (ULONG k = 0; k < SelectCount; k++)
						{
							PSBNode SelectNode(node.Object, code3 + arrSelects[k]);

							PsbArray arr5(SelectNode.pCode + 1);
							PsbArray arr6(SelectNode.pCode + 1 + arr5.nBytes);

							std::map<std::string, PSBNode> SelectsInfo;
							for (ULONG m = 0; m < arr5.nElementCount; ++m)
							{
								std::string name3 = SelectNode.ProbeDictName(arr5[m]);
								SelectsInfo[name3] = PSBNode(SelectNode.Object, SelectNode.pCode + 1 + arr6[m] + arr5.nBytes + arr6.nBytes);
							}

							if (SelectsInfo.find("text") != SelectsInfo.end() &&
								lstrlenA(SelectsInfo["text"].AsString()))
							{
								fprintf(file, "%s\r\n", SelectsInfo["text"].AsString());
							}
						}
					}
				}
			}
		}

		fclose(file);
	}

	void dumpAllFile(const PSBNode& node)
	{
		switch (node.GetType())
		{
		case PsbValueBinary:
			
			//dumpBinaryInternal(node);
			break;

		case PsbValueArray:
			dumpArrayInternal(node);
			break;
		case PsbValueDictionay:
			dumpDictInternal(node);
			break;
		default:
			break;
		}
	}

	void dumpArrayInternal(const PSBNode &node)
	{
		int count = PsbGetCount(node.pCode + 1);
		if (count == 0)
		{
			return;
		}

		PsbArray arr(node.pCode + 1);
		LPBYTE code = node.pCode + arr.nBytes + 1;

		for (ULONG i = 0; i < count; ++i)
		{
			dumpAllFile(PSBNode(node.Object, code + arr[i]));
		}
	}


	void dumpDictInternal(const PSBNode &node)
	{
		PsbArray arr1(node.pCode + 1);
		PsbArray arr2(node.pCode + 1 + arr1.nBytes);

		if (arr1.nElementCount == 0)
		{
			return;
		}

		if (dumpBitmapInternal(node))
		{
			return;
		}

		for (ULONG i = 0; i < arr1.nElementCount; ++i)
		{
			lastDictName = node.ProbeDictName(arr1[i]);
			dumpAllFile(PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes));
		}
	}

	BOOLEAN dumpBitmapInternal(const PSBNode &node)
	{
		PsbArray arr1(node.pCode + 1);
		PsbArray arr2(node.pCode + 1 + arr1.nBytes);

		std::map<std::string, PSBNode> bitmapInfo;
		int foundBitmap = -2;
		for (ULONG i = 0; i < arr1.nElementCount; ++i)
		{
			std::string name = node.ProbeDictName(arr1[i]);
			bitmapInfo[name] = PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes);
			if (name == "width" || name == "height" || name == "pixel")
			{
				++foundBitmap;
			}
		}

		if (foundBitmap <= 0)
		{
			return false;
		}

		int width = bitmapInfo["width"].AsInt();
		int height = bitmapInfo["height"].AsInt();

		bitmapInfo.erase(bitmapInfo.find("width"));
		bitmapInfo.erase(bitmapInfo.find("height"));

		const unsigned char *pal = nullptr;
		auto it = bitmapInfo.find("pal");
		ULONG palsize = 0, Index;
		if (it != bitmapInfo.end())
		{
			pal = it->second.AsBinary(palsize, Index);
			bitmapInfo.erase(it);
		}
		int pixelbytes = pal ? 1 : 4;
		ULONG pixdatasize;
		const unsigned char *pixeldata = bitmapInfo["pixel"].AsBinary(pixdatasize, Index);

		it = bitmapInfo.find("compress");
		bool allocBitmap = false;
		if (it != bitmapInfo.end() && !strcmp(it->second.AsString(), "RL"))
		{
			unsigned char *uncomp = new unsigned char[pixelbytes * width * height];
			if (pixelbytes == 1)
			{
				DecompressRLE<unsigned char>(uncomp, pixeldata, pixdatasize);
			}
			else
			{
				DecompressRLE<unsigned long>(uncomp, pixeldata, pixdatasize);
			}
			pixeldata = uncomp;
			allocBitmap = true;
			bitmapInfo.erase(bitmapInfo.find("compress"));
		}
		bitmapInfo.erase(bitmapInfo.find("pixel"));

		//
		WriteBmp(lastDictName, width, height, pixeldata, pal, pixelbytes);

		if (allocBitmap) delete[] pixeldata;


		for (it = bitmapInfo.begin(); it != bitmapInfo.end(); ++it)
		{
			dumpAllFile(it->second);
		}

		return TRUE;
	}

	std::wstring GetExtensionName(const std::wstring& Name)
	{
		std::wstring::size_type Pos = Name.find_last_of(L".");

		if (Pos == std::wstring::npos)
		{
			return NULL;
		}
		else
		{
			return Name.substr(Pos + 1, Name.length());
		}
	}
};


ForceInline std::wstring FASTCALL GetPackageName(std::wstring& FileName)
{
	auto Index = FileName.find_last_of(L'\\');

	if (Index != std::wstring::npos)
		FileName = FileName.substr(Index + 1, std::wstring::npos);

	Index = FileName.find_last_of(L'/');
	if (Index != std::wstring::npos)
		FileName = FileName.substr(Index + 1, std::wstring::npos);

	return FileName;
}


ForceInline std::wstring FASTCALL ReplaceFileNameExtension(std::wstring& Path, PCWSTR NewExtensionName)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == std::wstring::npos)
		return Path + NewExtensionName;

	return Path.substr(0, Ptr) + NewExtensionName;
}

ForceInline std::wstring FASTCALL GetFileBasePath(std::wstring& Path)
{
	ULONG_PTR Ptr;

	auto PathFormat = Path;
	for (auto& Ch : PathFormat)
		if (Ch == L'/')
			Ch = L'\\';

	Ptr = PathFormat.find_last_of(L"\\");
	if (Ptr == std::wstring::npos)
		return std::wstring(NULL);

	return Path.substr(0, Ptr);
}


#include "PsbFile.h"

int _tmain(int argc, _TCHAR *argv[])
{
	ml::MlInitialize();

	if (argc <= 1)
		return 0;
	
	FILE* fp = _tfopen(argv[1], _T("rb"));
	if (!fp) 
	{
		_tprintf(_T("file \"%s\" does not exist"), argv[1]);
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char *buf = new unsigned char[size];
	fread(buf, size, 1, fp);
	fclose(fp);

	std::wstring FileName = ReplaceFileNameExtension(GetPackageName(std::wstring(argv[1])), L"");
	std::wstring BaseName = GetFileBasePath(std::wstring(argv[1]));

	//PsbJson::DecompilePsbJson(buf, size, BaseName.c_str(), FileName.c_str());

	PSBFile psbf;
	if (!psbf.InitFromBuff(buf, size))
	{
		TVPThrowExceptionMessage(TJS_W("Invalid psb file"));
		return -2;
	}
	PSBDumper dumper(argv[1]);
	dumper.start(psbf.GetRootNode(), std::wstring(argv[1]));

	return 0;
}
