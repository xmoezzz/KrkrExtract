#ifndef _ImageEx_
#define _ImageEx_

#include <Windows.h>

#define RGBA(r, g, b, a) (UInt32)(((u32)(u8)(r)) | (((u32)(u8)(g) << 8)) | (((u32)(u8)(b) << 16)) | (((u32)(u8)(a) << 24)))

#pragma pack(push, 1)

typedef struct
{
	UINT16 wTag;                   // 0x00
	UINT32 dwFileSize;             // 0x02
	UINT32 dwReserve;              // 0x06
	UINT32 dwRawOffset;            // 0x0A
	struct
	{
		UINT32 dwInfoHeaderSize;   // 0x0E
		UINT32 dwWidth;            // 0x12
		UINT32 dwHeight;           // 0x16
		UINT16 wLayer;             // 0x1A
		UINT16 wBit;               // 0x1C
		UINT32 dwCompressed;       // 0x1E
		UINT32 dwSizeImage;        // 0x22
		INT32  iXPelsPerMeter;     // 0x26
		INT32  iYPelsPerMeter;     // 0x2A
		UINT32 dwClrUsed;          // 0x2E
		UINT32 dwClrImportant;     // 0x32
	} Info;
} IMG_BITMAP_HEADER, SBitMapHeader, *PBitMapHeader, *LPBitMapHeader;

typedef struct
{
	UCHAR  ID;
	UCHAR  byPaletteType;
	UCHAR  byImageType;
	UINT16 usPaletteEntry;
	UINT16 usPaletteLength;
	UCHAR  byPaletteBitCount;
	UINT16 usOriginX;
	UINT16 usOriginY;
	UINT16 usWidth;
	UINT16 usHeight;
	UCHAR  byPixelDepth;
	UCHAR  byImageDescription;
} IMG_TGA_HEADER, STGAHeader;

typedef struct
{
	union
	{
		LPVOID lpBuffer;
		PBYTE  pbBuffer;
	};
	INT32  Width;
	INT32  Height;
	INT32  BitsPerPixel;
	INT32  Stride;
	LPVOID lpExtraInfo;
	INT32  ExtraInfoSize;
} UCIInfo, *PUCIInfo, *LPUCIInfo;

#pragma pack(pop)

BOOL
__fastcall
InitBitmapHeader(
IMG_BITMAP_HEADER  *pHeader,
INT32               Width,
INT32               Height,
INT32               BitsPerPixel,
PINT32              pStride
);


#endif
