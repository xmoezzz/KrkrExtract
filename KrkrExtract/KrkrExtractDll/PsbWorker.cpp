#include "zlib.h"
#include "tp_stub.h"
#include "KrkrExtract.h"
#include <Psapi.h>
#include "lz4.h"
#include "lz4frame.h"
#include "my.h"
#include "ml.h"


VOID FASTCALL OutputInfo(const WCHAR* Info)
{
	if (GlobalData::GetGlobalData()->DebugOn)
		PrintConsoleW(Info);
}

#pragma comment(lib, "psapi.lib")

#ifndef LZ4_MAGIC
#define LZ4_MAGIC 0x184D2204
#endif



#pragma pack(push, 1)

typedef struct PSB_HEADER_V2
{
	DWORD  HeaderMagic;
	USHORT Version;
	USHORT Flag;
	LPBYTE pHeader;
	LPBYTE pStringIndex;
	LPBYTE pStringOffsetArray;
	LPCSTR pStringPool;
	LPBYTE pBinOffsetArray;
	LPBYTE pBinSizeArray;
	LPBYTE pBinaryPool;
	LPBYTE pRootCode;
} PSB_HEADER_V2, *pPSB_HEADER_V2;

#pragma pack(pop)

typedef struct PsbTextureMetaData
{
	ULONG       Width;
	ULONG       Height;
	ULONG       OffsetX;
	ULONG       OffsetY;
	ULONG       BPP;
	CHAR        TexType[20];
	ULONG       FullWidth;
	ULONG       FullHeight;
	ULONG       DataOffset;
	ULONG       DataSize;

	PsbTextureMetaData()
	{
		RtlZeroMemory(TexType, sizeof(TexType));
	}
}PsbTextureMetaData, *pPsbTextureMetaData;


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

unsigned char *getDataFromMDF(const unsigned char *buff, unsigned long &size)
{
	if (size <= 10 || *(PDWORD)buff == TAG3("mdf")) return NULL;

	ULONG Uncompsize = *(unsigned long*)&buff[4];
	PBYTE uncomp = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Uncompsize);
	if (Z_OK != uncompress(uncomp, &Uncompsize, buff + 8, size - 8))
	{
		HeapFree(GetProcessHeap(), 0, uncomp);
		return NULL;
	}
	size = Uncompsize;
	return uncomp;
}


unsigned char *getDataFromLz4(const unsigned char *buff, unsigned long &size)
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
	uncompr_data = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, LoDword(frameInfo.contentSize));

	do
	{
		dstSize = LoDword(frameInfo.contentSize) - dstPos;
		srcSize = size - pos;

		err = LZ4F_decompress(context, &uncompr_data[dstPos], &dstSize, &buff[pos], &srcSize, NULL);

		if (LZ4F_isError(err))
		{
			HeapFree(GetProcessHeap(), 0, uncompr_data);
			return nullptr;
		}

		dstPos += dstSize;
		pos += srcSize;
	} while (err);

	size = LoDword(frameInfo.contentSize);
	return uncompr_data;
}

static ePsbValueType GetValueTypeByID(int nID)
{
	switch (nID)
	{
	case 0u:
	case 1u: //null
		return PsbValueVoid;

	case 2u: //true
	case 3u: //false
		return PsbValueBoolean;

	case 4u:
	case 5u:
	case 6u:
	case 7u:
	case 8u:
		//Int

	case 9u:
	case 0xAu:
	case 0xBu:
	case 0xCu:

		return PsbValueInteger;

	case 0x1Du:
	case 0x1Eu:
	case 0x1Fu: //Double
		return PsbValueFloat;

	case 0x15u:
	case 0x16u:
	case 0x17u:
	case 0x18u:
		return PsbValueString;

	case 0x19u:
	case 0x1Au:
	case 0x1Bu:
	case 0x1Cu:
		return PsbValueBinary;

	case 0x20u:
		return PsbValueArray;

	case 0x21u:
		return PsbValueDictionay;

	default:
		OutputInfo(L"psb: internal error: unknown internal type detected.\n");
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
	case 9:  return (LONG64)*(PULONG)code | (((tjs_int64)*(PCHAR)(code + 4)) << 32);
	case 10: return (LONG64)*(PULONG)code | (((tjs_int64)*(PSHORT)(code + 4)) << 32);
	case 11: return (LONG64)*(PULONG)code | (((tjs_int64)*(PSHORT)(code + 4)) << 32)
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
		return (double)PsbGetLongLong(code);
	case 29: case 30:
		return PsbGetFloat(code);
	case 31:
		return PsbGetDouble(code);
	default:
		OutputInfo(L"psb: can't convert value to double.");
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
		return (LONG64)PsbGetFloat(code);
	case 31:
		return (LONG64)PsbGetDouble(code);
	default:
		OutputInfo(L"psb: can't convert value to long int.");
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
		return (LONG)PsbGetLongLong(code);
	case 29: case 30:
		return (LONG)PsbGetFloat(code);
	case 31:
		return (LONG)PsbGetDouble(code);
	default:
		OutputInfo(L"psb: can't convert value to int.");
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
		return (BOOLEAN)PsbGetLong(code);
	case 9: case 10: case 11: case 12:
		return (BOOLEAN)PsbGetLongLong(code);
	case 29: case 30:
		return (BOOLEAN)PsbGetFloat(code);
	case 31:
		return (BOOLEAN)PsbGetDouble(code);
	default:
		OutputInfo(L"psb: can't convert value to bool.");
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
	ULONG          EmotePrivateKey;

	PSBObject(LPBYTE data, ULONG len, NTSTATUS& Result)
		: Data(data),
		DataLength(len),
		EmotePrivateKey(0)
	{
		Result = STATUS_SUCCESS;
		pInfo = (PSB_HEADER_V2*)Data;

		if (pInfo->Version != 2 && pInfo->Version != 3)
		{
			OutputInfo(L"psb: unsupported version");
		}
		else
		{
			m_key[0] = 0x075BCD15;
			m_key[1] = 0x159A55E5;
			m_key[2] = 0x1F123BB5;
			m_key[3] = GetPrivateKey();
			m_key[4] = 0;
			m_key[5] = 0;

			if (pInfo->Flag & 1 && pInfo->Version == 3)
				Decrypt(data + 0x8, 0, pInfo->Version == 3 ? 0x24 : 0x20);

			//PSB V2 will not mark the field
			//root is a directory?
			if ( pInfo->Flag & 2 ||
				((ULONG_PTR)pInfo->pRootCode >= len) || 
				data[(ULONG_PTR)pInfo->pRootCode] != 0x21)
			{
				Decrypt(data + (ULONG_PTR)pInfo->pStringIndex, 0,
					(ULONG_PTR)pInfo->pBinOffsetArray - (ULONG_PTR)pInfo->pStringIndex);
				//Decrypt(data + 8, 0, 0x20);

				if (data[(ULONG_PTR)pInfo->pRootCode] != 0x21)
				{
					Result = STATUS_UNSUCCESSFUL;
					return;
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
	}

	ULONG  GetPrivateKey()
	{
		ULONG    Key;

		if (EmotePrivateKey)
			return EmotePrivateKey;

		if (FindEmoteKeyByParse(&Key) >= 0 || FindEmoteKeyByMark(&Key) >= 0 || FindEmoteKeyByEmotePlayer(&Key) >= 0)
		{
			EmotePrivateKey = Key;
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
					
					*PrivateKey = StringToInt32A((LPSTR)AddressOfPrivate);
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
			hModule = GetModuleHandleW(L"emotedriver.dll");

			if (!hModule)
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

					*PrivateKey = StringToInt32A((LPSTR)AddressOfPrivate);
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

						*PrivateKey = StringToInt32A((LPSTR)AddressOfPrivate);
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
		HeapFree(GetProcessHeap(), 0, Data);
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

	string ProbeDictName(unsigned int idx) const
	{
		LPBYTE code = Object->pInfo->pStringIndex;
		PsbArray arr1(code); code += arr1.nBytes;
		PsbArray arr2(code); code += arr2.nBytes;
		PsbArray arr3(code); code += arr3.nBytes;
		
		vector<char> reverse_name;
		ULONG c1 = arr2[arr3[idx]];
		while (c1)
		{
			ULONG c2 = arr2[c1];
			reverse_name.push_back((CHAR)(c1 - arr1[c2]));
			c1 = c2;
		}

		std::reverse(reverse_name.begin(), reverse_name.end());

		return string(&reverse_name[0], reverse_name.size());
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
		if (Object)
		{
			delete Object;
		}
	}

	PSBObject* DetachObject()
	{
		PSBObject *obj = Object;
		Object = nullptr;
		return obj;
	}

	bool InitFromBuff(LPBYTE buff, ULONG size)
	{
		NTSTATUS Status;

		if (size > 8)
		{
			if (memcmp(buff, "PSB", 4))
			{
				LPBYTE uncomp = NULL;

				if (*(PDWORD)(buff) == LZ4_MAGIC)
					uncomp = getDataFromLz4(buff, size);
				else
					uncomp = getDataFromMDF(buff, size);
				
				if (uncomp)
				{
					HeapFree(GetProcessHeap(), 0, buff);
					buff = uncomp;
				}
			}

			if (size < 64 || RtlCompareMemory(buff, "PSB", 4) != 4)
				return false;
			if (Object)
			{
				delete Object;
			}

			Object = new PSBObject(buff, size, Status);
			return NT_SUCCESS(Status) ? true : false;
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
	string             lastDictName; // for bitmap dump
	wstring            PsbImgPath;
	string             BuildScript;
	ULONG              m_GlobalRetryCount;

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

	PSBDumper(LPWSTR Path)
	{
		PsbImgPath = Path;
		auto Index = PsbImgPath.find_last_of(L".");
		PsbImgPath = PsbImgPath.substr(0, Index);
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
					BuildScript += "\\\"";
				else if (ch == '\\')
					BuildScript += "\\\\";
				else if (ch == 0x08)
					BuildScript += "\\b";
				else if (ch == 0x0c)
					BuildScript += "\\f";
				else if (ch == 0x0a)
					BuildScript += "\\n";
				else if (ch == 0x0d)
					BuildScript += "\\r";
				else if (ch == 0x09)
					BuildScript += "\\t";
				else if ((UCHAR)ch < 0x20)
				{
					CHAR buf[256];
					wsprintfA(buf, "\\u%04x", ch);
					BuildScript += buf;
				}
				else
					BuildScript += (char)ch;
			}
			BuildScript += '"';
		}
		else
		{
			BuildScript += "\"\"";
		}
	}

	void dumpArray(const PSBNode &node, const string& indentstr)
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
		string indentstr2 = indentstr + INDENT;
		for (ULONG i = 0; i < (ULONG)count; ++i)
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

	BOOLEAN dumpBitmap(const PSBNode &node, const string& indentstr)
	{
		PsbArray arr1(node.pCode + 1);
		PsbArray arr2(node.pCode + 1 + arr1.nBytes);

		map<string, PSBNode> bitmapInfo;
		int foundBitmap = -2;
		for (ULONG i = 0; i < arr1.nElementCount; ++i)
		{
			string name = node.ProbeDictName(arr1[i]);
			bitmapInfo[name] = PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes);
			if (name == "width" || name == "height" || name == "pixel")
				++foundBitmap;
		}

		if (foundBitmap <= 0)
			return false;

		ULONG width = bitmapInfo.find("truncated_width") == bitmapInfo.end() ?
			(ULONG)bitmapInfo["width"].AsInt() : (ULONG)bitmapInfo["truncated_width"].AsInt();

		ULONG height = bitmapInfo.find("truncated_height") == bitmapInfo.end() ?
			(ULONG)bitmapInfo["height"].AsInt() : (ULONG)bitmapInfo["truncated_height"].AsInt();

		bitmapInfo.erase(bitmapInfo.find("width"));
		bitmapInfo.erase(bitmapInfo.find("height"));

		auto itr = bitmapInfo.find("truncated_width");
		if (itr != bitmapInfo.end())
			bitmapInfo.erase(itr);


		itr = bitmapInfo.find("truncated_height");
		if (itr != bitmapInfo.end())
			bitmapInfo.erase(itr);

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
		if (it != bitmapInfo.end() && !lstrcmpA(it->second.AsString(), "RL"))
		{
			unsigned char *uncomp = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pixelbytes * width * height);
			if (pixelbytes == 1)
				DecompressRLE<unsigned char>(uncomp, pixeldata, pixdatasize);
			else
				DecompressRLE<unsigned long>(uncomp, pixeldata, pixdatasize);
			
			pixeldata = uncomp;
			allocBitmap = true;
			bitmapInfo.erase(bitmapInfo.find("compress"));
		}
		bitmapInfo.erase(bitmapInfo.find("pixel"));

		//
		WriteBmp(lastDictName, width, height, pixeldata, pal, pixelbytes);

		if (allocBitmap) 
			HeapFree(GetProcessHeap(), 0, (PVOID)pixeldata);

		BuildScript += "(const) %[\r\n";
		string indentstr2 = indentstr + INDENT;
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


	void WriteFile(string& FileName, LPBYTE Buffer, ULONG Size)
	{
		wstring    FilePath;
		NtFileDisk File;
		WCHAR      WideFileName[MAX_PATH];

		RtlZeroMemory(WideFileName, countof(WideFileName) * sizeof(WCHAR));
		CreateDirectoryW(PsbImgPath.c_str(), NULL);

		MultiByteToWideChar(CP_UTF8, 0, FileName.c_str(), FileName.length(), WideFileName, MAX_PATH);

		FilePath =  PsbImgPath + L"\\";
		FilePath += WideFileName;

		File.Create(FilePath.c_str());
		File.Write(Buffer, Size);
		File.Close();
	}


	void WriteBmp(string& FileName, int width, int height, LPCBYTE pixeldata, LPCBYTE pal, int pixelbytes)
	{
		NtFileDisk File;
		wstring    FilePath;
		WCHAR      WideFileName[MAX_PATH];

		ULONG bmppitch = width * pixelbytes;
		bmppitch = (((bmppitch - 1) >> 2) + 1) << 2;

		CreateDirectoryW(PsbImgPath.c_str(), NULL);
		RtlZeroMemory(WideFileName, countof(WideFileName) * sizeof(WCHAR));

		MultiByteToWideChar(CP_UTF8, 0, FileName.c_str(), FileName.length(), WideFileName, MAX_PATH);

		FilePath =  PsbImgPath + L"\\";
		FilePath += WideFileName;
		FilePath += L".bmp";

		File.Create(FilePath.c_str());

		USHORT uidata = 0x4d42;
		File.Write(&uidata, 2);  /* bfType */
		uidata = (USHORT)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmppitch * height + (pixelbytes == 1 ? 1024 : 0));
		File.Write(&uidata, 4);  /* bfSize */
		uidata = 0;
		File.Write(&uidata, 2);  /* bfReserved1 */
		File.Write(&uidata, 2);  /* bfReserved2 */
		uidata = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (pixelbytes == 1 ? 1024 : 0);
		File.Write(&uidata, 4);  /* bfOffBits */
		uidata = sizeof(BITMAPINFOHEADER);
		File.Write(&uidata, 4);  /* biSize */
		File.Write(&width, 4);  /* biWidth */
		uidata = -height;
		File.Write(&uidata, 4);  /* biHeight */
		uidata = 1;
		File.Write(&uidata, 2);  /* biPlanes */
		uidata = pixelbytes * 8;
		File.Write(&uidata, 2);  /* biBitCount */
		uidata = BI_RGB;
		File.Write(&uidata, 4);  /* biCompression */
		uidata = 0;
		File.Write(&uidata, 4);  /* biSizeImage */
		File.Write(&uidata, 4);  /* biXPelsPerMeter */
		File.Write(&uidata, 4);  /* biYPelsPerMeter */
		File.Write(&uidata, 4);  /* biClrUsed */
		File.Write(&uidata, 4);  /* biClrImportant */

		// write palette
		if (pixelbytes == 1)
			File.Write((PVOID)pal, 1024);

		// write pixel
		static char paddingData[16];
		RtlZeroMemory(paddingData, sizeof(paddingData));
		int paddingSize = bmppitch - width * pixelbytes;
		
		const unsigned char *line = pixeldata;
		int srcpitch = width * pixelbytes;

		for (int y = 0; y < height; ++y)
		{
			File.Write((PVOID)line, srcpitch);
			line += srcpitch;
			if (paddingSize) File.Write(paddingData, paddingSize);
		}

		File.Close();
	}

	void SaveEmoteImage(string& FileName, PBYTE Stream, const PsbTextureMetaData& meta)
	{
		auto pixels = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, meta.Width * meta.Height * 4);

		if (!pixels)
		{
			OutputInfo(L"Couldn't save emote image...\n");
			return;
		}

		if (GlobalData::GetGlobalData()->DebugOn)
			PrintConsoleW(L"Emote size : %d x %d\n", meta.Width, meta.Height);

		if (!lstrcmpA("RGBA8", meta.TexType))
		{
			ReadRgba8(Stream, meta, pixels);
			WriteBmp(FileName, meta.Width, meta.Height, pixels, NULL, 4);
		}
		else if (!lstrcmpA("RGBA4444", meta.TexType))
		{
			ReadRgba4444(Stream, meta, pixels);
			WriteBmp(FileName, meta.Width, meta.Height, pixels, NULL, 4);
		}
		else
		{
			WriteFile(FileName, Stream, meta.DataSize);
		}

		if (pixels)
			HeapFree(GetProcessHeap(), 0, pixels);
	}

	USHORT ToUInt16(LPBYTE value, ULONG index)
	{
		return (USHORT)(value[index] | value[index + 1] << 8);
	}

	void ReadRgba4444(LPBYTE input, const PsbTextureMetaData& meta, LPBYTE output)
	{
		int dst_stride = meta.Width * 4;
		int src_stride = meta.FullWidth * 2;
		int dst = 0;
		auto row = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, src_stride);

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
		HeapFree(GetProcessHeap(), 0, row);
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

	void dumpDict(const PSBNode &node, const string& indentstr)
	{
		PsbArray arr1(node.pCode + 1);
		PsbArray arr2(node.pCode + 1 + arr1.nBytes);
		if (arr1.nElementCount == 0)
		{
			BuildScript += "(const) %[]";
			return;
		}

		BuildScript += "(const) %[\r\n";
		string indentstr2 = indentstr + INDENT;
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

	void dump(const PSBNode &node, const string& indentstr = "")
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
			wsprintfA(NumStr, "%d", node.AsInt());
			BuildScript += NumStr;
			break;

		case PsbValueFloat:
			memset(NumStr, 0, sizeof(NumStr));
			
			_gcvt(node.AsDouble(), 60, NumStr);
			
			if (NumStr[lstrlenA(NumStr) - 1] == '.')
				lstrcatA(NumStr, "0");
			
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

			map<string, PSBNode> EmoteInfo;
			for (ULONG i = 0; i < arr1.nElementCount; ++i)
			{
				string name = node.ProbeDictName(arr1[i]);
				EmoteInfo[name] = PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes);
			}

			auto it = EmoteInfo.find("source");
			if (it == EmoteInfo.end())
				return false;
			else
			{
				const PSBNode& sourceNode = it->second;

				PsbArray arr3(sourceNode.pCode + 1);
				PsbArray arr4(sourceNode.pCode + 1 + arr3.nBytes);

				map<string, PSBNode> EmoteTextureInfo;
				for (ULONG i = 0; i < arr3.nElementCount; ++i)
				{
					string name = node.ProbeDictName(arr3[i]);
					EmoteTextureInfo[name] = PSBNode(sourceNode.Object, sourceNode.pCode + 1 + arr4[i] + arr3.nBytes + arr4.nBytes);
				}

				for (auto& emoteList : EmoteTextureInfo)
				{

					PsbArray arr5(emoteList.second.pCode + 1);
					PsbArray arr6(emoteList.second.pCode + 1 + arr5.nBytes);

					map<string, PSBNode> EmoteChunk;

					for (ULONG i = 0; i < arr5.nElementCount; ++i)
					{
						string name = node.ProbeDictName(arr5[i]);
						EmoteChunk[name] = PSBNode(emoteList.second.Object, emoteList.second.pCode + 1 + arr6[i] + arr5.nBytes + arr6.nBytes);
					}

					if (EmoteChunk.find("texture") == EmoteChunk.end())
						return false;

					const PSBNode& textureMeta = EmoteChunk["texture"];

					string FileName = emoteList.first;


					PsbArray arr7(textureMeta.pCode + 1);
					PsbArray arr8(textureMeta.pCode + 1 + arr7.nBytes);

					map<string, PSBNode> EmoteChunkInfo;
					for (ULONG i = 0; i < arr7.nElementCount; ++i)
					{
						string name = node.ProbeDictName(arr7[i]);
						EmoteChunkInfo[name] = PSBNode(textureMeta.Object, textureMeta.pCode + 1 +
							arr8[i] + arr7.nBytes + arr8.nBytes);
					}

					ULONG pixdatasize, Index;
					ULONG width = (ULONG)EmoteChunkInfo["width"].AsInt();
					ULONG height = (ULONG)EmoteChunkInfo["height"].AsInt();
					ULONG truncated_width = (ULONG)EmoteChunkInfo["truncated_width"].AsInt();
					ULONG truncated_height = (ULONG)EmoteChunkInfo["truncated_height"].AsInt();
					PCSTR TextureType = EmoteChunkInfo["type"].AsString();
					LPCBYTE pixeldata = EmoteChunkInfo["pixel"].AsBinary(pixdatasize, Index);

					PsbTextureMetaData info;
					info.BPP = 32;
					info.FullWidth = width;
					info.FullHeight = height;
					info.Width = truncated_width;
					info.Height = truncated_height;
					info.DataSize = pixdatasize;

					lstrcpyA(info.TexType, TextureType);

					SaveEmoteImage(FileName, (LPBYTE)pixeldata, info);
				}
			}
		}
		return true;
	}


	VOID dumpPimg(const PSBNode& node)
	{
		PsbArray arr1(node.pCode + 1);
		PsbArray arr2(node.pCode + 1 + arr1.nBytes);

		map<string, PSBNode> PimgInfo;
		for (ULONG i = 0; i < arr1.nElementCount; ++i)
		{
			string name = node.ProbeDictName(arr1[i]);
			PimgInfo[name] = PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes);
		}

		for (auto& it : PimgInfo)
		{
			if (it.second.GetType() == ePsbValueType::PsbValueBinary)
			{
				string FileName = it.first;
				ULONG  FileSize, Index;
				LPCBYTE Buffer = it.second.AsBinary(FileSize, Index);

				WriteFile(FileName, (LPBYTE)Buffer, FileSize);
			}
		}
	}

	void start(const PSBNode &node, const wstring& FileName)
	{
		auto&& FileExtension = GetExtensionName(FileName);

		dump(node);

		if (GlobalData::GetGlobalData()->PsbFlagOn(PSB_DECOM) ||
			GlobalData::GetGlobalData()->PsbFlagOn(PSB_ALL))
		{
			//Decompile Script
			NtFileDisk    File;

			wstring ScriptFile = PsbImgPath + L".tjs";
			File.Create(ScriptFile.c_str());
			File.Write((PVOID)BuildScript.c_str(), BuildScript.length());
			File.Close();
		}

		if (GlobalData::GetGlobalData()->PsbFlagOn(PSB_ALL) ||
			GlobalData::GetGlobalData()->PsbFlagOn(PSB_ANM) ||
			GlobalData::GetGlobalData()->PsbFlagOn(PSB_IMAGE))
		{
			//Dump Binary File: Emote, RL/Bitmap, Other
			if (!lstrcmpiW(FileExtension.c_str(), L"PIMG"))
				dumpPimg(node);
			else if (!dumpEmote(node))
				dumpAllFile(node);
		}

		if (GlobalData::GetGlobalData()->PsbFlagOn(PSB_ALL) ||
			GlobalData::GetGlobalData()->PsbFlagOn(PSB_TEXT))
		{
			dumpText(node);
		}
	}


	void dumpText(const PSBNode& node)
	{
		PsbArray arr1(node.pCode + 1);
		PsbArray arr2(node.pCode + 1 + arr1.nBytes);

		map<string, PSBNode> SceneInfo;
		for (ULONG i = 0; i < arr1.nElementCount; ++i)
		{
			string name = node.ProbeDictName(arr1[i]);
			SceneInfo[name] = PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes);
		}

		if (SceneInfo.find("scenes") == SceneInfo.end() && SceneInfo.find("list") == SceneInfo.end())
			return;

		NtFileDisk File;

		wstring FileName = PsbImgPath + L".ks.txt";
		File.Create(FileName.c_str());

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

				for (ULONG i = 0; i < (ULONG)count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrScene[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					map<string, PSBNode> TextInfo;
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						string name2 = TextNode.ProbeDictName(arr3[j]);
						TextInfo[name2] = PSBNode(TextNode.Object, TextNode.pCode + 1 + arr4[j] + arr3.nBytes + arr4.nBytes);
					}

					if (TextInfo.find("texts") == TextInfo.end())
						continue;

					PSBNode TextChunkNode = TextInfo["texts"];

					int count2 = PsbGetCount(TextChunkNode.pCode + 1);
					if (count2 == 0)
						break;

					PsbArray arrText(TextChunkNode.pCode + 1);
					LPBYTE code2 = TextChunkNode.pCode + arrText.nBytes + 1;

					for (ULONG k = 0; k < (ULONG)count2; k++)
					{
						PSBNode TextListNode(node.Object, code2 + arrText[k]);

						int count3 = PsbGetCount(TextListNode.pCode + 1);
						if (count3 == 0)
							break;

						PsbArray arrAtomText(TextListNode.pCode + 1);
						LPBYTE code3 = TextListNode.pCode + arrAtomText.nBytes + 1;

						for (ULONG idx = 0; idx < 3; idx++)
						{
							PSBNode TextString(node.Object, code3 + arrAtomText[idx]);

							if (TextString.GetType() == ePsbValueType::PsbValueString)
							{
								if (lstrlenA(TextString.AsString()))
									File.Print(NULL, "%s\r\n", TextString.AsString());
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

				for (ULONG i = 0; i < (ULONG)count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrScene[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					map<string, PSBNode> TextInfo;
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						string name2 = TextNode.ProbeDictName(arr3[j]);
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

					for (ULONG k = 0; k < (ULONG)count2; k++)
					{
						PSBNode SelNode(node.Object, code2 + arrText[k]);

						PsbArray arr5(SelNode.pCode + 1);
						PsbArray arr6(SelNode.pCode + 1 + arr5.nBytes);

						map<string, PSBNode> SelInfo;
						for (ULONG m = 0; m < arr5.nElementCount; ++m)
						{
							string name3 = SelNode.ProbeDictName(arr5[m]);
							SelInfo[name3] = PSBNode(SelNode.Object, SelNode.pCode + 1 + arr6[m] + arr5.nBytes + arr6.nBytes);
						}


						if (SelInfo.find("text") != SelInfo.end() &&
							lstrlenA(SelInfo["text"].AsString()))
						{
							File.Print(NULL, "%s\r\n", SelInfo["text"].AsString());
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

				for (ULONG i = 0; i < (ULONG)count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrScene[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					map<string, PSBNode> TextInfo;
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						string name2 = TextNode.ProbeDictName(arr3[j]);
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

					for (ULONG k = 0; k < (ULONG)count2; k++)
					{
						PSBNode SelNode(node.Object, code2 + arrText[k]);

						PsbArray arr5(SelNode.pCode + 1);
						PsbArray arr6(SelNode.pCode + 1 + arr5.nBytes);

						map<string, PSBNode> SelInfo;
						for (ULONG m = 0; m < arr5.nElementCount; ++m)
						{
							string name3 = SelNode.ProbeDictName(arr5[m]);
							SelInfo[name3] = PSBNode(SelNode.Object, SelNode.pCode + 1 + arr6[m] + arr5.nBytes + arr6.nBytes);
						}


						if (SelInfo.find("runLineStr") != SelInfo.end() &&
							lstrlenA(SelInfo["runLineStr"].AsString()))
						{
							File.Print(NULL, "%s\r\n", SelInfo["runLineStr"].AsString());
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

				for (ULONG i = 0; i < (ULONG)count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrList[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					map<string, PSBNode> TitleInfo;
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						string name2 = TextNode.ProbeDictName(arr3[j]);
						TitleInfo[name2] = PSBNode(TextNode.Object, TextNode.pCode + 1 + arr4[j] + arr3.nBytes + arr4.nBytes);
					}

					if (TitleInfo.find("title") != TitleInfo.end() &&
						lstrlenA(TitleInfo["title"].AsString()))
					{
						File.Print(NULL, "%s\r\n", TitleInfo["title"].AsString());
					}

				}

				for (ULONG i = 0; i < (ULONG)count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrList[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					map<string, PSBNode> TitleInfo;
					//selects
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						string name2 = TextNode.ProbeDictName(arr3[j]);
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

						for (ULONG k = 0; k < (ULONG)SelectCount; k++)
						{
							PSBNode SelectNode(node.Object, code3 + arrSelects[k]);

							PsbArray arr5(SelectNode.pCode + 1);
							PsbArray arr6(SelectNode.pCode + 1 + arr5.nBytes);

							map<string, PSBNode> SelectsInfo;
							for (ULONG m = 0; m < arr5.nElementCount; ++m)
							{
								string name3 = SelectNode.ProbeDictName(arr5[m]);
								SelectsInfo[name3] = PSBNode(SelectNode.Object, SelectNode.pCode + 1 + arr6[m] + arr5.nBytes + arr6.nBytes);
							}

							if (SelectsInfo.find("runLineStr") != SelectsInfo.end() &&
								lstrlenA(SelectsInfo["runLineStr"].AsString()))
							{
								File.Print(NULL, "%s\r\n", SelectsInfo["runLineStr"].AsString());
							}
						}
					}
				}

				for (ULONG i = 0; i < (ULONG)count; ++i)
				{
					PSBNode TextNode(node.Object, code + arrList[i]);

					PsbArray arr3(TextNode.pCode + 1);
					PsbArray arr4(TextNode.pCode + 1 + arr3.nBytes);

					map<string, PSBNode> TitleInfo;
					//selects
					for (ULONG j = 0; j < arr3.nElementCount; ++j)
					{
						string name2 = TextNode.ProbeDictName(arr3[j]);
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

						for (ULONG k = 0; k < (ULONG)SelectCount; k++)
						{
							PSBNode SelectNode(node.Object, code3 + arrSelects[k]);

							PsbArray arr5(SelectNode.pCode + 1);
							PsbArray arr6(SelectNode.pCode + 1 + arr5.nBytes);

							map<string, PSBNode> SelectsInfo;
							for (ULONG m = 0; m < arr5.nElementCount; ++m)
							{
								string name3 = SelectNode.ProbeDictName(arr5[m]);
								SelectsInfo[name3] = PSBNode(SelectNode.Object, SelectNode.pCode + 1 + arr6[m] + arr5.nBytes + arr6.nBytes);
							}

							if (SelectsInfo.find("text") != SelectsInfo.end() &&
								lstrlenA(SelectsInfo["text"].AsString()))
							{
								File.Print(NULL, "%s\r\n", SelectsInfo["text"].AsString());
							}
						}
					}
				}
			}
		}

		File.Close();
	}


	VOID dumpAllFile(const PSBNode& node)
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
			return;

		PsbArray arr(node.pCode + 1);
		LPBYTE code = node.pCode + arr.nBytes + 1;

		for (ULONG i = 0; i < (ULONG)count; ++i)
		{
			dumpAllFile(PSBNode(node.Object, code + arr[i]));
		}
	}


	void dumpDictInternal(const PSBNode &node)
	{
		PsbArray arr1(node.pCode + 1);
		PsbArray arr2(node.pCode + 1 + arr1.nBytes);

		if (arr1.nElementCount == 0)
			return;

		if (dumpBitmapInternal(node))
			return;

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

		map<string, PSBNode> bitmapInfo;
		int foundBitmap = -2;
		for (ULONG i = 0; i < arr1.nElementCount; ++i)
		{
			string name = node.ProbeDictName(arr1[i]);
			bitmapInfo[name] = PSBNode(node.Object, node.pCode + 1 + arr2[i] + arr1.nBytes + arr2.nBytes);
			if (name == "width" || name == "height" || name == "pixel")
			{
				++foundBitmap;
			}
		}

		if (foundBitmap <= 0)
			return false;

		ULONG width  = (ULONG)bitmapInfo["width"].AsInt();
		ULONG height = (ULONG)bitmapInfo["height"].AsInt();

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
			unsigned char *uncomp = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pixelbytes * width * height);
			if (pixelbytes == 1)
				DecompressRLE<unsigned char>(uncomp, pixeldata, pixdatasize);
			else
				DecompressRLE<unsigned long>(uncomp, pixeldata, pixdatasize);
			
			pixeldata = uncomp;
			allocBitmap = true;
			bitmapInfo.erase(bitmapInfo.find("compress"));
		}
		bitmapInfo.erase(bitmapInfo.find("pixel"));

		//
		WriteBmp(lastDictName, width, height, pixeldata, pal, pixelbytes);

		if (allocBitmap)
			HeapFree(GetProcessHeap(), 0, (PVOID)pixeldata);

		for (it = bitmapInfo.begin(); it != bitmapInfo.end(); ++it)
			dumpAllFile(it->second);

		return TRUE;
	}

	wstring GetExtensionName(const wstring& Name)
	{
		auto Index = Name.find_last_of(L'.');

		if (Index == wstring::npos)
			return nullptr;
		else
		{
			wstring Result;
			for (auto& EachChar : Name.substr(Index + 1))
			{
				Result += CHAR_UPPER(EachChar);
			}
			return Result;
		}
	}
};

int WINAPI ExtractPsb(IStream* InFile, BOOL Image, BOOL Script, const wstring& ExtName, const WCHAR* lpOutName)
{ 
	if (InFile == nullptr)
		return -1;

	ULARGE_INTEGER uMoved;
	LARGE_INTEGER  uStart;
	uStart.QuadPart = 0;
	InFile->Seek(uStart, STREAM_SEEK_SET, &uMoved);

	STATSTG t;
	InFile->Stat(&t, STATFLAG_DEFAULT);
	ULONG nRead;
	unsigned char *buf = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, t.cbSize.LowPart);
	InFile->Read(buf, t.cbSize.LowPart, &nRead);

	PSBFile psbf;
	if (!psbf.InitFromBuff(buf, t.cbSize.LowPart)) 
	{
		//SafeDumper
		OutputInfo(L"Invalid Psb file\n");
		if (buf)
			HeapFree(GetProcessHeap(), 0, buf);

		return -2;
	}

	PSBDumper dumper((LPWSTR)lpOutName);
	dumper.start(psbf.GetRootNode(), ExtName);

	return 0;
}
