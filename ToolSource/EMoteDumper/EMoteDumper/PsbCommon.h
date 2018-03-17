#pragma once

#include <Windows.h>
#include <string>

#pragma pack(push, 1)

typedef struct PSB_HEADER_V2
{
	DWORD HeaderMagic;
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
	std::string TexType;
	ULONG       FullWidth;
	ULONG       FullHeight;
	ULONG       DataOffset;
	ULONG       DataSize;
}PsbTextureMetaData, *pPsbTextureMetaData;
